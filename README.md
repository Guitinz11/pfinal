# Garra hidráulica controlada pelo celular (ESP32 + motores DC)

Garra movida por **8 seringas com água** (4 pares mestre → escrava) e **4 motores DC**
com **2 pontes H**, controlada pelo celular através da rede Wi-Fi criada pelo próprio ESP32.
Não precisa de app nem de internet, e o código **não usa nenhuma biblioteca extra**.

```
[Celular] --Wi-Fi "Garra-ESP"--> [ESP32: página web]
                                        │ 8 fios de sinal (IN1..IN4 x2)
                              [2 pontes H]  ← fonte externa
                                        │
                                  [4 motores DC]
                                        │ empurram/puxam
                              seringa MESTRE ──mangueira c/ água──► seringa ESCRAVA
```

## Por que ponte H?

O motor DC só muda de sentido quando o + e o − dele são invertidos. A ponte H faz essa
inversão sob comando do ESP32. Cada módulo comum (L298N, L9110S, TB6612) controla **2 motores**,
então são necessários **2 módulos**.

## Motores e pinos

| Motor    | Ponte H | Entradas  | Pinos ESP32 | Botões no celular  |
|----------|---------|-----------|-------------|--------------------|
| Base     | 1       | IN1 / IN2 | 32 / 33     | Esquerda / Direita |
| Ombro    | 1       | IN3 / IN4 | 25 / 26     | Desce / Sobe       |
| Cotovelo | 2       | IN1 / IN2 | 27 / 14     | Recolhe / Estica   |
| Garra    | 2       | IN3 / IN4 | 18 / 19     | Abre / Fecha       |

Motor 1 de cada ponte nos bornes **OUT1/OUT2**, motor 2 nos bornes **OUT3/OUT4**.
Tudo configurável em [`firmware/garra_esp32/config.h`](firmware/garra_esp32/config.h).

## Ligação

```
 ESP32                  PONTE H 1                 PONTE H 2
 32 ─────────────────── IN1                       
 33 ─────────────────── IN2      OUT1/OUT2 → motor Base
 25 ─────────────────── IN3      OUT3/OUT4 → motor Ombro
 26 ─────────────────── IN4
 27 ───────────────────────────────────────────── IN1
 14 ───────────────────────────────────────────── IN2   OUT1/OUT2 → motor Cotovelo
 18 ───────────────────────────────────────────── IN3   OUT3/OUT4 → motor Garra
 19 ───────────────────────────────────────────── IN4
 GND ────────────────── GND ───────────────────── GND   ← OBRIGATÓRIO (GND comum)

 FONTE (+) ──────────── +12V/VCC ──────────────── +12V/VCC
 FONTE (−) ──────────── GND ───────────────────── GND
 ESP32 alimentado pelo USB.
```

- Os pinos IN são machos: use **jumpers fêmea-fêmea** do ESP32 até a ponte H.
- Os bornes (+12V, GND, OUT) são de parafuso: só encaixar o fio e apertar. Para levar a fonte
  às duas pontes, use a trilha da protoboard ou coloque 2 fios no mesmo borne.
- **L298N:** deixe os jumpers **ENA** e **ENB** no lugar. Ele "perde" uns 2 V, então para motor
  amarelo (TT) de 3–6 V use fonte de **7–9 V**. Não ligue nada no pino "5V" dele.
- **L9110S / TB6612:** quase não perdem tensão, use fonte de **5–6 V**.
- Se um motor girar ao contrário do botão, inverta os 2 fios dele no borne OUT ou mude
  `invertido` para `true` no `config.h`.

## Parte mecânica: motor DC empurrando a seringa

O motor precisa ter **caixa de redução** (o motor amarelo de carrinho já tem). Para transformar
o giro em empurra/puxa:

- **Manivela (mais simples):** um braço preso no eixo do motor e uma haste até o êmbolo.
  Girar para um lado empurra, girar para o outro puxa. Use menos de meia volta.
- **Barra roscada (mais força):** o motor gira uma barra roscada e uma porca presa ao êmbolo
  anda para frente e para trás. É lenta, mas segura a posição sozinha quando o motor para.

Em todos os casos o **êmbolo precisa estar preso** (furo + parafuso ou arame), senão ele só
empurra e a junta não volta.

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
3. **Segure** um botão para mover o motor e **solte** para parar. Dá para segurar dois ao mesmo tempo.
   - **PARAR TUDO**: para todos os motores na hora.
   - **Velocidade**: força dos motores, em %.

## Segurança embutida

- O motor só gira enquanto o botão está apertado. Se o celular parar de mandar o comando
  (botão solto, Wi-Fi caiu, tela bloqueada), o motor para em 0,4 s.
- Se o botão ficar apertado por mais de 6 s seguidos, o motor para e só volta depois de soltar.
  Isso protege o fim de curso da seringa. Ajuste `TEMPO_MAX_MS` no `config.h`.
- Partida suave (rampa), para não dar tranco na hidráulica.

Como o motor DC não sabe a própria posição, **é você quem para antes do fim da seringa**.
Para ficar automático, dá para adicionar chaves de fim de curso (microswitch) depois.

## Protocolo (se quiser fazer outro controle)

`GET http://192.168.4.1/cmd?c=<comandos>` (vários separados por `;`):

| Comando       | Efeito                                         |
|---------------|------------------------------------------------|
| `m,<j>,1`     | motor `j` no sentido do botão da direita       |
| `m,<j>,-1`    | motor `j` no sentido do botão da esquerda      |
| `m,<j>,0`     | para o motor `j`                               |
| `v,<30-100>`  | velocidade em %                                |
| `x`           | PARAR tudo                                     |
| `p`           | ping                                           |

Enquanto o motor deve girar, o comando `m` precisa ser repetido a cada ~150 ms.
