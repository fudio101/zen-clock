// SPDX-License-Identifier: MIT
// BSP Buttons — GPIO interrupt + debounce task for 2 buttons

#include "bsp.h"
#include "board_config.h"

#include <esp_log.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "bsp_buttons";

#define BTN_DEBOUNCE_MS   50
#define BTN_TASK_STACK    2048
#define BTN_TASK_PRIORITY 3

static const int s_btn_pins[BSP_BTN_COUNT] = {PIN_BTN_BOOT, PIN_BTN_IO14};
static bsp_button_cb_t s_callback = NULL;
static QueueHandle_t s_btn_queue = NULL;

// ============================================================
// GPIO ISR — sends button ID to queue
// ============================================================
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
  int btn_id = (int)(intptr_t)arg;
  xQueueSendFromISR(s_btn_queue, &btn_id, NULL);
}

// ============================================================
// Button task — debounces and dispatches callbacks
// ============================================================
static void button_task(void *pvParameters)
{
  int btn_id;
  bool last_state[BSP_BTN_COUNT] = {true, true}; // pulled-up = HIGH (not pressed)

  while (1)
  {
    if (xQueueReceive(s_btn_queue, &btn_id, portMAX_DELAY))
    {
      // Debounce: wait then re-read
      vTaskDelay(pdMS_TO_TICKS(BTN_DEBOUNCE_MS));

      bool level = gpio_get_level(s_btn_pins[btn_id]);
      if (level != last_state[btn_id])
      {
        last_state[btn_id] = level;
        bool pressed = !level; // active LOW
        if (s_callback)
        {
          s_callback(btn_id, pressed);
        }
      }
    }
  }
}

// ============================================================
// Public API
// ============================================================
void bsp_buttons_init(bsp_button_cb_t callback)
{
  ESP_LOGI(TAG, "Initializing buttons...");

  s_callback = callback;
  s_btn_queue = xQueueCreate(10, sizeof(int));

  // Install GPIO ISR service (ignore if already installed)
  esp_err_t err = gpio_install_isr_service(0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
  {
    ESP_ERROR_CHECK(err);
  }

  // Configure button GPIOs with interrupt
  for (int i = 0; i < BSP_BTN_COUNT; i++)
  {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << s_btn_pins[i]),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_isr_handler_add(s_btn_pins[i], gpio_isr_handler, (void *)(intptr_t)i));
  }

  // Start button processing task
  xTaskCreatePinnedToCore(button_task, "buttons", BTN_TASK_STACK, NULL, BTN_TASK_PRIORITY, NULL, 0);

  ESP_LOGI(TAG, "Buttons ready: GPIO%d (BOOT), GPIO%d (IO14)", PIN_BTN_BOOT, PIN_BTN_IO14);
}
