#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "led_matrix.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// LED RGB, botão e buzzers
#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12
#define BUTTON_A 5
#define BUZZER_A 21 
#define BUZZER_B 10
// Constantes para a matriz de leds
#define IS_RGBW false
#define LED_MATRIX_PIN 7
// Definições da I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Booleano para indicar se vai imprimir branco no display
bool cor = true;
// Variáveis da PIO declaradas no escopo global
PIO pio;
uint sm;
// Variável para indicar qual luz do semáforo está ativa
uint semaforo_state = 0; // 0: Verde | 1: Amarelo | 2: Vermelho
// Variáveis do PWM (setado para freq. de 312,5 Hz)
uint wrap = 2000;
uint clkdiv = 25;
// Variáveis para debounce do botão 
uint32_t last_time = 0; // Armazena o ultimo tempo do botao
bool last_button_state = false; // Armazena o ultimo estado do botao
// Variável que controla o modo noturno
bool night_mode = false;
// Tempos de cada cor no semáforo (em ms)
const uint green_time = 15000;
const uint yellow_time = 5000;
const uint red_time = 10000;
// Booleano para a ativação do buzzer verde
bool buzzer_green = true;
// Index do frame que será exibido na matriz de leds
uint green_frame_index = 0;
// Intensidade da cor na matriz de leds (funciona apenas para vermelho e amarelo)
int matrix_intensity_step = 10;
bool matrix_intensity_rising = false;
// String para armazenar o tempo restante do semáforo
char converted_num; // Armazena um dígito
char converted_string[3]; // Armazena o número convertido (2 dígitos)
// Contadores para o display
uint green_count = green_time/1000;
uint yellow_count = yellow_time/1000;
uint red_count = red_time/1000;


// FUNÇÕES AUXILIARES =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Função para configurar o PWM e iniciar com 0% de DC
void set_pwm(uint gpio, uint wrap){
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true); 
    pwm_set_gpio_level(gpio, 0);
}

// Função que converte int para char
void int_2_char(int num, char *out){
    *out = '0' + num;
}

void int_2_string(int num){
    if(num<9){ // Gera string para as menores que 10
        int_2_char(num, &converted_num); // Converte o dígito à direita do número para char
        converted_string[0] = '0'; // Char para melhorar o visual
        converted_string[1] = converted_num; // Int convertido para char
        converted_string[2] = '\0'; // Terminador nulo da String 
    }
    else{ // Gera a string para as maiores/iguais que 10
        int divider = num/10; // Obtém as dezenas
        int_2_char(divider, &converted_num);
        converted_string[0] = converted_num;

        int_2_char(num%10, &converted_num); // Obtém a parte das unidades
        converted_string[1] = converted_num; // Int convertido para char
        converted_string[2] = '\0'; // Terminador nulo da String
    }
}


// TASKS UTILIZADAS NO CÓDIGO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Task para controlar a temporização do semáforo
void vTimerSemaforoTask(){
    while(true){
        red_count = red_time/1000; // Reseta o contador vermelho do display
        green_count = green_time/1000; // Reseta o contador verde do display
        yellow_count = yellow_time/1000; // Reseta o contador amarelo do display
            
        semaforo_state = 0;
        vTaskDelay(pdMS_TO_TICKS(green_time));

        semaforo_state = 1;
        vTaskDelay(pdMS_TO_TICKS(yellow_time));

        semaforo_state = 2;
        vTaskDelay(pdMS_TO_TICKS(red_time));

        buzzer_green = true; // Engatilha o buzzer da luz verde novamente
    }
}

// Task para leitura do botão A
void vReadButtonTask(){
    // Iniciando o pino do Botão A para as leituras
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    while(true){
        uint32_t current_time = to_us_since_boot(get_absolute_time()); // Pega o tempo atual (em us)
        bool current_button_state = gpio_get(BUTTON_A); // Pega o estado atual do botao

        if(!current_button_state && last_button_state && (current_time - last_time > 200000)){ // Pegando a borda de descida com debounce de 200ms
            last_time = current_time; // Atualiza o ultimo tempo
            night_mode = !night_mode; // Alterna o flag do modo noturno

            // Logs para indicar o modo que está agora
            if(night_mode){
                printf("(MODE) NIGHT\n");
            }
            else{
                printf("(MODE) NORMAL\n");
                semaforo_state = 0; // Na volta para o modo normal retorna para a cor verde
                green_frame_index=0; // Retorna para o frame 0 da animação da luz verde
                matrix_intensity_step=10; // Retorna para 10% de intensidade na matriz de leds (cores vermelho e amarelo)
                matrix_intensity_rising=false; // Indica que a intensidade tem que descer
                buzzer_green = true; // Engatilha o buzzer da luz verde
            }
        }

        last_button_state = current_button_state; // Atualiza o ultimo estado do botão A
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay de 100ms para reduzir o uso de CPU
    }
}

// Task para controlar o LED RGB do semáforo
void vLedsRGBSemaforoTask(){
    // Ativando o PWM do LED RGB com 0% de DC
    set_pwm(LED_RED, wrap);
    set_pwm(LED_GREEN, wrap);
    set_pwm(LED_BLUE, wrap);

    float led_luminosity = 0.05; // Intensidade dos LEDs

    while(true){
        // Ações do modo noturno do semáforo
        if(night_mode){
            semaforo_state = 0; // Atualiza continuamente para o clico de cor verde do semáforo
            // Alterna 2s on/2s of
            // Amarelo = 0.5*verde + 0.5*vermelho
            pwm_set_gpio_level(LED_GREEN, led_luminosity*wrap);
            pwm_set_gpio_level(LED_RED, led_luminosity*wrap);
            vTaskDelay(pdMS_TO_TICKS(2000));
            pwm_set_gpio_level(LED_GREEN, 0);
            pwm_set_gpio_level(LED_RED, 0);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        // Modo normal do semáforo
        else{
            switch(semaforo_state){
                // Cor VERDE
                case 0:
                    pwm_set_gpio_level(LED_RED, 0);
                    pwm_set_gpio_level(LED_GREEN, led_luminosity*wrap);
                    pwm_set_gpio_level(LED_BLUE, 0);
                    break;
                
                // Cor AMARELA
                case 1:
                    pwm_set_gpio_level(LED_RED, led_luminosity*wrap);
                    pwm_set_gpio_level(LED_GREEN, led_luminosity*wrap);
                    pwm_set_gpio_level(LED_BLUE, 0);
                    break;
    
                // Cor VERMELHA
                case 2:
                    pwm_set_gpio_level(LED_RED, led_luminosity*wrap);
                    pwm_set_gpio_level(LED_GREEN, 0);
                    pwm_set_gpio_level(LED_BLUE, 0);
                    break;
            }
        }

        vTaskDelay(10);
    }
}


// Task para controlar o buzzer
void vBuzzerTask(){
    // Iniciando o PWM dos buzzers
    set_pwm(BUZZER_A, wrap);
    set_pwm(BUZZER_B, wrap);

    // Booleano que indica quando deve ter o buzzer do sinal verde
    buzzer_green = true;

    while(true){

        // Modo noturno
        if(night_mode){
            // Aciona os buzzers durante 200ms
            pwm_set_gpio_level(BUZZER_A, wrap*0.05);
            pwm_set_gpio_level(BUZZER_B, wrap*0.05);
            vTaskDelay(pdMS_TO_TICKS(200));
            // Desativa ambos e espera 3600ms
            pwm_set_gpio_level(BUZZER_A, 0);
            pwm_set_gpio_level(BUZZER_B, 0);
            vTaskDelay(pdMS_TO_TICKS(3800));
            bool buzzer_green = true ; // Deixa o buzzer pronto para ser acionado na cor verde
        }
        // Modo normal
        else{
            switch(semaforo_state){
                // Cor verde
                case 0:
                    // Alterna 1s on/restante do tempo off, graças ao booleano
                    if(buzzer_green){
                        pwm_set_gpio_level(BUZZER_A, wrap*0.05);
                        pwm_set_gpio_level(BUZZER_B, wrap*0.05);
                        buzzer_green=false; // Desaciona o buzzer para os segundos seguintes da luz verde
                    }
                    else{
                        pwm_set_gpio_level(BUZZER_A, 0);
                        pwm_set_gpio_level(BUZZER_B, 0);
                    }
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    break;

                // Cor amarela
                case 1:
                    // Alterna 0,25s on/0,25s of
                    pwm_set_gpio_level(BUZZER_A, wrap*0.05);
                    pwm_set_gpio_level(BUZZER_B, wrap*0.05);
                    vTaskDelay(pdMS_TO_TICKS(250));
                    pwm_set_gpio_level(BUZZER_A, 0);
                    pwm_set_gpio_level(BUZZER_B, 0);
                    vTaskDelay(pdMS_TO_TICKS(250));
                    break;

                // Cor vermelha
                case 2:
                    // Alterna 0.5s on/1.5s off
                    pwm_set_gpio_level(BUZZER_A, wrap*0.05);
                    pwm_set_gpio_level(BUZZER_B, wrap*0.05);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    pwm_set_gpio_level(BUZZER_A, 0);
                    pwm_set_gpio_level(BUZZER_B, 0);
                    vTaskDelay(pdMS_TO_TICKS(1500)); 
                    break;
            }
        }
    }
}

// Task para controle do Display OLED
void vDisplayOLEDTask(){
    // Configurando a I2C
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_t ssd;                                                // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    while(true){
        ssd1306_fill(&ssd, false); // Limpa o display

        // Frame que será reutilizado para todos
        ssd1306_rect(&ssd, 0, 0, 128, 64, cor, !cor);
        // Nome superior
        ssd1306_rect(&ssd, 0, 0, 128, 12, cor, cor); // Fundo preenchido
        ssd1306_draw_string(&ssd, "SEMAFORO", 4, 3, true); // String: Semaforo
        ssd1306_draw_string(&ssd, "TM", 107, 3, true);
        // Modo
        ssd1306_draw_string(&ssd, "MODO:", 4, 16, false);
        // Cor
        ssd1306_draw_string(&ssd, "COR:", 4, 28, false);
        // Borda do tempo
        ssd1306_rect(&ssd, 48, 100, 26, 8, cor, !cor);

        // Modo noturno
        if(night_mode){
            // Modo
            ssd1306_draw_string(&ssd, "NOTURNO", 48, 16, false);
            // Cor
            ssd1306_draw_string(&ssd, "AMARELO", 48, 28, false);
            // Mensagem
            ssd1306_draw_string(&ssd, "ATENCAO", 4, 48, false);
            // Tempo
            ssd1306_draw_string(&ssd, "!", 109, 48, false);

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Modo normal
        else{
            switch(semaforo_state){
                // Luz verde
                case 0:
                    // Modo
                    ssd1306_draw_string(&ssd, "NORMAL", 48, 16, false);
                    // Cor
                    ssd1306_draw_string(&ssd, "VERDE", 48, 28, false);
                    // Mensagem
                    ssd1306_draw_string(&ssd, "LIBERADO", 4, 48, false);
                    // Tempo
                    green_count--; 
                    int_2_string(green_count);
                    ssd1306_draw_string(&ssd, converted_string, 105, 48, false);
                    break;

                // Luz amarela
                case 1:
                    // Modo
                    ssd1306_draw_string(&ssd, "NORMAL", 48, 16, false);
                    // Cor
                    ssd1306_draw_string(&ssd, "AMARELO", 48, 28, false);
                    // Mensagem
                    ssd1306_draw_string(&ssd, "ATENCAO", 4, 48, false);
                    // Tempo
                    yellow_count--; 
                    int_2_string(yellow_count);
                    ssd1306_draw_string(&ssd, converted_string, 105, 48, false);
                    break;

                // Luz vermelha
                case 2:
                    // Modo
                    ssd1306_draw_string(&ssd, "NORMAL", 48, 16, false);
                    // Cor
                    ssd1306_draw_string(&ssd, "VERMELHA", 48, 28, false);
                    // Mensagem
                    ssd1306_draw_string(&ssd, "PARE!", 4, 48, false);
                    // Tempo
                    red_count--; 
                    int_2_string(red_count);
                    ssd1306_draw_string(&ssd, converted_string, 105, 48, false);
                    // Decrementa o contador de tempo a cada 1s
                    break;
            }
            vTaskDelay(1000);
        }

        ssd1306_send_data(&ssd); // Envia os dados para o display, atualizando o mesmo
    }
}

// Task para controlar a matriz de leds
void vLedMatrixTask(){
    // Inicializando a PIO
    pio = pio0;
    sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_MATRIX_PIN, 800000, IS_RGBW);

    float matrix_intensity;

    while(true){
        // Modo noturno
        if(night_mode){
            matrix_intensity = 0.01*matrix_intensity_step;
            yellow_animation(matrix_intensity);
            // Animação de pulsar o desenho na matriz de leds
            if(matrix_intensity_rising){ 
                matrix_intensity_step++;
                if(matrix_intensity_step==10){
                    matrix_intensity_rising=false;
                }
            }
            else{
                matrix_intensity_step--;
                if(matrix_intensity_step==0){
                    matrix_intensity_rising=true;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        // Modo normal
        else{
            switch(semaforo_state){
                // Cor verde
                case 0:
                    green_animation(green_frame_index);
                    green_frame_index++; // Incrementa o index do frame
                    if(green_frame_index>5){ // Limita a no máximo 5 (são 6 frames)
                        green_frame_index=0;
                    }
                    vTaskDelay(pdMS_TO_TICKS(200));
                    break;
    
                // Cor amarela
                case 1:
                    matrix_intensity = 0.01*matrix_intensity_step;
                    yellow_animation(matrix_intensity);
                    // Animação de pulsar o desenho na matriz de leds
                    if(matrix_intensity_rising){ 
                        matrix_intensity_step++;
                        if(matrix_intensity_step==10){
                            matrix_intensity_rising=false;
                        }
                    }
                    else{
                        matrix_intensity_step--;
                        if(matrix_intensity_step==0){
                            matrix_intensity_rising=true;
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                    break;
    
                // Cor vermelha
                case 2:
                    matrix_intensity = 0.005*matrix_intensity_step;
                    red_animation(matrix_intensity);
                    // Animação de pulsar o desenho na matriz de leds
                    if(matrix_intensity_rising){ 
                        matrix_intensity_step++;
                        if(matrix_intensity_step==10){
                            matrix_intensity_rising=false;
                        }
                    }
                    else{
                        matrix_intensity_step--;
                        if(matrix_intensity_step==0){
                            matrix_intensity_rising=true;
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                    break;
            }
        }
    }
}


int main(){
    stdio_init_all();

    xTaskCreate(vTimerSemaforoTask, "Timer Semaforo Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vReadButtonTask, "Read Button Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vLedsRGBSemaforoTask, "Leds Semaforo Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayOLEDTask, "Display OLED Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vLedMatrixTask, "Led Matrix Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}
