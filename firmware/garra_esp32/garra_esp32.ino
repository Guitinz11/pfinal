// ============================================================
//  Garra hidráulica (seringas + água) controlada pelo celular
//  ESP32 cria uma rede Wi-Fi, serve uma página web e recebe
//  comandos via WebSocket.
//
//  Protocolo (texto, celular -> ESP32):
//    s,<junta>,<graus>   define o alvo de uma junta   ex: "s,0,120"
//    v,<graus/s>         define a velocidade          ex: "v,90"
//    x                   PARAR (congela tudo onde está)
//    h                   HOME (volta às posições iniciais)
//    p                   ping (mantém a conexão "viva")
//
//  ESP32 -> celular: JSON com o estado (juntas, limites, alvos, velocidade)
// ============================================================
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>

#include "config.h"
#include "pagina.h"

Servo servos[NUM_JUNTAS];
volatile float atual[NUM_JUNTAS];  // posição atual (vai andando até o alvo)
volatile int   alvo[NUM_JUNTAS];   // posição pedida pelo celular
volatile float velocidade = VELOCIDADE_PADRAO;
volatile bool  parado = false;
volatile bool  estadoMudou = true;
volatile unsigned long ultimoComando = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ------------------------------------------------------------
void escreverServo(int i, float pos) {
  int graus = (int)roundf(pos);
  if (JUNTAS[i].invertido) graus = 180 - graus;
  servos[i].write(graus);
}

void pararTudo() {
  for (int i = 0; i < NUM_JUNTAS; i++) alvo[i] = (int)roundf(atual[i]);
  parado = true;
  estadoMudou = true;
}

String estadoJson() {
  String s = "{\"v\":" + String((int)velocidade) +
             ",\"p\":" + (parado ? "true" : "false") + ",\"j\":[";
  for (int i = 0; i < NUM_JUNTAS; i++) {
    if (i) s += ",";
    s += "{\"n\":\"" + String(JUNTAS[i].nome) + "\"" +
         ",\"min\":" + JUNTAS[i].min +
         ",\"max\":" + JUNTAS[i].max +
         ",\"a\":"   + alvo[i] + "}";
  }
  return s + "]}";
}

// ------------------------------------------------------------
void tratarMensagem(const char* msg) {
  ultimoComando = millis();
  int j, v;
  switch (msg[0]) {
    case 's':
      if (sscanf(msg, "s,%d,%d", &j, &v) == 2 && j >= 0 && j < NUM_JUNTAS) {
        alvo[j] = constrain(v, JUNTAS[j].min, JUNTAS[j].max);
        parado = false;
        estadoMudou = true;
      }
      break;
    case 'v':
      if (sscanf(msg, "v,%d", &v) == 1) {
        velocidade = constrain(v, VELOCIDADE_MIN, VELOCIDADE_MAX);
        estadoMudou = true;
      }
      break;
    case 'x':
      pararTudo();
      Serial.println("PARAR acionado");
      break;
    case 'h':
      for (int i = 0; i < NUM_JUNTAS; i++) alvo[i] = JUNTAS[i].inicial;
      parado = false;
      estadoMudou = true;
      break;
    case 'p':
    default:
      break;
  }
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Celular #%u conectado\n", client->id());
    ultimoComando = millis();
    client->text(estadoJson());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Celular #%u desconectado\n", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len &&
        info->opcode == WS_TEXT && len < 32) {
      char buf[32];
      memcpy(buf, data, len);
      buf[len] = '\0';
      tratarMensagem(buf);
    }
  }
}

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Garra hidraulica ESP32 ===");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  for (int i = 0; i < NUM_JUNTAS; i++) {
    atual[i] = alvo[i] = JUNTAS[i].inicial;
    servos[i].setPeriodHertz(50);
    servos[i].attach(JUNTAS[i].pino, PULSO_MIN_US, PULSO_MAX_US);
    escreverServo(i, atual[i]);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_SENHA);
  Serial.printf("Rede: %s  |  Senha: %s\n", WIFI_SSID, WIFI_SENHA);
  Serial.print("Abra no celular: http://");
  Serial.println(WiFi.softAPIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/html", PAGINA_HTML);
  });
  server.onNotFound([](AsyncWebServerRequest* req) { req->redirect("/"); });
  server.begin();
}

void loop() {
  static unsigned long tMov = 0, tEstado = 0;
  unsigned long agora = millis();

  // --- Movimento suave em velocidade constante ---
  if (agora - tMov >= PERIODO_MS) {
    tMov = agora;
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
    if (movendo && (ws.count() == 0 || agora - ultimoComando > TIMEOUT_MS)) {
      pararTudo();
      Serial.println("Sem comunicacao: garra congelada");
    }
  }

  // --- Sincroniza todos os celulares conectados (no máx. 10x/s) ---
  if (estadoMudou && agora - tEstado >= 100) {
    tEstado = agora;
    estadoMudou = false;
    ws.textAll(estadoJson());
  }

  ws.cleanupClients();
}
