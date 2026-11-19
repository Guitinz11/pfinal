# Garra hidráulica controlada pelo celular (ESP32)

Garra movida por **8 seringas com água** (4 pares mestre → escrava) e **4 servos**,
controlada pelo celular através da rede Wi-Fi criada pelo próprio ESP32.
Usa só o que já vem com o suporte ao ESP32: **nenhuma biblioteca extra**.
Não precisa de app nem de internet: qualquer navegador funciona.

```
[Celular] --Wi-Fi "Garra-ESP"--> [ESP32: página web]
                                        │ PWM
                                  [4 servos]  ← fonte externa 5–6 V
                                        │ empurram
                              seringa MESTRE ──mangueira c/ água──► seringa ESCRAVA
                                                                    (move a junta)
```

## Juntas (1 servo + 1 par de seringas cada)

| # | Junta    | GPIO | O que faz            |
|---|----------|------|----------------------|
| 0 | Base     | 13   | gira o braço         |
| 1 | Ombro    | 14   | sobe/desce o braço   |
| 2 | Cotovelo | 27   | estica/recolhe       |
| 3 | Garra    | 26   | abre/fecha           |

Nomes, pinos e limites ficam em [`firmware/garra_esp32/config.h`](firmware/garra_esp32/config.h).

### Base giratória (junta 0)

A seringa empurra em linha reta, então é preciso converter esse movimento em giro:

```
        vista de cima
     ┌───────────────┐
     │   ( base  )   │   seringa ESCRAVA presa na parte FIXA,
     │   (   •   )───┼── ponta do êmbolo presa num pino
     │   ( gira  )   │   fora do centro do disco (alavanca)
     └───────────────┘
```

- **Alavanca (mais simples):** o êmbolo empurra um pino a uns 3–5 cm do centro.
  Dá cerca de **90–120° de giro**. Use um pino/parafuso solto (articulado) nas duas
  pontas da seringa, porque o ângulo muda durante o giro.
- **Cremalheira/polia (mais giro):** o êmbolo puxa um fio enrolado numa polia
  (ou move uma cremalheira numa engrenagem) e você ganha **até ~180°**, mas a força é menor.
- Apoie o disco num **rolamento de mesa giratória ("lazy susan")** ou num rolamento
  grande, para o peso do braço não travar o giro.

## Ligação elétrica

```
 FONTE 5–6 V (≥ 3 A)                     ESP32
   (+) ──┬──────────── VCC (fio vermelho) dos 4 servos
         │
        [C] 1000 µF (perna + no +)
         │
   (−) ──┴──────────── GND (fio marrom) dos 4 servos ──── GND do ESP32   ← OBRIGATÓRIO

 Sinal (fio laranja/amarelo):
   Servo Base     → GPIO 13
   Servo Ombro    → GPIO 14
   Servo Cotovelo → GPIO 27
   Servo Garra    → GPIO 26

 ESP32 alimentado pelo USB (ou pelo pino 5V/VIN a partir de um regulador).
```

- **Nunca** alimente os servos pelo 3V3/5V do ESP32 — ele reinicia ou queima.
- O **GND tem que ser comum** entre fonte e ESP32, senão o sinal não funciona.
- 4 × MG996R podem puxar mais de 2 A juntos quando forçam: use fonte de 3 A ou mais.

## Montagem hidráulica

1. Encha mangueira e seringas **debaixo d'água** para não sobrar bolha de ar
   (ar = garra "mole" e imprecisa).
2. Deixe a seringa mestre com o êmbolo **no meio do curso** e a escrava também,
   assim a junta consegue ir para os dois lados a partir da posição inicial (90°).
3. Prenda bem as seringas mestre (abraçadeira/suporte) — se elas se moverem,
   o servo empurra a seringa inteira em vez do êmbolo.
4. Mestre **menor** que a escrava = mais força, menos curso. Iguais = 1:1.
5. Deixe a eletrônica acima e afastada da água.

## Gravando o firmware

**Arduino IDE** (não precisa instalar nenhuma biblioteca)
1. Em Preferências, adicione a URL de placas
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Gerenciador de Placas → instale **esp32** (Espressif Systems).
3. Abra `firmware/garra_esp32/garra_esp32.ino`, placa **ESP32 Dev Module**, e envie.

**PlatformIO**
```bash
pio run -t upload && pio device monitor
```

## Usando

1. No celular, conecte na rede **Garra-ESP** (senha `garra1234`).
2. Abra **http://192.168.4.1**.
3. Use os sliders (ou −/+ para ajuste fino) para mover cada junta.
   - **PARAR**: congela tudo na posição atual.
   - **Home**: volta todas as juntas para a posição inicial.
   - **Velocidade**: mais devagar = menos tranco na hidráulica.

Vários celulares podem se conectar ao mesmo tempo; todos ficam sincronizados.

## Calibração (faça antes de usar pra valer!)

Com a hidráulica montada, para cada junta:
1. Mova devagar o slider até o êmbolo **quase** chegar no fim da seringa
   (escrava ou mestre, o que acontecer primeiro). Anote o ângulo.
2. Faça o mesmo para o outro lado.
3. Coloque esses valores (com ~5° de folga) em `min` e `max` no `config.h`.
4. Se a junta se mexe ao contrário do slider, mude `invertido` para `true`.

Assim o servo nunca força contra o fim de curso (o que esquenta o motor e pode
estourar a mangueira).

## Segurança embutida

- Limites mínimo/máximo por junta (o ESP32 ignora valores fora da faixa).
- Movimento em velocidade constante (sem trancos).
- Se o celular desconectar ou parar de responder por 3 s **durante** um movimento,
  a garra congela onde está.

## Protocolo (se quiser fazer outro controle/app)

Requisição HTTP `GET http://192.168.4.1/cmd?c=<comando>`:

| Mensagem      | Efeito                                   |
|---------------|------------------------------------------|
| `s,<j>,<g>`   | junta `j` vai para `g` graus             |
| `v,<g/s>`     | velocidade em graus por segundo          |
| `x`           | PARAR                                    |
| `h`           | HOME                                     |
| `p`           | ping (enviar a cada ~1 s)                |

Toda resposta é um JSON com o estado:
`{"v":60,"p":false,"j":[{"n":"Base","min":10,"max":170,"a":90}, ...]}`

## Trocando servo por outro motor

Se os servos não tiverem força para as seringas, a alternativa é **motor de passo
(28BYJ-48 + ULN2003 ou NEMA17 + A4988) empurrando o êmbolo com barra roscada**.
Só a função `escreverServo()` e o `setup()` do `.ino` precisam mudar; a página e o
protocolo continuam iguais.
