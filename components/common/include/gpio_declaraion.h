//
// Created by uv on 16/08/2026.
//

#ifndef SMART_IRRIGATE_GPIO_DECLARAION_H
#define SMART_IRRIGATE_GPIO_DECLARAION_H
// ====================================================================
// CONFIGURABLE SYSTEM PINS
// ====================================================================
#define GPIO_LED             GPIO_NUM_15  // Status LED
#define GPIO_CONFIG_MODE_PIN GPIO_NUM_23  // Shifted to avoid sensitive analog blocks
#define GPIO_ONEWIRE_BUS     GPIO_NUM_4   // Isolated 1-Wire pin to handle 10m cable noise
#define GPIO_WIND_EXPANSION  GPIO_NUM_5   //
#define I2C_PORT             I2C_NUM_0
#define I2C_SDA_PIN          GPIO_NUM_19
#define I2C_SCL_PIN          GPIO_NUM_20
// === Inter-Chip Communication (UART Link to ESP32-S3) ===
#if defined(ESP32S3_UART)
#define GPIO_S3_LINK_TX      GPIO_NUM_6   // Safe pin -> Connects to ESP32-S3 RX
#define GPIO_S3_LINK_RX      GPIO_NUM_7   // Safe pin -> Connects to ESP32-S3 TX
#define S3_LINK_UART_PORT    UART_NUM_1   // Auxiliary hardware UART port
#define S3_LINK_UART_BUFF    1024         // Buffer size
#endif

#endif //SMART_IRRIGATE_GPIO_DECLARAION_H
