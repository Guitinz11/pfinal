// ============================================================
//  Garra hidráulica (seringas + água) controlada pelo celular
//
//  NÃO PRECISA DE NENHUMA BIBLIOTECA EXTRA: usa só o que já vem
//  com o suporte ao ESP32 na Arduino IDE (WiFi, WebServer e LEDC).
//
//  O ESP32 cria uma rede Wi-Fi, serve uma página web e recebe
//  comandos em http://192.168.4.1/cmd?c=<comando>
//
//  Comandos:
//    s,<junta>,<graus>   define o alvo de uma junta   ex: s,0,120
//    v,<graus/s>         define a velocidade          ex: v,90
//    x                   PARAR (congela tudo onde está)
//    h                   HOME (volta às posições iniciais)
//    p                   ping (mantém a conexão "viva")
//  Toda resposta é um JSON com o estado atual.
// ============================================================
#include <WiFi.h>
#include <WebServer.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#include "config.h"
#include "pagina.h"

float atual[NUM_JUNTAS];   // posição atual (vai andando até o alvo)
int   alvo[NUM_JUNTAS];    // posição pedida pelo celular
float velocidade = VELOCIDADE_PADRAO;
bool  parado = false;
unsigned long ultimoComando = 0;

WebServer server(80);

// ------------------------------------------------------------
//  Servos via LEDC (PWM nativo do ESP32), 50 Hz, 14 bits
// ------------------------------------------------------------
#define SERVO_FREQ 50
#define SERVO_BITS 14

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // Suporte ESP32 versão 3.x
  void servoIniciar(int i) { ledcAttach(JUNTAS[i].pino, SERVO_FREQ, SERVO_BITS); }
  void servoDuty(int i, uint32_t d) { ledcWrite(JUNTAS[i].pino, d); }
#else
  // Suporte ESP32 versão 2.x (um canal LEDC por junta)
  void servoIniciar(int i) { ledcSetup(i, SERVO_FREQ, SERVO_BITS); ledcAttachPin(JUNTAS[i].pino, i); }
  void servoDuty(int i, uint32_t d) { ledcWrite(i, d); }
#endif

void escreverServo(int i, float pos) {
  if (JUNTAS[i].invertido) pos = 180.0f - pos;
  float us = PULSO_MIN_US + (PULSO_MAX_US - PULSO_MIN_US) * pos / 180.0f;
  uint32_t duty = (uint32_t)(us * ((1 << SERVO_BITS) - 1) / (1000000.0f / SERVO_FREQ));
  servoDuty(i, duty);
}

// ------------------------------------------------------------
void pararTudo() {
  for (int i = 0; i < NUM_JUNTAS; i++) alvo[i] = (int)roundf(atual[i]);
  parado = true;
}

String estadoJson() {
  String s = "{\"v\":" + String((int)velocidade) +
             ",\"p\":" + (parado ? "true" : "false") + ",\"j\":[";
  for (int i = 0; i < NUM_JUNTAS; i++) {
    if (i) s += ",";
    s += "{\"n\":\"" + String(JUNTAS[i].nome) + "\"" +
         ",\"min\":" + String(JUNTAS[i].min) +
         ",\"max\":" + String(JUNTAS[i].max) +
         ",\"a\":"   + String(alvo[i]) + "}";
  }
  return s + "]}";
}

void tratarMensagem(const char* msg) {
  ultimoComando = millis();
  int j, v;
  switch (msg[0]) {
    case 's':
      if (sscanf(msg, "s,%d,%d", &j, &v) == 2 && j >= 0 && j < NUM_JUNTAS) {
        alvo[j] = constrain(v, JUNTAS[j].min, JUNTAS[j].max);
        parado = false;
      }
      break;
    case 'v':
      if (sscanf(msg, "v,%d", &v) == 1)
        velocidade = constrain(v, VELOCIDADE_MIN, VELOCIDADE_MAX);
      break;
    case 'x':
      pararTudo();
      Serial.println("PARAR acionado");
      break;
    case 'h':
      for (int i = 0; i < NUM_JUNTAS; i++) alvo[i] = JUNTAS[i].inicial;
      parado = false;
      break;
    default:   // 'p' (ping) ou desconhecido: só responde o estado
      break;
  }
}

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Garra hidraulica ESP32 ===");

  for (int i = 0; i < NUM_JUNTAS; i++) {
    atual[i] = alvo[i] = JUNTAS[i].inicial;
    servoIniciar(i);
    escreverServo(i, atual[i]);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_SENHA);
  Serial.printf("Rede: %s  |  Senha: %s\n", WIFI_SSID, WIFI_SENHA);
  Serial.print("Abra no celular: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    server.send(200, "text/html", PAGINA_HTML);
  });
  server.on("/cmd", []() {
    if (server.hasArg("c")) tratarMensagem(server.arg("c").c_str());
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

  static unsigned long tMov = 0;
  unsigned long agora = millis();
  if (agora - tMov < PERIODO_MS) return;
  tMov = agora;

  // --- Movimento suave em velocidade constante ---
  float passo = velocidade * PERIODO_MS / 1000.0f;
  bool movendo = false;
  for (int i = 0; i < NUM_JUNTAS; i++) {
    float d = alvo[i] - atual[i];
    if (fabsf(d) <= passo) {
      atual[i] = alvo[i];
    } else {
      atual[i] += (d > 0 ? passo : -passo);
      movendo = true;
    }
    escreverServo(i, atual[i]);
  }

  // --- Segurança: perdeu o celular no meio do movimento -> congela ---
  if (movendo && agora - ultimoComando > TIMEOUT_MS) {
    pararTudo();
    Serial.println("Sem comunicacao: garra congelada");
  }
}
