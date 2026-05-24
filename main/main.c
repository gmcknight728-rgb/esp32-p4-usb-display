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
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/touch_pad.h"

static const char *TAG = "ESP32P4_USB_DISPLAY";

// Display configuration for Waveshare 10.1" screen
#define LCD_H_RES 1280
#define LCD_V_RES 800
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8
#define LCD_CLK_SPEED_HZ (20 * 1000 * 1000)
#define LCD_SPI_HOST SPI2_HOST

// SPI pins for LCD
#define LCD_PIN_NUM_MOSI 11
#define LCD_PIN_NUM_MISO 13
#define LCD_PIN_NUM_CLK 12
#define LCD_PIN_NUM_CS 10
#define LCD_PIN_NUM_DC 8
#define LCD_PIN_NUM_RST 4
#define LCD_PIN_NUM_BACKLIGHT 9

// Touch pins
#define TOUCH_I2C_NUM I2C_NUM_0
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

static esp_lcd_panel_handle_t panel_handle = NULL;
static QueueHandle_t usb_rx_queue = NULL;
static QueueHandle_t touch_event_queue = NULL;

// Initialize LCD panel
static void lcd_init(void)
{
    ESP_LOGI(TAG, "Initializing LCD...");
    
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_PIN_NUM_MOSI,
        .miso_io_num = LCD_PIN_NUM_MISO,
        .sclk_io_num = LCD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_PIN_NUM_CS,
        .dc_gpio_num = LCD_PIN_NUM_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_CLK_SPEED_HZ,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
    };
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701s(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    // Setup backlight
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_NUM_BACKLIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_NUM_BACKLIGHT, 1));
    
    ESP_LOGI(TAG, "LCD initialized successfully");
}

// USB serial communication task
static void usb_serial_task(void *arg)
{
    uint8_t buffer[256];
    usb_serial_jtag_driver_config_t usb_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_config));
    
    ESP_LOGI(TAG, "USB Serial initialized");
    
    while (1) {
        int len = usb_serial_jtag_read_bytes(buffer, sizeof(buffer), portMAX_DELAY);
        if (len > 0) {
            // Process received data
            // This will handle MJPEG video frames from PC
            ESP_LOGI(TAG, "Received %d bytes from USB", len);
            
            // Queue for display task
            xQueueSend(usb_rx_queue, buffer, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Touch input task
static void touch_input_task(void *arg)
{
    // Initialize touch controller (GT911 for Waveshare 10.1")
    // Configure I2C for touch
    
    ESP_LOGI(TAG, "Touch input task started");
    
    while (1) {
        // Read touch coordinates
        // Send as USB HID mouse input to PC
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Display video frame task
static void display_video_task(void *arg)
{
    uint8_t frame_buffer[LCD_H_RES * LCD_V_RES * 2]; // RGB565 buffer
    
    ESP_LOGI(TAG, "Display video task started");
    
    while (1) {
        // Receive MJPEG frame from USB queue
        if (xQueueReceive(usb_rx_queue, frame_buffer, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Decompress JPEG (if needed)
            // Display frame on LCD
            ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, frame_buffer));
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
