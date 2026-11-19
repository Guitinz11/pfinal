// ============================================================
//  CONFIGURAÇÃO DA GARRA HIDRÁULICA (MOTORES DC + PONTE H)
//  Edite este arquivo para ajustar pinos, nomes e rede.
// ============================================================
#pragma once
#include <Arduino.h>

// ---------- Rede Wi-Fi criada pelo ESP32 (modo Access Point) ----------
// O celular conecta nesta rede e abre http://192.168.4.1
#define WIFI_SSID   "Garra-ESP"
#define WIFI_SENHA  "garra1234"   // mínimo 8 caracteres

// ---------- Motores (cada um = 1 motor DC + 1 par de seringas) ----------
// 8 seringas = 4 pares = 4 motores = 2 módulos de ponte H (2 motores cada).
//
// pinoA / pinoB : vão nas entradas da ponte H (IN1/IN2 ou IN3/IN4).
// botaoMenos / botaoMais : texto dos botões no celular.
// invertido     : true se o motor gira ao contrário do botão.
//
//  Ponte H 1:  IN1 = 32  IN2 = 33  (Base)    |  IN3 = 25  IN4 = 26  (Ombro)
//  Ponte H 2:  IN1 = 27  IN2 = 14  (Cotovelo)|  IN3 = 18  IN4 = 19  (Garra)
struct Motor {
  const char* nome;
  const char* botaoMenos;
  const char* botaoMais;
  uint8_t     pinoA;
  uint8_t     pinoB;
  bool        invertido;
};

#define NUM_MOTORES 4

static const Motor MOTORES[NUM_MOTORES] = {
  // nome        botão −      botão +     pinoA pinoB invertido
  { "Base",     "Esquerda",  "Direita",    32,   33,   false },
  { "Ombro",    "Desce",     "Sobe",       25,   26,   false },
  { "Cotovelo", "Recolhe",   "Estica",     27,   14,   false },
  { "Garra",    "Abre",      "Fecha",      18,   19,   false },
};

// ---------- Velocidade (PWM nas entradas da ponte H) ----------
#define VELOCIDADE_PADRAO  70    // % (ajustável pelo celular)
#define VELOCIDADE_MIN     30    // abaixo disso o motor costuma nem girar
#define PWM_FREQ         1000    // Hz
#define PWM_BITS            8    // 0–255
#define RAMPA_MS          150    // partida suave (evita tranco na hidráulica)

// ---------- Segurança ----------
// O celular manda o comando repetidamente enquanto o botão está apertado.
// Se parar de chegar por este tempo, o motor para (botão solto / sinal caiu).
#define TEMPO_SEM_COMANDO_MS  400
// Tempo máximo girando sem soltar o botão (protege o fim de curso da seringa).
#define TEMPO_MAX_MS         6000
