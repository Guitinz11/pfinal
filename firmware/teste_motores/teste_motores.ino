// ============================================================
//  TESTE DE MOTORES (sem Wi-Fi e sem app)
//
//  Serve para saber se o problema está na MONTAGEM ou no APP.
//  Gira os motores sozinho, em sequência, para sempre:
//
//    Motor 1 (OUT1/OUT2) para um lado  ... 2 s
//    parado                            ... 1 s
//    Motor 1 para o outro lado         ... 2 s
//    parado                            ... 1 s
//    Motor 2 (OUT3/OUT4) para um lado  ... 2 s
//    parado                            ... 1 s
//    Motor 2 para o outro lado         ... 2 s
//    parado                            ... 1 s
//
//  O LED azul da placa (GPIO 2) acende enquanto algum motor gira.
//
//  Ligação: a mesma do teste rápido
//    D32 -> IN1   D33 -> IN2   D25 -> IN3   D26 -> IN4
//    GND do ESP32 -> GND da ponte H (junto com o − da fonte)
//
//  Resultado:
//    - Motores giram  -> montagem OK. O problema é no app/código principal.
//    - LED pisca mas motores não giram -> problema na ligação ou na fonte.
//    - LED não pisca -> o ESP32 não está ligado ou o código não foi gravado.
// ============================================================

const int IN1 = 32;
const int IN2 = 33;
const int IN3 = 25;
const int IN4 = 26;
const int LED = 2;

void motor(int a, int b, int sentido, const char* texto) {
  digitalWrite(a, sentido > 0 ? HIGH : LOW);
  digitalWrite(b, sentido < 0 ? HIGH : LOW);
  digitalWrite(LED, sentido != 0 ? HIGH : LOW);
  Serial.println(texto);
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(LED, OUTPUT);
  motor(IN1, IN2, 0, "");
  motor(IN3, IN4, 0, "");
  Serial.println("\n=== Teste de motores ===");
  delay(1000);
}

void loop() {
  motor(IN1, IN2,  1, "Motor 1 -> sentido A  (IN1=1 IN2=0)"); delay(2000);
  motor(IN1, IN2,  0, "Motor 1 parado");                     delay(1000);
  motor(IN1, IN2, -1, "Motor 1 -> sentido B  (IN1=0 IN2=1)"); delay(2000);
  motor(IN1, IN2,  0, "Motor 1 parado");                     delay(1000);
  motor(IN3, IN4,  1, "Motor 2 -> sentido A  (IN3=1 IN4=0)"); delay(2000);
  motor(IN3, IN4,  0, "Motor 2 parado");                     delay(1000);
  motor(IN3, IN4, -1, "Motor 2 -> sentido B  (IN3=0 IN4=1)"); delay(2000);
  motor(IN3, IN4,  0, "Motor 2 parado");                     delay(1000);
}
