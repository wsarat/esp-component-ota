#include <stdio.h>
#include "ota.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "driver/gpio.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

static const char *TAG = "OTA Component";

esp_err_t _ota_http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

void ota_task_start(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA task");

    esp_http_client_config_t config = {
        .url = (char*)pvParameter,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = _ota_http_event_handler,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    free(pvParameter);
}

void ota_start(char *url)
{
    char *buf = malloc(sizeof(char)*255);
    snprintf(buf, 255, "%s", url);
    xTaskCreate(&ota_task_start, "ota_task", 8192, buf, 5, NULL);
}

void ota_wait_pin(int gpio_num, int timeout_ms, int pressed)
{
    uint64_t pin_select = (1ULL << gpio_num);

    // Configure GPIO0 as input
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // No interrupt
    io_conf.mode = GPIO_MODE_INPUT;        // Set as input
    io_conf.pin_bit_mask = pin_select; // Set the pin mask
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // Enable the pull-down resistor
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; // Disable pull-up resistor
    gpio_config(&io_conf); // Apply the configuration

    // Wait for 10 seconds
    uint32_t start_time = xTaskGetTickCount();
    const uint32_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    ESP_LOGI(TAG, "Press IO_0 to start OTA");
    while (xTaskGetTickCount() - start_time < timeout_ticks) {
        // Check if the button is pressed
        if (gpio_get_level(gpio_num) == pressed) {
            ota_start(CONFIG_OTA_FIRMWARE_UPGRADE_URL); // Call OTA function if button is pressed
            return; // Exit the function after starting OTA
        }
        // Small delay to avoid busy waiting
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100 ms
    }
}

void ota_info(void)
{
    // Get the currently running partition
    const esp_partition_t *current_partition = esp_ota_get_running_partition();

    if (current_partition) {
        printf("Current Partition Information:\n");
        printf("  Label: %s\n", current_partition->label);
        printf("  Address: 0x%lx\n", current_partition->address);
        printf("  Size: %ld bytes\n", current_partition->size);
        printf("  Type: %d\n", current_partition->type);
        printf("  Subtype: %d\n", current_partition->subtype);
    } else {
        printf("No current partition found.\n");
    }
}