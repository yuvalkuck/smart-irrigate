#ifndef SMART_IRRIGATE_GPIO_DECLARAION_H
#define SMART_IRRIGATE_GPIO_DECLARAION_H
// Generated from gpio_declaraion.in.h by CMake. Edit hardware/boards/<BOARD>.cmake instead.

#define LED_Pin              @BOARD_PIN_LED@          // Status LED
#define CONFIG_MODE_Pin      @BOARD_PIN_CONFIG_MODE@  // Boot into Configuration Mode when LOW
#define ONEWIRE_Pin          @BOARD_PIN_ONEWIRE@      // Isolated 1-Wire pin to handle 10m cable noise
#define I2C_Port             @BOARD_PORT_I2C@
#define I2C_SDA_Pin          @BOARD_PIN_I2C_SDA@
#define I2C_SCL_Pin          @BOARD_PIN_I2C_SCL@

// === RS-485 link to the sensor cluster (MAX3485). DE and /RE are tied together ===
#define RS485_TX_Pin         @BOARD_PIN_RS485_TX@
#define RS485_RX_Pin         @BOARD_PIN_RS485_RX@
#define RS485_DE_Pin         @BOARD_PIN_RS485_DE@
#define RS485_UART_Port      @BOARD_PORT_RS485@

// === Analog (ADC) ===
#define SENSORS_ADC_Port       @BOARD_PORT_ADC_SENSORS@
#define WIND_ADC_Port          @BOARD_PORT_ADC_WIND@
#define XDB401_ADC_Port        @BOARD_PORT_ADC_XDB401@
#define PRESSURE_ADC_Channel   @BOARD_ADC_CH_PRESSURE@
#define WIND_EXPANSION_Channel @BOARD_ADC_CH_WIND@

#endif //SMART_IRRIGATE_GPIO_DECLARAION_H
