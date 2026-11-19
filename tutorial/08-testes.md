# 8. Testes e calibração (por etapas)

[← App do celular](07-app-celular.md) · [Índice](../README.md) · [Próximo: problemas comuns →](09-problemas.md)

> Só tem 1 ponte H e 2 motores? Use o [teste rápido](00-teste-rapido.md) para validar o código primeiro.

Monte e teste **uma etapa por vez**. Só avance quando a atual funcionar. Assim, quando algo
der errado, você sabe exatamente onde está o problema.

| Etapa | O que montar | Passou se… |
|---|---|---|
| **1. ESP32 sozinho** | Só o ESP32 no USB | O Monitor Serial mostra a rede, o celular abre `192.168.4.1` e o indicador fica verde |
| **2. Um motor** | ESP32 + Ponte H 1 + motor da Base + fonte | Segurar "Direita" gira para um lado, "Esquerda" para o outro, e soltar para |
| **3. Segurança** | Igual à etapa 2 | Com o botão apertado, desligar o Wi‑Fi do celular para o motor em menos de 1 s |
| **4. Os 4 motores** | As duas pontes e os 4 motores, sem seringas | Todos giram. Segurando 2 ao mesmo tempo, nada reinicia |
| **5. Sentido** | Igual à etapa 4 | Cada botão gira no sentido certo. Se não, troque os 2 fios no borne OUT ou use `invertido = true` |
| **6. Um par de seringas** | Um motor com manivela + um par de seringas na bancada | A escrava acompanha a mestre, empurrando e puxando, sem "atraso esponjoso" |
| **7. Curso** | Igual à etapa 6 | Você sabe por quanto tempo segurar antes do êmbolo chegar no fim. Ajuste `TEMPO_MAX_MS` se precisar |
| **8. Uma junta** | Comece pela **Garra** | Abre e fecha segurando um objeto leve, sem forçar o motor |
| **9. Braço completo** | Uma junta por vez; a **Base por último** | Pega um objeto, levanta, gira e solta |
| **10. Resistência** | Tudo | 10–15 min de uso sem vazamento, bolha ou motor esquentando. Teste com o celular a 5–10 m |

## Sinais de problema

- **Motor zumbe e não gira:** velocidade baixa demais, fonte fraca ou seringa no fim do curso.
- **ESP32 reinicia quando o motor liga:** falta GND comum ou a fonte não aguenta. Confira a trilha −.
- **Junta mole ou volta sozinha:** ar na hidráulica.
- **Motor ou L298N muito quente:** está forçando contra o fim da seringa. Solte o botão mais cedo.

> Anote os resultados de cada etapa: ajuda a refazer a montagem e serve para o relatório.
