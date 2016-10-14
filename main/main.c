#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

SemaphoreHandle_t xSemaphore;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

typedef struct {
    int input;
    int output;
} thread_args;

int fib(int n)
{
    if (n < 2) return n;
    else {
        int x = fib(n - 1);
        int y = fib(n - 2);
        return x + y;
    }
}

void core1_task(void *ptr)
{
    int i = ((thread_args *) ptr)->input;
    ((thread_args *) ptr)->output = fib(i);
    xSemaphoreGive( xSemaphore );
    vTaskDelete(NULL);
}

int test_single_core(int num)
{
    int start, end, f;
    start = xTaskGetTickCount();
    f = fib(num);
    end = xTaskGetTickCount();
    return end - start;
}

int test_dual_core(int num)
{
    thread_args args;
    int start, end, f;

    start = xTaskGetTickCount();

    args.input = num - 1;
    xTaskCreatePinnedToCore(core1_task, "core1", 10*1024, &args, 10, NULL, 1);
    f = fib(num - 2);
    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    f += args.output;
    end = xTaskGetTickCount();
    return end - start;
}

void core0_task(void *p)
{
    int i, max_fib_test = 100, single_time, dual_time;
    float ratio;
    printf("+------------------------------------+\r\n");
    printf("|    ESP32 Fibonacci performance     |\r\n");
    printf("+------------------------------------+\r\n");
    printf("N \t Single Core \t Dual core \t Ratio\r\n");

    xSemaphore = xSemaphoreCreateBinary();

    for (i = 20; i < max_fib_test; i++) {
        single_time = test_single_core(i);
        dual_time = test_dual_core(i);
        ratio =  (float)single_time / (float)dual_time;
        printf("%d\t\t%d\t\t%d\t%03.2f\r\r\n", i, single_time, dual_time, ratio);
    }
}
int app_main(void)
{
    nvs_flash_init();
    system_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    core0_task(NULL);

    while (true) {
        printf("tick\r\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    return 0;
}

