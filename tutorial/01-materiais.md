# 1. Materiais

[← Índice](../README.md) · [Próximo: preparar a Arduino IDE →](02-arduino-ide.md)

## Eletrônica

| Item | Qtd. | Observação |
|---|---|---|
| ESP32 DevKit (38 ou 30 pinos) | 1 | Tem Wi‑Fi embutido. O desenho usa a de 38 pinos |
| Ponte H **L298N** | 2 | Cada uma controla 2 motores |
| Motor DC amarelo **TT** com caixa de redução (3–6 V) | 4 | De preferência já com os fios soldados |
| Fonte **7–9 V, 2 A ou mais** | 1 | O L298N perde ~2 V, então chegam 5–7 V nos motores |
| Protoboard | 1 | Só as trilhas laterais (+ e −) são usadas |
| Jumper **fêmea‑fêmea** | 8 | Pinos do ESP32 → pinos IN das pontes H |
| Jumper **macho‑macho** | 4 | Bornes de alimentação das pontes → protoboard |
| Jumper **macho‑fêmea** | 1 | GND do ESP32 → protoboard |
| Cabo USB **de dados** | 1 | Muitos cabos só carregam. Use um que transfere arquivos |

> Não precisa de solda nem de emenda. Sinais são encaixados com jumper, e motores e fonte vão
> em bornes de parafuso. O único ponto que precisaria de solda é se o motor vier sem fio.

## Hidráulica

| Item | Qtd. | Observação |
|---|---|---|
| Seringa de 10 ou 20 ml | 8 | 4 pares: mestre (no motor) + escrava (na junta) |
| Mangueira de aquário ou de soro | ~4 m | Precisa encaixar **justa** no bico da seringa |
| Água | — | Pode colocar um pouco de corante para ver bolhas |

## Estrutura

| Item | Qtd. | Observação |
|---|---|---|
| MDF 3–6 mm, madeira ou palito de picolé | — | Base, coluna, braços, pinças e manivelas |
| Rolamento de prato giratório ("lazy susan") | 1 | Para o disco da base girar leve |
| Parafuso M3 com porca | ~24 | Pivôs das juntas, das seringas e das manivelas |
| Abraçadeira de nylon | ~16 | Prender as seringas |
| Cola quente | — | Fixar suportes |

## Ferramentas

- Computador com a Arduino IDE
- Chave de fenda pequena (bornes do L298N)
- Alicate ou estilete (descascar o fio da fonte)
- Furadeira ou parafusadeira com broca de 3 mm
- Multímetro (para conferir qual fio da fonte é o +)
