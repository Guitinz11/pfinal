# 10. Como funciona (para o relatório)

[← Problemas comuns](09-problemas.md) · [Índice](../README.md)

## Objetivo

Construir um braço robótico com garra, movido por um **sistema hidráulico de seringas com
água** e acionado por **motores elétricos**, controlado pelo **celular** pela rede Wi‑Fi do
próprio microcontrolador **ESP32**, sem internet e sem aplicativo de loja.

## Caminho do comando

```
Celular → Wi-Fi → ESP32 → Ponte H → Motor DC → Manivela → Seringa mestre → água → Seringa escrava → Junta
```

## As três áreas do projeto

**Hidráulica.** O **princípio de Pascal** diz que a pressão aplicada num líquido confinado se
transmite igualmente por todo ele. Como a água praticamente não comprime, empurrar o êmbolo
de uma seringa empurra o da outra na mesma medida. Com seringas de diâmetros diferentes, a
força se multiplica (a seringa maior faz mais força e anda menos).

**Eletrônica.** O ESP32 não consegue alimentar motores, então os motores recebem energia de
uma fonte externa através de **pontes H**. A ponte H é um conjunto de chaves eletrônicas que
inverte a polaridade do motor e, com isso, o sentido de giro. A velocidade é controlada por
**PWM**: o ESP32 liga e desliga a alimentação muito rápido (1000 vezes por segundo), e a
proporção do tempo ligado define a força média. O **GND comum** entre ESP32, pontes H e fonte
garante que todos os sinais tenham a mesma referência.

**Programação e rede.** O ESP32 funciona como **ponto de acesso Wi‑Fi** e como **servidor web**.
A página de controle fica gravada na memória dele. Enquanto um botão é segurado, o celular
envia um comando a cada 150 ms. Se os comandos param de chegar, o ESP32 desliga o motor. Esse
mecanismo, chamado *watchdog* ou *heartbeat*, é comum em robótica para evitar que uma máquina
continue se movendo quando perde a comunicação.

## Movimentos

| Par | Movimento | Mecanismo |
|---|---|---|
| 1 | Base (giro) | Alavanca: a seringa empurra um pino fora do centro do disco |
| 2 | Ombro | Seringa entre a coluna e o braço 1 |
| 3 | Cotovelo | Seringa empurrando uma alavanca presa no braço 2 |
| 4 | Garra | Seringa movendo uma peça deslizante ligada às pinças |

## Conceitos envolvidos

- **Física:** princípio de Pascal, pressão, força, alavancas, conversão de movimento circular em linear (manivela)
- **Eletrônica:** alimentação, corrente, GND comum, ponte H, PWM
- **Programação:** microcontrolador, servidor web, protocolo de comandos, segurança por timeout
- **Mecânica:** articulações, transmissão de força, estrutura

## Possíveis melhorias

- Chaves de fim de curso para o motor parar sozinho no fim da seringa
- Potenciômetro em cada junta para saber a posição e voltar a usar sliders
- Barra roscada no lugar da manivela, para ter mais força e precisão
- Gravar e repetir sequências de movimento
- Seringas de tamanhos diferentes para aumentar a força da garra
