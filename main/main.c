#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "esp_bsp.h"          // ⭐ IMPORTANT (real display init)
#include "driver/usb_serial_jtag.h"

static const char *TAG = "P4_USB_DISPLAY";

static QueueHandle_t usb_rx_queue;

// Frame buffer queue size
#define FRAME_SIZE  (1280 * 800 * 2)

// -------------------- USB TASK --------------------
static void usb_serial_task(void *arg)
{
    uint8_t buffer[512];

    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));

    ESP_LOGI(TAG, "USB Serial Ready");

    while (1) {
        int len = usb_serial_jtag_read_bytes(buffer, sizeof(buffer), 100);

        if (len > 0) {
            ESP_LOGI(TAG, "USB RX: %d bytes", len);

            if (usb_rx_queue) {
                xQueueSend(usb_rx_queue, buffer, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// -------------------- DISPLAY TASK --------------------
static void display_task(void *arg)
{
    uint8_t frame[FRAME_SIZE];

    ESP_LOGI(TAG, "Display task started");

    while (1) {
        if (xQueueReceive(usb_rx_queue, frame, pdMS_TO_TICKS(100))) {

            // ⚠️ Placeholder:
            // Real implementation depends on your PC sending format (RGB/MJPEG/etc)

            ESP_LOGI(TAG, "Frame received (%d bytes)", FRAME_SIZE);

            // If using LVGL or BSP framebuffer:
            // memcpy(lcd_buffer, frame, FRAME_SIZE);
            // esp_lcd_panel_draw_bitmap(...);

        }

        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

// -------------------- APP MAIN --------------------
void app_main(void)
{
    ESP_LOGI(TAG, "Booting ESP32-P4 USB Display");

    // ---------------- DISPLAY INIT (THIS FIXES YOUR BLACK SCREEN) ----------------
    ESP_ERROR_CHECK(bsp_display_start());

    // Turn on backlight (THIS IS WHAT YOU WERE MISSING)
    ESP_ERROR_CHECK(bsp_display_backlight_on());

    ESP_LOGI(TAG, "Display initialized via BSP");

    // ---------------- QUEUE ----------------
    usb_rx_queue = xQueueCreate(5, FRAME_SIZE);

    // ---------------- TASKS ----------------
    xTaskCreate(usb_serial_task, "usb_serial", 4096, NULL, 5, NULL);
    xTaskCreate(display_task, "display", 8192, NULL, 4, NULL);

    ESP_LOGI(TAG, "System ready");
}
