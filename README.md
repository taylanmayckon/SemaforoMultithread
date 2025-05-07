# 🚥 Semáforo Multithread - BitDogLab 🚥

Este projeto é a Atividade 3 da Fase 2 do EmbarcaTech. Consiste no desenvolvimento de um Semáforo com a utilização de recursos de multitarefa através do FreeRTOS. A proposta do projeto era gerar pelo menos uma Task para cada periférico utilizado da BitDogLab, trazendo sincronismo entre as Tasks utilizadas.

Desenvolvido por: Taylan Mayckon

---
## 📽️ Link do Video de Demonstração:
[Youtube](https://youtu.be/XJibyRbBtNc)

---

## 📌 **Funcionalidades Implementadas**

- FreeRTOS para geração de diferentes Tasks: Foram geradas seis tasks para o desenvolvimento do projeto
- Modo Noturno/Normal: Foi aplicada na task vReadButtonTask uma rotina que constantemente verifica bordas de descida no Botão A da BitDogLab, no caso as leituras de pressionamento de otão, tendo um debounce de 200ms aplicado no código para excluir leituras erradas
- Luz do semáforo: No LED RGB, tem-se a indicação do modo atual do semáforo, sendo composto pelas luzes verde (livre), amarela (atenção e vermelha (pare). O tempo de cada luz do semáforo é, respectivamente: 15s, 5s e 15s. No modo noturno, a temporização não é exibida, permanecendo sempre no modo de alerta.
- Alerta sonoro para deficientes auditivos: Utilizou-se de buzzers para gerar alertas sonoros para os deficientes auditivos. Quando o semáforo está no modo noturno, tem-se um beep de 200ms com buzzer ativo e 3800ms com ele desativado. Para a indicação de cada estado do modo normal, tem-se na luz verde um beep contínuo de 1s, seguido de 14s desativado. Na luz amarela um beep intermitente de 250ms ativo e 250ms desligado. Na cor vermelha, tem-se 500ms ativado e 1500ms desativado.
- Mensagens informativas no Display OLED: No display OLED é possível ver o modo atual do semáforo, a luz referente à esse modo, uma mensagem indicativa para o modo atual, e o tempo restante até que o modo seja alterado.
- Animações interativas na Matriz de LEDs: No modo da cor verde do semáforo, tem-se uma animação de seta verde, que cruza a matriz de LEDs, indicando que está livre para passagem. Na cor amarela (noturno/normal) tem-se uma exclamação em amarelo que faz animação de pulsar. No modo vermelho, tem-se uma animação que se assemelha com uma placa de STOP, pulsando rapidamente na matriz.

---

## 🛠 **Recursos Utilizados**

- **FreeRTOS:** é um sistema operacional de código aberto e tempo real (RTOS) projetado para microcontroladores e dispositivos embarcados. Ele permite a criação de diferentes tarefas e faz o gerenciamento das mesmas para serem executadas de forma paralela.
- **Display OLED:** foi utilizado o ssd1306, que possui 128x64 pixels, para informações visuais sobre o estado atual do semáforo e também o tempo restante de cada etapa do semáforo.
- **Matriz de LEDs Endereçáveis:** A BitDogLab possui uma matriz de 25 LEDs WS2812, que foi operada com o auxílio de uma máquina de estados.
- **Leitura de botões:** Utilizou-se do Botão A para uma interface que permita a alternância entre os modos do semáforo.
- **LED RGB:** Utilizado para indicar o estado atual do semáforo, exibindo a cor do mesmo.
- **Buzzers:** Emite sons para gerar alertas sonoros para deficientes auditivos.

---

## 📸 **Imagens do Funcionamento**

🚧🚧🚧🚧🚧 Esse tópico está em construção!

---

## 📂 **Estrutura do Código**

```
📂 SemaforoMultithread/
├── 📄 SemaforoMultithread.c           # Código principal do projeto
├──── 📂lib
├───── 📄 FreeRTOSConfig.h             # Arquivos de configuração para o FreeRTOS
├───── 📄 font.h                       # Fonte utilizada no Display I2C
├───── 📄 led_matrix.c                 # Funções para manipulação da matriz de LEDs endereçáveis
├───── 📄 led_matrix.h                 # Cabeçalho para o led_matrix.c
├───── 📄 ssd1306.c                    # Funções que controlam o Display I2C
├───── 📄 ssd1306.h                    # Cabeçalho para o ssd1306.c
├───── 📄 structs.h                    # Structs utilizadas no código principal
├───── 📄 ws2812.pio                   # Máquina de estados para operar a matriz de LEDs endereçáveis
├── 📄 CMakeLists.txt                  # Configurações para compilar o código corretamente
└── 📄 README.md                       # Documentação do projeto
```

---
