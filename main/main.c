#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/usb_serial_jtag.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "ESP32P4_USB_DISPLAY";

// Display configuration for Waveshare 10.1" screen
#define LCD_H_RES 1280
#define LCD_V_RES 800

// SPI pins for LCD
#define LCD_PIN_NUM_MOSI 11
#define LCD_PIN_NUM_MISO 13
#define LCD_PIN_NUM_CLK 12
#define LCD_PIN_NUM_CS 10
#define LCD_PIN_NUM_DC 8
#define LCD_PIN_NUM_RST 4
#define LCD_PIN_NUM_BACKLIGHT 9

// Touch pins
#define TOUCH_I2C_SCL_IO 7
#define TOUCH_I2C_SDA_IO 6

// USB packet structure for mouse/keyboard
typedef struct {
    uint8_t type;  // 0 = mouse, 1 = keyboard
    uint16_t x;
    uint16_t y;
    uint8_t buttons;
    uint8_t keys[6];
} usb_input_packet_t;

// Frame buffer for video
typedef struct {
    uint8_t *data;
    uint32_t size;
    uint16_t width;
    uint16_t height;
} frame_buffer_t;

static QueueHandle_t usb_rx_queue = NULL;
static QueueHandle_t touch_event_queue = NULL;

// Simple SPI LCD initialization
static void lcd_init(void)
{
    ESP_LOGI(TAG, "Initializing LCD...");
    
    // Initialize SPI bus for LCD
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_PIN_NUM_MOSI,
        .miso_io_num = LCD_PIN_NUM_MISO,
        .sclk_io_num = LCD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    // Setup control pins (DC, RST, Backlight)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_PIN_NUM_DC) | (1ULL << LCD_PIN_NUM_RST) | (1ULL << LCD_PIN_NUM_BACKLIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Reset LCD
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_NUM_RST, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_NUM_RST, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Enable backlight
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_NUM_BACKLIGHT, 1));
    
    ESP_LOGI(TAG, "LCD initialized successfully");
}
while (1) {
    gpio_set_level(LCD_PIN_NUM_BACKLIGHT, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(LCD_PIN_NUM_BACKLIGHT, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}


// USB serial communication task
static void usb_serial_task(void *arg)
{
    uint8_t buffer[256];
    usb_serial_jtag_driver_config_t usb_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_config));
    
    ESP_LOGI(TAG, "USB Serial initialized");
    
    while (1) {
        int len = usb_serial_jtag_read_bytes(buffer, sizeof(buffer), 100);
        if (len > 0) {
            // Process received data
            // This will handle MJPEG video frames from PC
            ESP_LOGI(TAG, "Received %d bytes from USB", len);
            
            // Queue for display task
            if (usb_rx_queue != NULL) {
                xQueueSend(usb_rx_queue, buffer, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Touch input task
static void touch_input_task(void *arg)
{
    ESP_LOGI(TAG, "Touch input task started");
    
    while (1) {
        // Read touch coordinates from I2C
        // Send as USB HID mouse input to PC
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Display video frame task
static void display_video_task(void *arg)
{
    uint8_t frame_buffer[256];
    
    ESP_LOGI(TAG, "Display video task started");
    
    while (1) {
        // Receive frame data from USB queue
        if (xQueueReceive(usb_rx_queue, frame_buffer, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Process and display frame
            ESP_LOGD(TAG, "Displaying frame");
        }
        vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-P4 USB Display Application Starting");
    ESP_LOGI(TAG, "Display: 10.1\" @ %d x %d", LCD_H_RES, LCD_V_RES);
    
    // Create queues
    usb_rx_queue = xQueueCreate(5, 256);
    touch_event_queue = xQueueCreate(10, sizeof(usb_input_packet_t));
    
    // Initialize LCD
    lcd_init();
    
    // Start tasks
    xTaskCreate(usb_serial_task, "usb_serial", 4096, NULL, 5, NULL);
    xTaskCreate(display_video_task, "display_video", 4096, NULL, 4, NULL);
    xTaskCreate(touch_input_task, "touch_input", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "All tasks started");
    
    // Keep main task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
