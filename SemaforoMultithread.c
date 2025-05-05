#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12
#define BUTTON_A 5
#define BUZZER_A 21 
#define BUZZER_B 10

// Variável para indicar qual luz do semáforo está ativa
uint semaforo_state = 0; // 0: Verde | 1: Amarelo | 2: Vermelho
// Variáveis do PWM (setado para freq de 625 Hz)
uint wrap = 2000;
uint clkdiv = 50;
// Variáveis para debounce do botão 
uint32_t last_time = 0; // Armazena o ultimo tempo do botao
bool last_button_state = false; // Armazena o ultimo estado do botao
// Variável que controla o modo noturno
bool night_mode = false;
// Tempos de cada cor no semáforo (em ms)
uint green_time = 15000;
uint yellow_time = 5000;
uint red_time = 10000;


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


// TASKS UTILIZADAS NO CÓDIGO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Task para controlar a temporização do semáforo
void vTimerSemaforoTask(){
    while(true){
        semaforo_state = 0;
        vTaskDelay(pdMS_TO_TICKS(green_time));

        semaforo_state = 1;
        vTaskDelay(pdMS_TO_TICKS(yellow_time));

        semaforo_state = 2;
        vTaskDelay(pdMS_TO_TICKS(red_time));
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

    while(true){

        // Modo noturno
        if(night_mode){
            // Aciona os buzzers durante 200ms
            pwm_set_gpio_level(BUZZER_A, wrap*0.4);
            pwm_set_gpio_level(BUZZER_B, wrap*0.4);
            vTaskDelay(pdMS_TO_TICKS(200));
            // Desativa ambos e espera 3600ms
            pwm_set_gpio_level(BUZZER_A, 0);
            pwm_set_gpio_level(BUZZER_B, 0);
            vTaskDelay(pdMS_TO_TICKS(3800));
        }
        // Modo normal
        else{
            switch(semaforo_state){
                // Cor verde
                case 0:
                    // Alterna 1s on/restante do tempo off
                    pwm_set_gpio_level(BUZZER_A, wrap*0.4);
                    pwm_set_gpio_level(BUZZER_B, wrap*0.4);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    pwm_set_gpio_level(BUZZER_A, 0);
                    pwm_set_gpio_level(BUZZER_B, 0);
                    vTaskDelay(pdMS_TO_TICKS(green_time-1000));
                    break;

                // Cor amarela
                case 1:
                    // Alterna 0,5s on/0,5s of
                    pwm_set_gpio_level(BUZZER_A, wrap*0.4);
                    pwm_set_gpio_level(BUZZER_B, wrap*0.4);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    pwm_set_gpio_level(BUZZER_A, 0);
                    pwm_set_gpio_level(BUZZER_B, 0);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    break;

                // Cor vermelha
                case 2:
                    // Alterna 0.5s on/1.5s off
                    pwm_set_gpio_level(BUZZER_A, wrap*0.4);
                    pwm_set_gpio_level(BUZZER_B, wrap*0.4);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    pwm_set_gpio_level(BUZZER_A, 0);
                    pwm_set_gpio_level(BUZZER_B, 0);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    break;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}


int main(){
    stdio_init_all();

    xTaskCreate(vTimerSemaforoTask, "Timer Semaforo Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vReadButtonTask, "Read Button Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vLedsRGBSemaforoTask, "Leds Semaforo Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    
    

    vTaskStartScheduler();
    panic_unsupported();
}
