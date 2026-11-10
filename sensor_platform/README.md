# Sensor Platform

Firmware project for the **sensor cluster**, the outdoor node on the sensor mast. It is a separate
STM32CubeMX project (`sensor_platform.ioc`), not part of the ESP-IDF build. The board is in
`hardware/sensor-cluster/`.

The node reads the local sensors and answers the ESP32-C6 over RS-485. The C6 is the Modbus master
and polls the cluster; the cluster never sends on its own.

## Hardware

| Item | Value |
|---|---|
| MCU | STM32C011F6P6 (TSSOP-20, 32 KB flash, 6 KB RAM), `Mcu.CPN` in the `.ioc` is STM32C011F6P3 |
| Clock | Internal, 12 MHz (as set in the `.ioc`) |
| Logic supply | 3.3 V from U3 (AP2112K-3.3), fed from the incoming 5 V |
| Bus transceiver | U2 MAX3485EPA (DIP-8) |
| Bus protection | D1 SM712 on A/B, D2 SMAJ5.0A on 5 V |
| Input filter | FB1 470 ohm ferrite on the 5 V, C8 47 uF bulk |

Power arrives from the main box on its own twisted pair. There is no 24 V on this board.

### Sensors

| Sensor | Interface | Purpose |
|---|---|---|
| SHT41 | I2C1 (J2) | Air temperature and relative humidity, for vapor pressure deficit |
| BMP581 | I2C1 (J2) | Barometric pressure and temperature, for rain-front prediction |
| Solar panel | ADC, PA0 (J2 pin 1) | Sunlight level, for solar load |

The TSL2591 is dropped from the design. SHT41 and BMP581 sit in a ventilated radiation shield and
connect to J2 over a short I2C cable. I2C pull-ups (R1, R2, 4.7 k) go to 3.3 V, so both sensors run at
3.3 V logic.

## Pin map

From the netlist of `hardware/sensor-cluster/sensor-cluster.kicad_sch`.

| U1 pin | Function | Net |
|---|---|---|
| 1 | I2C1_SDA | J2 pin 3, R1 pull-up |
| 20 | I2C1_SCL | J2 pin 4, R2 pull-up |
| 7 (PA0) | ADC1_IN0 | Solar divider |
| 8 (PA1) | USART2_DE | U2 DE and /RE, R7 10 k to GND |
| 9 (PA2) | USART2_TX | U2 DI |
| 10 (PA3) | USART2_RX | U2 RO |
| 18 | SWDIO | J2 pin 6 |
| 19 | SWCLK | J2 pin 7 |
| 6 | NRST | J2 pin 8, C3 100 nF |
| 11-17, 2, 3 | unused | unconnected |

DE and /RE are tied together and driven by the USART2 DE output (hardware driver-enable).

### Connectors

| Ref | Pins |
|---|---|
| J1 (4-pin terminal) | 1 = A, 2 = B, 3 = +5 V, 4 = GND |
| J2 (8-pin 2.54 mm row) | 1 = AIN, 2 = GND, 3 = SDA, 4 = SCL, 5 = 3.3 V out, 6 = SWDIO, 7 = SWCLK, 8 = NRST |
| JP1 | Fits the 120 ohm terminator R6 across A and B. Fit at the bus end only. |

Only AIN tolerates 5 V. SDA, SCL and the SWD pins are 3.3 V only.

### Solar input

The panel connects to J2 pin 1 and GND. R5 (22 ohm, 1206, 0.25 W) across AIN and GND is the shunt, so
the panel current becomes a voltage. R3 (10 k) and R4 (15 k) divide it by 0.6 to the PA0 input, with
C7 (100 nF) as filter. Full scale at the pin is 3.0 V for 5.0 V at AIN. The panel current in watts is
only a relative sunlight measure, so calibrate it against a reference.

## RS-485 link

Modbus RTU, 9600 baud, 8N1, matching `components/hmodbus/include/hmodbus.h` on the C6
(`HMODBUS_BAUD`, `HMODBUS_TIMEOUT_MS` = 200 ms). The cluster is a slave. It must support function
`0x03` (read holding registers) and `0x04` (read input registers), and answer a bad request with a
Modbus exception.

Bus end rules: 120 ohm terminator at both ends, 390 ohm bias (A to 3.3 V, B to GND) at the C6 end only.
The cluster end fits only the terminator, through JP1.

C6 side: GPIO 9 TX, GPIO 8 RX, GPIO 2 DE and /RE, through its own MAX3485, terminal J5
(1 = A, 2 = B, 3 = +5 V, 4 = GND).

### Register map

**TBD.** The slave address and register layout are not defined yet. Decide them with the C6 firmware.
Candidate input registers, all 16-bit, big-endian on the wire:

| Quantity | Source |
|---|---|
| Air temperature | SHT41 |
| Relative humidity | SHT41 |
| Pressure | BMP581 |
| Solar level | ADC PA0 |
| Status / sensor-online flags | Firmware |

## Build and flash

Open `sensor_platform.ioc` in STM32CubeMX (6.18.1, package STM32Cube FW_C0 V1.4.1) and generate the
GCC project. Current state: the `.ioc` only has RCC, SYS and the NVIC; the peripherals above
(I2C1, ADC1, USART2) are not configured yet. Program through J2 (SWDIO, SWCLK, NRST, GND, 3.3 V).

## Open items

* Register map and slave address.
* Peripheral configuration in the `.ioc` to match the pin map.
* Confirm the 4.7 uF capacitor on U1 (C2) against the ST datasheet.
* Solar shunt calibration.
