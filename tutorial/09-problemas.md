# 9. Problemas comuns

[← Testes](08-testes.md) · [Índice](../README.md) · [Próximo: como funciona →](10-como-funciona.md)

## Arduino IDE

| Sintoma | Causa | Solução |
|---|---|---|
| `fatal error: config.h: No such file or directory` | Código de uma versão antiga, que tinha vários arquivos | Use o `garra_esp32.ino` atual, que é um arquivo só. Ele não tem `#include "config.h"` |
| Abas `config.h` / `pagina.h` não aparecem | Versão antiga aberta de dentro do ZIP | Não existem mais. A versão atual é um arquivo só |
| `Não há suporte para caminhos UNC` | Projeto numa pasta de rede (`\\servidor\...`) | Copie para o disco `C:` ou cole o código num Novo Sketch. Se compilar, o aviso pode ser ignorado |
| Erro pedindo `ESPAsyncWebServer`, `AsyncTCP` ou `ESP32Servo` | Código da primeira versão | O código atual não usa nenhuma biblioteca. Use o arquivo atual |
| `Failed uploading: no upload port provided` | Nenhuma porta escolhida | **Ferramentas → Porta** → a COM do ESP32 |
| Menu **Porta** cinza, não dá para clicar | O Windows não vê o ESP32 | Troque por um cabo de **dados**, troque a entrada USB e instale o driver ([capítulo 2](02-arduino-ide.md#23-driver-usb-se-a-porta-não-aparecer)) |
| Trava em `Connecting....___` | A placa não entrou em modo de gravação | Segure o botão **BOOT** até aparecer a porcentagem |

## Celular

| Sintoma | Solução |
|---|---|
| A rede `Garra-ESP` não aparece | Confira no Monitor Serial se o ESP32 iniciou. Aperte EN/RST |
| Conecta mas o celular sai da rede ("sem internet") | Escolha **manter conectado** e desligue os dados móveis |
| A página não abre | Digite `http://192.168.4.1` (com `http://`) |
| Indicador vermelho ("desconectado") | O celular saiu da rede Garra-ESP. Reconecte |

## Motores e hidráulica

| Sintoma | Solução |
|---|---|
| Nenhum motor gira | Jumpers ENA/ENB no lugar? GND do ESP32 na trilha −? Fonte ligada? |
| Motor gira ao contrário | Troque os 2 fios no borne OUT ou use `invertido = true` |
| Motor fraco | O L298N perde ~2 V: use fonte de 7–9 V e aumente a velocidade no app |
| A junta empurra mas não volta | O êmbolo está só encostado: prenda com parafuso. Confira se há ar ou mangueira frouxa |
| A manivela inverte o movimento no meio | Passou de meia volta. Solte o botão antes ou diminua `TEMPO_MAX_MS` |
| Aparece "solte o botão" no app | Atingiu o tempo máximo. Solte e aperte de novo |
