// ============================================================
//  Garra hidráulica (seringas + água) com MOTORES DC + PONTE H
//  controlada pelo celular.
//
//  NÃO PRECISA DE NENHUMA BIBLIOTECA EXTRA: usa só o que já vem
//  com o suporte ao ESP32 na Arduino IDE (WiFi, WebServer e LEDC).
//  Tudo está neste arquivo único: a configuração fica logo abaixo.
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

// ============================================================
//  CONFIGURAÇÃO  (edite aqui: pinos, nomes, rede, velocidade)
// ============================================================

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
//  Ponte H 2:  IN1 = 23  IN2 = 22  (Cotovelo)|  IN3 = 19  IN4 = 18  (Garra)
//  (Ponte H 1 usa pinos do lado esquerdo da placa, a Ponte H 2 do lado direito.)
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
  { "Cotovelo", "Recolhe",   "Estica",     23,   22,   false },
  { "Garra",    "Abre",      "Fecha",      19,   18,   false },
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

// ============================================================
//  PÁGINA DO CELULAR
// ============================================================
// Página web servida pelo ESP32 (abre em qualquer navegador de celular).
// Motores DC: SEGURE o botão para mover, SOLTE para parar.

const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>Garra Hidráulica</title>
<style>
  :root { --bg:#10141a; --card:#1b222c; --txt:#e8edf3; --sub:#8a96a6; --acc:#2f9bff; --red:#e5484d; --ok:#30a46c; --warn:#f5a524; }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; -webkit-user-select:none; user-select:none; -webkit-touch-callout:none; }
  body { margin:0; font-family:system-ui,sans-serif; background:var(--bg); color:var(--txt); padding:16px; }
  header { display:flex; justify-content:space-between; align-items:center; margin-bottom:14px; }
  h1 { font-size:20px; margin:0; }
  #st { font-size:13px; padding:4px 10px; border-radius:99px; background:var(--red); }
  #st.on { background:var(--ok); }
  .card { background:var(--card); border-radius:14px; padding:14px; margin-bottom:12px; }
  .top { display:flex; justify-content:space-between; align-items:center; font-size:16px; margin-bottom:10px; }
  .tag { font-size:12px; color:var(--sub); }
  .tag.mov { color:var(--acc); } .tag.lim { color:var(--warn); }
  .row { display:flex; gap:10px; }
  .hold { flex:1; height:64px; border:0; border-radius:12px; background:#2a3442; color:var(--txt); font-size:17px; font-weight:600; touch-action:none; }
  .hold.on { background:var(--acc); color:#fff; }
  #stop { width:100%; height:60px; border:0; border-radius:14px; background:var(--red); color:#fff; font-size:20px; font-weight:800; margin-bottom:12px; }
  input[type=range] { width:100%; height:36px; accent-color:var(--acc); }
  small { color:var(--sub); }
</style>
</head>
<body>
<header><h1>🦾 Garra Hidráulica</h1><span id="st">desconectado</span></header>
<button id="stop">PARAR TUDO</button>
<div id="motores"></div>
<div class="card">
  <div class="top"><span>Velocidade</span><b id="vv">--</b></div>
  <input type="range" id="vel" min="30" max="100" step="5">
  <small>Segure o botão para mover e solte para parar.</small>
</div>

<script>
const $ = id => document.getElementById(id);
let estado = null, segurando = {}, ocupado = false, fila = null;

function online(ok) { $('st').textContent = ok ? 'conectado' : 'desconectado'; $('st').classList.toggle('on', ok); }

// Uma requisição por vez; o comando mais novo substitui o que estava esperando
function enviar(m) {
  if (ocupado) { fila = fila ? fila + ';' + m : m; return; }
  ocupado = true;
  fetch('/cmd?c=' + encodeURIComponent(m), { cache: 'no-store' })
    .then(r => r.json()).then(s => { online(true); atualizar(s); })
    .catch(() => online(false))
    .finally(() => { ocupado = false; if (fila) { const f = fila; fila = null; enviar(f); } });
}

function montar(j) {
  $('motores').innerHTML = j.map((x, i) => `
    <div class="card">
      <div class="top"><span>${x.n}</span><span class="tag" id="t${i}">parado</span></div>
      <div class="row">
        <button class="hold" data-i="${i}" data-d="-1">◀ ${x.m}</button>
        <button class="hold" data-i="${i}" data-d="1">${x.p} ▶</button>
      </div>
    </div>`).join('');
  document.querySelectorAll('.hold').forEach(b => {
    const i = +b.dataset.i, d = +b.dataset.d;
    const ini = e => { e.preventDefault(); b.setPointerCapture && b.setPointerCapture(e.pointerId);
                       segurando[i] = d; b.classList.add('on'); enviar(`m,${i},${d}`); };
    const fim = () => { if (segurando[i] !== d) return; delete segurando[i]; b.classList.remove('on'); enviar(`m,${i},0`); };
    b.addEventListener('pointerdown', ini);
    ['pointerup', 'pointercancel', 'lostpointercapture'].forEach(ev => b.addEventListener(ev, fim));
    b.addEventListener('contextmenu', e => e.preventDefault());
  });
}

function atualizar(s) {
  if (!estado || estado.j.length !== s.j.length) montar(s.j);
  estado = s;
  s.j.forEach((x, i) => {
    const t = $('t' + i);
    if (x.t) { t.textContent = 'tempo máximo: solte o botão'; t.className = 'tag lim'; }
    else if (x.d) { t.textContent = 'movendo: ' + (x.d > 0 ? x.p : x.m); t.className = 'tag mov'; }
    else { t.textContent = 'parado'; t.className = 'tag'; }
  });
  if (document.activeElement !== $('vel')) $('vel').value = s.v;
  $('vv').textContent = s.v + '%';
}

$('stop').onclick = () => {
  Object.keys(segurando).forEach(k => delete segurando[k]);
  document.querySelectorAll('.hold.on').forEach(b => b.classList.remove('on'));
  fila = null; enviar('x');
};
$('vel').oninput = () => { $('vv').textContent = $('vel').value + '%'; enviar('v,' + $('vel').value); };

// Enquanto um botão está apertado, repete o comando (se parar de chegar, o motor para)
setInterval(() => {
  const ks = Object.keys(segurando);
  enviar(ks.length ? ks.map(i => `m,${i},${segurando[i]}`).join(';') : 'p');
}, 150);
enviar('p');
document.addEventListener('visibilitychange', () => { if (document.hidden) $('stop').onclick(); });
</script>
</body>
</html>
)rawliteral";

// ============================================================
//  PROGRAMA
// ============================================================

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
