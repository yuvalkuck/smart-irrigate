#ifndef SMART_IRRIGATE_GPIO_DECLARAION_H
#define SMART_IRRIGATE_GPIO_DECLARAION_H
// Generated from gpio_declaraion.in.h by CMake. Edit hardware/boards/<BOARD>.cmake instead.

#define LED_Pin              GPIO_NUM_@PIN_LED@          // Status LED
#define CONFIG_MODE_Pin      GPIO_NUM_@PIN_CONFIG_MODE@  // Boot into Configuration Mode when LOW
#define ONEWIRE_Pin          GPIO_NUM_@PIN_ONEWIRE@      // Isolated 1-Wire pin to handle 10m cable noise
#define I2C_Port             I2C_NUM_0
#define I2C_SDA_Pin          GPIO_NUM_@PIN_I2C_SDA@
#define I2C_SCL_Pin          GPIO_NUM_@PIN_I2C_SCL@

// === RS-485 link to the sensor cluster (MAX3485). DE and /RE are tied together ===
#define RS485_TX_Pin         GPIO_NUM_@PIN_RS485_TX@
#define RS485_RX_Pin         GPIO_NUM_@PIN_RS485_RX@
#define RS485_DE_Pin         GPIO_NUM_@PIN_RS485_DE@

// === Analog channels (ADC1) ===
#define PRESSURE_ADC_Pin     ADC_CHANNEL_@ADC_CH_PRESSURE@
#define WIND_EXPANSION_Pin   ADC_CHANNEL_@ADC_CH_WIND@

#endif //SMART_IRRIGATE_GPIO_DECLARAION_H
