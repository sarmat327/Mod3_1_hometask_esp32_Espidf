#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LDR_GPIO GPIO_NUM_1     //ADC
#define LED_GPIO GPIO_NUM_21    //RED_LED
#define SMA_WINDOW 8

#define UP_THRESHOLD     3000
#define LOW_THRESHOLD    2000
#define LED_ON              1
#define LED_OFF             0

static uint16_t buf[SMA_WINDOW];
static uint8_t idx = 0;
static uint32_t sum = 0;
static uint8_t count = 0; //для першої ініціалізація фільтра
int data_filtered = 0;

static int sma_filtered(int raw)
{
    
    if (count < SMA_WINDOW){
        count++;
    } 

    sum = sum-buf[idx];
    buf[idx] = raw;
    sum = sum + buf[idx];
    idx = (idx+1)%SMA_WINDOW;  //кльова штука

    return sum / count; //якщо початок роботи не важливий то можна без counter - retunr sum/SMA_WINDOW

}

void app_main(void)
{
    // 1. Створюємо ADC1 (як HAL_ADC_Init)
    adc_oneshot_unit_handle_t adc1;
    adc_oneshot_unit_init_cfg_t unit_cfg = {0};
    unit_cfg.unit_id = ADC_UNIT_1;


    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1));

    // 2. Дізнаємось канал за номером GPIO
    adc_unit_t unit;
    adc_channel_t channel;

    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(LDR_GPIO, &unit, &channel));

    printf("unit = %d, channel = %d\n", unit, channel); // ПИТАННЯ - як це працює, коли канал визначається автоматично?

    // 3. Налаштовуємо канал 

    adc_oneshot_chan_cfg_t chan_cfg = {0};
    chan_cfg.atten = ADC_ATTEN_DB_12;
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1, channel, &chan_cfg));

    // 4. Світлодіод
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    
    

    // 5. Цикл
    while (1) {
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1, channel, &raw));
        data_filtered = sma_filtered(raw);
        printf("raw = %d  filtered = %d\n", raw, data_filtered);
        
        if(data_filtered >= UP_THRESHOLD)
        {
            gpio_set_level(LED_GPIO, LED_OFF);
        }
         if(data_filtered <= LOW_THRESHOLD)
        {
            gpio_set_level(LED_GPIO, LED_ON);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}