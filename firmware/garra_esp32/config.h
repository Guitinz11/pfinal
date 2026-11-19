// ============================================================
//  CONFIGURAÇÃO DA GARRA HIDRÁULICA
//  Edite este arquivo para ajustar pinos, limites e rede.
// ============================================================
#pragma once
#include <Arduino.h>

// ---------- Rede Wi-Fi criada pelo ESP32 (modo Access Point) ----------
// O celular conecta nesta rede e abre http://192.168.4.1
#define WIFI_SSID   "Garra-ESP"
#define WIFI_SENHA  "garra1234"   // mínimo 8 caracteres

// ---------- Juntas (cada uma = 1 motor + 1 par de seringas) ----------
// 8 seringas = 4 pares = 4 juntas.
//
// min / max  : limites em graus do servo. CALIBRE com a hidráulica montada
//              para que o motor NUNCA force o êmbolo até o fim do curso.
// inicial    : posição ao ligar e ao apertar "Home".
// invertido  : true se o movimento ficou ao contrário do slider.
struct Junta {
  const char* nome;
  uint8_t     pino;
  int         min;
  int         max;
  int         inicial;
  bool        invertido;
};

#define NUM_JUNTAS 4

static const Junta JUNTAS[NUM_JUNTAS] = {
  // nome        pino  min  max  inicial invertido
  { "Base",       13,   10, 170,  90,    false },
  { "Ombro",      14,   30, 150,  90,    false },
  { "Cotovelo",   27,   30, 150,  90,    false },
  { "Garra",      26,   40, 140,  90,    false },
};

// ---------- Movimento ----------
#define VELOCIDADE_PADRAO  60    // graus por segundo (ajustável pelo celular)
#define VELOCIDADE_MIN      5
#define VELOCIDADE_MAX    360
#define PERIODO_MS         20    // intervalo de atualização dos servos

// ---------- Segurança ----------
// Se o celular parar de responder por este tempo, a garra congela onde está.
#define TIMEOUT_MS       3000

// Largura de pulso dos servos (MG996R / SG90 funcionam bem com 500–2400 µs)
#define PULSO_MIN_US      500
#define PULSO_MAX_US     2400
