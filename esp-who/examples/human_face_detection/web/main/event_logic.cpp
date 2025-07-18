#include <stdio.h>
#include "event_logic.hpp"
#include "who_human_face_recognition.hpp"
#include "iot_button.h"
#include "esp_log.h"

#define GPIO_BOOT GPIO_NUM_0

static const char *TAG = "EVENT_LOGIC";  // ✅ Add this line

static QueueHandle_t xQueueEventO = NULL;
static recognizer_state_t recognizer_state;
static button_handle_t boot_button;

void button_cb(void *button_handle, void *usr_data)
{
    button_event_t event = iot_button_get_event(button_handle);
    switch (event)
    {
    case BUTTON_SINGLE_CLICK:
        recognizer_state = RECOGNIZE;
        ESP_LOGI(TAG, "Single Click Detected");
        break;

    case BUTTON_LONG_PRESS_START:
        recognizer_state = ENROLL;
        ESP_LOGI(TAG, "Long Press Detected");
        break;

    case BUTTON_DOUBLE_CLICK:
        recognizer_state = DELETE;
        ESP_LOGI(TAG, "Double Click Detected");
        break;

    default:
        recognizer_state = DETECT;
        ESP_LOGI(TAG, "Unknown Event");
        break;
    }
    xQueueSend(xQueueEventO, &recognizer_state, portMAX_DELAY);
}

void register_button_events(const QueueHandle_t event_o)
{
    // Save output Queue
    xQueueEventO = event_o;

    // Init BOOT button
    button_config_t boot_button_config;
    boot_button_config.type = BUTTON_TYPE_GPIO;
    boot_button_config.long_press_time = 1500;
    boot_button_config.short_press_time = 100;
    boot_button_config.gpio_button_config.gpio_num = GPIO_BOOT;
    boot_button_config.gpio_button_config.active_level = 0;
    boot_button = iot_button_create(&boot_button_config);
    assert(boot_button);

    // Register button callbacks
    iot_button_register_cb(boot_button, BUTTON_SINGLE_CLICK, button_cb, NULL);
    iot_button_register_cb(boot_button, BUTTON_DOUBLE_CLICK, button_cb, NULL);
    iot_button_register_cb(boot_button, BUTTON_LONG_PRESS_START, button_cb, NULL);
}
