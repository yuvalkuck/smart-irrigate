#include <iterator>
#include "hmodbus.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_modbus_master.h"
#include "esp_board_manager_includes.h"
#include "gpio_declaraion.h"

// GPIO number of a "gpio" peripheral declared in components/board_manager/board_peripherals.yaml
static int board_gpio(const char *name) {
    void *periph = nullptr;
    ESP_ERROR_CHECK(esp_board_manager_get_periph_handle(name, &periph));
    return static_cast<periph_gpio_handle_t *>(periph)->gpio_num;
}

static constexpr auto TAG = "hModbus:";
static void* master_handle = nullptr;

// esp-modbus refuses to start with an empty parameter table. Placeholder until the register map is defined.
static const mb_parameter_descriptor_t cluster_parameters[] = {
    {0, "placeholder", "", 1, MB_PARAM_INPUT, 0, 1, 0, PARAM_TYPE_U16, 2, {}, PAR_PERMS_READ},
};

esp_err_t init_modbus() {
    mb_communication_info_t comm = {};
    comm.ser_opts.port = RS485_UART_Port;
    comm.ser_opts.mode = MB_RTU;
    comm.ser_opts.baudrate = HMODBUS_BAUD;
    comm.ser_opts.parity = MB_PARITY_NONE;
    comm.ser_opts.uid = 0;
    comm.ser_opts.response_tout_ms = HMODBUS_TIMEOUT_MS;
    comm.ser_opts.data_bits = UART_DATA_8_BITS;
    comm.ser_opts.stop_bits = UART_STOP_BITS_1;

    esp_err_t rc = mbc_master_create_serial(&comm, &master_handle);
    if (rc != ESP_OK || master_handle == nullptr) return rc != ESP_OK ? rc : ESP_ERR_INVALID_STATE;
    // RTS drives DE and /RE, which the MAX3485 board ties together
    rc = uart_set_pin(RS485_UART_Port, board_gpio("gpio_rs485_tx"), board_gpio("gpio_rs485_rx"),
                      board_gpio("gpio_485de_led"), UART_PIN_NO_CHANGE);
    if (rc != ESP_OK) return rc;
    rc = mbc_master_set_descriptor(master_handle, cluster_parameters, std::size(cluster_parameters));
    if (rc != ESP_OK) return rc;
    rc = uart_set_mode(RS485_UART_Port, UART_MODE_RS485_HALF_DUPLEX);
    if (rc != ESP_OK) return rc;
    rc = mbc_master_start(master_handle);
    if (rc != ESP_OK) return rc;
    ESP_LOGI(TAG, "RS-485 up, %d baud", HMODBUS_BAUD);
    return ESP_OK;
}

esp_err_t modbus_read_registers(uint8_t slave, ModbusFunction function, uint16_t address,
                                uint16_t count, uint16_t* out) {
    if (master_handle == nullptr) return ESP_ERR_INVALID_STATE;
    if (out == nullptr || count == 0 || count > HMODBUS_MAX_REGISTERS) return ESP_ERR_INVALID_ARG;

    mb_param_request_t request = {};
    request.slave_addr = slave;
    request.command = function;
    request.reg_start = address;
    request.reg_size = count;

    uint8_t raw[2 * HMODBUS_MAX_REGISTERS];
    const esp_err_t rc = mbc_master_send_request(master_handle, &request, raw);
    if (rc != ESP_OK) return rc;
    for (uint16_t i = 0; i < count; ++i) {
        out[i] = static_cast<uint16_t>(raw[2 * i] << 8 | raw[2 * i + 1]); // registers arrive big-endian
    }
    return ESP_OK;
}
