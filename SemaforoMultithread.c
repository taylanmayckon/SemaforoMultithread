#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12

// Variável para indicar qual luz do semáforo está ativa
uint semaforo_state = 0; // 0: Verde | 1: Amarelo | 2: Vermelho
// Variáveis do PWM
uint wrap = 2000;
uint clkdiv = 125;
// Variável para debounce do botão (armazena o último tempo)
uint32_t last_time = 0;


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
    // Tempos de cada cor no semáforo (em ms)
    uint green_time = 15000;
    uint yellow_time = 5000;
    uint red_time = 15000;

    while(true){
        semaforo_state = 0;
        vTaskDelay(pdMS_TO_TICKS(green_time));

        semaforo_state = 1;
        vTaskDelay(pdMS_TO_TICKS(yellow_time));

        semaforo_state = 2;
        vTaskDelay(pdMS_TO_TICKS(red_time));
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
        // Desativando todos os LEDs antes da próxima ação
        pwm_set_gpio_level(LED_RED, 0);
        pwm_set_gpio_level(LED_GREEN, 0);
        pwm_set_gpio_level(LED_BLUE, 0);

        switch(semaforo_state){
            // Cor VERDE
            case 0:
                // Alterna 1s on/1s off
                pwm_set_gpio_level(LED_GREEN, led_luminosity*wrap);
                vTaskDelay(pdMS_TO_TICKS(1000));
                pwm_set_gpio_level(LED_GREEN, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            
            // Cor AMARELA
            case 1:
                // Alterna 0,5s on/0,5s of
                // Amarelo = 0.5*verde + 0.5*vermelho
                pwm_set_gpio_level(LED_GREEN, led_luminosity*wrap);
                pwm_set_gpio_level(LED_RED, led_luminosity*wrap);
                vTaskDelay(pdMS_TO_TICKS(500));
                pwm_set_gpio_level(LED_GREEN, 0);
                pwm_set_gpio_level(LED_RED, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            // Cor VERMELHA
            case 2:
                // Alterna 0.5s on/1.5s off
                pwm_set_gpio_level(LED_RED, led_luminosity*wrap);
                vTaskDelay(pdMS_TO_TICKS(500));
                pwm_set_gpio_level(LED_RED, 0);
                vTaskDelay(pdMS_TO_TICKS(1500));
                break;
        }
    }
}

int main(){
    stdio_init_all();

    xTaskCreate(vTimerSemaforoTask, "Timer Semaforo Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vLedsRGBSemaforoTask, "Leds Semaforo Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    

    vTaskStartScheduler();
    panic_unsupported();
}
