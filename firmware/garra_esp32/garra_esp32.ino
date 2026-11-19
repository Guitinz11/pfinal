// ============================================================
//  Garra hidráulica (seringas + água) com MOTORES DC + PONTE H
//  controlada pelo celular.
//
//  NÃO PRECISA DE NENHUMA BIBLIOTECA EXTRA: usa só o que já vem
//  com o suporte ao ESP32 na Arduino IDE (WiFi, WebServer e LEDC).
//
//  Funciona com ponte H L298N, L9110S, TB6612 ou parecidas
//  (2 entradas por motor). No L298N, deixe os jumpers ENA/ENB.
//
//  Comandos em http://192.168.4.1/cmd?c=<comando>
//    m,<motor>,<dir>   dir = 1 (botão +), -1 (botão −), 0 (parar)
//                      o celular repete enquanto o botão está apertado
//    v,<0-100>         velocidade em %
//    x                 PARAR tudo
//    p                 ping
//  Vários comandos podem ir juntos, separados por ';'  ex: m,0,1;m,1,-1
//  Toda resposta é um JSON com o estado atual.
// ============================================================
#include <WiFi.h>
#include <WebServer.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#include "config.h"
#include "pagina.h"

int   direcao[NUM_MOTORES];          // -1, 0, 1
float duty[NUM_MOTORES];             // força atual (0..1), sobe na rampa
unsigned long ultimoCmd[NUM_MOTORES];
unsigned long inicioGiro[NUM_MOTORES];
bool  travado[NUM_MOTORES];          // parou por tempo máximo: espera soltar o botão
int   velocidade = VELOCIDADE_PADRAO;

WebServer server(80);

// ------------------------------------------------------------
//  PWM via LEDC (nativo do ESP32), compatível com core 2.x e 3.x
// ------------------------------------------------------------
#define PWM_MAX ((1 << PWM_BITS) - 1)

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  void pwmIniciar(uint8_t pino, uint8_t) { ledcAttach(pino, PWM_FREQ, PWM_BITS); }
  void pwmEscrever(uint8_t pino, uint8_t, uint32_t v) { ledcWrite(pino, v); }
#else
  void pwmIniciar(uint8_t pino, uint8_t canal) { ledcSetup(canal, PWM_FREQ, PWM_BITS); ledcAttachPin(pino, canal); }
  void pwmEscrever(uint8_t, uint8_t canal, uint32_t v) { ledcWrite(canal, v); }
#endif

void aplicarMotor(int i) {
  int d = direcao[i];
  if (MOTORES[i].invertido) d = -d;
  uint32_t v = (uint32_t)(duty[i] * PWM_MAX);
  uint32_t a = (d > 0) ? v : 0;
  uint32_t b = (d < 0) ? v : 0;
  pwmEscrever(MOTORES[i].pinoA, i * 2,     a);
  pwmEscrever(MOTORES[i].pinoB, i * 2 + 1, b);
}

void pararMotor(int i) {
  direcao[i] = 0;
  duty[i] = 0;
  aplicarMotor(i);
}

void pararTudo() {
  for (int i = 0; i < NUM_MOTORES; i++) pararMotor(i);
}

// ------------------------------------------------------------
String estadoJson() {
  String s = "{\"v\":" + String(velocidade) + ",\"j\":[";
  for (int i = 0; i < NUM_MOTORES; i++) {
    if (i) s += ",";
    s += "{\"n\":\"" + String(MOTORES[i].nome) + "\"" +
         ",\"m\":\"" + String(MOTORES[i].botaoMenos) + "\"" +
         ",\"p\":\"" + String(MOTORES[i].botaoMais) + "\"" +
         ",\"d\":"   + String(direcao[i]) +
         ",\"t\":"   + (travado[i] ? "true" : "false") + "}";
  }
  return s + "]}";
}

void tratarMensagem(const char* msg) {
  int j, v;
  unsigned long agora = millis();
  switch (msg[0]) {
    case 'm':
      if (sscanf(msg, "m,%d,%d", &j, &v) == 2 && j >= 0 && j < NUM_MOTORES) {
        v = constrain(v, -1, 1);
        if (v == 0) { pararMotor(j); travado[j] = false; break; }
        if (travado[j]) { ultimoCmd[j] = agora; break; }   // ainda segurando: ignora
        if (v != direcao[j]) {               // começou a girar (ou trocou o sentido)
          duty[j] = 0;
          inicioGiro[j] = agora;
          direcao[j] = v;
        }
        ultimoCmd[j] = agora;
      }
      break;
    case 'v':
      if (sscanf(msg, "v,%d", &v) == 1) velocidade = constrain(v, VELOCIDADE_MIN, 100);
      break;
    case 'x':
      pararTudo();
      Serial.println("PARAR acionado");
      break;
    default:   // 'p' (ping)
      break;
  }
}

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Garra hidraulica ESP32 (motores DC) ===");

  for (int i = 0; i < NUM_MOTORES; i++) {
    pwmIniciar(MOTORES[i].pinoA, i * 2);
    pwmIniciar(MOTORES[i].pinoB, i * 2 + 1);
    pararMotor(i);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_SENHA);
  Serial.printf("Rede: %s  |  Senha: %s\n", WIFI_SSID, WIFI_SENHA);
  Serial.print("Abra no celular: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send(200, "text/html", PAGINA_HTML); });
  server.on("/cmd", []() {
    if (server.hasArg("c")) {            // vários comandos separados por ';'
      String c = server.arg("c");
      int ini = 0;
      while (ini < (int)c.length() && ini < 200) {
        int fim = c.indexOf(';', ini);
        if (fim < 0) fim = c.length();
        tratarMensagem(c.substring(ini, fim).c_str());
        ini = fim + 1;
      }
    }
    server.send(200, "application/json", estadoJson());
  });
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long t = 0;
  unsigned long agora = millis();
  if (agora - t < 10) return;
  t = agora;

  float alvo = velocidade / 100.0f;
  float passo = alvo * 10.0f / RAMPA_MS;

  for (int i = 0; i < NUM_MOTORES; i++) {
    // Destrava quando o botão foi solto (comandos pararam de chegar)
    if (travado[i] && agora - ultimoCmd[i] > TEMPO_SEM_COMANDO_MS) travado[i] = false;
    if (direcao[i] == 0) continue;

    // Segurança: botão solto / sinal perdido
    if (agora - ultimoCmd[i] > TEMPO_SEM_COMANDO_MS) {
      pararMotor(i);
      continue;
    }
    // Segurança: girando tempo demais sem soltar
    if (agora - inicioGiro[i] > TEMPO_MAX_MS) {
      Serial.printf("%s: tempo maximo atingido, parado\n", MOTORES[i].nome);
      pararMotor(i);
      travado[i] = true;
      continue;
    }
    // Partida suave
    duty[i] = min(alvo, duty[i] + passo);
    aplicarMotor(i);
  }
}
