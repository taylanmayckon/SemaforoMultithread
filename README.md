# 🚥 Semáforo Multithread - BitDogLab 🚥

Este projeto é a Atividade 3 da Fase 2 do EmbarcaTech. Consiste no desenvolvimento de um Semáforo com a utilização de recursos de multitarefa através do FreeRTOS. A proposta do projeto era gerar pelo menos uma Task para cada periférico utilizado da BitDogLab, trazendo sincronismo entre as Tasks utilizadas.

---

## 📌 **Funcionalidades Implementadas**

✅ FreeRTOS para geração de diferentes Tasks\

---

## 🛠 **Recursos Utilizados**

- **FreeRTOS:** é um sistema operacional de código aberto e tempo real (RTOS) projetado para microcontroladores e dispositivos embarcados. Ele permite a criação de diferentes tarefas e faz o gerenciamento das mesmas para serem executadas de forma paralela.
- **Display OLED:** foi utilizado o ssd1306, que possui 128x64 pixels, para informações visuais sobre o estado atual do semáforo e também o tempo restante de cada etapa do semáforo.
- **Matriz de LEDs Endereçáveis:**
- **Leitura de botões:** Foi utilizado pooling em uma das Tasks do código
- **LED RGB:**
- **Buzzers:**

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

## 📽️ **Vídeo no YouTube**
