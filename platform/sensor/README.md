# Sensor Platform

Firmware project for the **sensor cluster**, the outdoor node on the sensor mast. It is a separate
STM32CubeMX project (`sensor_platform.ioc`), not part of the ESP-IDF build. The board is in
`../../hardware/sensor-cluster/`.

The node reads the local sensors and answers the ESP32-C6 over RS-485. The C6 is the Modbus master
and polls the cluster; the cluster never sends on its own.

## Hardware

| Item | Value |
|---|---|
| MCU | STM32C011F6P3TR (TSSOP-20, 32 KB flash, 6 KB RAM), matches `Mcu.CPN` in the `.ioc` |
| Clock | Internal, 12 MHz (as set in the `.ioc`) |
| Logic supply | 3.3 V from U3 (AP2112K-3.3), fed from the incoming 5 V |
| Bus transceiver | U2 MAX3485EPA (DIP-8) |
| Bus protection | D1 SM712 on A/B behind R11 and R12 (10 ohm, 1206, in series with A and B), D2 SMAJ5.0A on 5 V |
| Sensor-side protection | D6 SMAJ5.0A across AIN and GND, D7 and D8 (3.3 V bidirectional ESD diodes, SOD-523) on SDA and SCL |
| Cable shield | J1 and J4 pin 5 (SHLD) go to GND through R13 (1 M) in parallel with C9 (1 nF, 1 kV) |
| Input fuse | F1, 5x20 mm fuse (500 mA) in the ELECAPITAL PCB holder (footprint `libraries:FuseHolder_5x20_ELECAPITAL_PCB_Cover`), between the raw 5 V input and D2/FB1. J1.3 and J4.3 are before the fuse, so a daisy-chained node is not fused by this one. |
| Input filter | FB1 470 ohm ferrite on the 5 V, C8 47 uF bulk |
| Indicators | D3 green on 3.3 V (R10 1 k), D4 yellow on TX and D5 yellow on RX (R8, R9 1 k, lit when the line is low) |
| Test points | TP1 +5V, TP2 +3V3, TP3 GND, TP4 A, TP5 B |
| Mounting | H1-H4, M3 holes (3.2 mm) |

Protection is for ESD and small induced surges only. A direct or very near lightning strike is not
covered: that needs a grounded mast with a lightning rod and the cable shield earthed at one end.

Power arrives from the main box on its own twisted pair. There is no 24 V on this board.

### Sensors

| Sensor | Interface | Purpose |
|---|---|---|
| SHT41 | I2C1 (J2, own connector) | Air temperature and relative humidity, for vapor pressure deficit |
| BMP581 | I2C1 (J7, own connector) | Barometric pressure and temperature, for rain-front prediction |
| Solar panel (0.3 W, 5 V polysilicon epoxy module) | ADC, PA0 (J6, own connector) | Sunlight level, for solar load |

The TSL2591 is dropped from the design. SHT41 and BMP581 sit in a ventilated radiation shield and
each connect over a short I2C cable, SHT41 to J2 and BMP581 to J7. The two connectors are in parallel on
the same I2C1 bus. I2C pull-ups (R1, R2, 4.7 k) go to 3.3 V, so both sensors run at 3.3 V logic.

## Pin map

From the netlist of `../../hardware/sensor-cluster/sensor-cluster.kicad_sch`.

| U1 pin | Function | Net |
|---|---|---|
| 1 | I2C1_SDA | J2 pin 2, J7 pin 2, R1 pull-up |
| 20 | I2C1_SCL | J2 pin 3, J7 pin 3, R2 pull-up |
| 7 (PA0) | ADC1_IN0 | Solar divider |
| 8 (PA1) | USART2_DE | U2 DE and /RE, R7 10 k to GND |
| 9 (PA2) | USART2_TX | U2 DI |
| 10 (PA3) | USART2_RX | U2 RO |
| 18 | SWDIO | J5 pin 2 |
| 19 | SWCLK | J5 pin 3 |
| 6 | NRST | J5 pin 4, C3 100 nF |
| 11-17, 2, 3 | unused | J3 breakout (net labels PA4-PA10, PC14, PC15) |

DE and /RE are tied together and driven by the USART2 DE output (hardware driver-enable).

### Connectors

| Ref | Pins |
|---|---|
| J1 (5-pin terminal) | 1 = A, 2 = B, 3 = +5 V, 4 = GND, 5 = SHLD (cable shield) |
| J4 (5-pin terminal) | Same pins as J1, wired in parallel with it, to daisy-chain the bus to another node. The +5 V pin is the raw input, before D2 and FB1. |
| J2 (4-pin 2.54 mm row) | SHT41: 1 = GND, 2 = SDA, 3 = SCL, 4 = 3.3 V out |
| J7 (4-pin 2.54 mm row) | BMP581: same pinout as J2 (1 = GND, 2 = SDA, 3 = SCL, 4 = 3.3 V out) |
| J6 (2-pin 2.54 mm row) | Solar panel, two wires: 1 = + (AIN), 2 = - (GND) |
| J5 (5-pin 2.54 mm row) | SWD programming: 1 = 3.3 V, 2 = SWDIO, 3 = SWCLK, 4 = NRST, 5 = GND |
| J3 (10-pin 2.54 mm row) | Breakout of the unused MCU pins: 1 = PC14, 2 = PC15, 3 = PA4, 4 = PA5, 5 = PA6, 6 = PA7, 7 = PA8, 8 = PA9, 9 = PA10, 10 = GND. 3.3 V only. |
| JP1 | Fits the 120 ohm terminator R6 across A and B. Fit at the bus end only. |

Only AIN tolerates 5 V. SDA, SCL and the SWD pins are 3.3 V only.

### Solar input

The panel connects to J6 (AIN and GND). R5 (22 ohm, 1206, 0.25 W) across AIN and GND is the shunt, so
the panel current becomes a voltage. R3 (10 k) and R4 (15 k) divide it by 0.6 to the PA0 input, with
C7 (100 nF) as filter. Full scale at the pin is 3.0 V for 5.0 V at AIN. The panel current in watts is
only a relative sunlight measure, so calibrate it against a reference.

The panel is a generic DIY solar charger module. Specification from the listing:

| Item | Value |
|---|---|
| Power | 0.3 W |
| Material | Polycrystalline silicon, epoxy board |
| Voltage | 5 V |
| Operating current | 0-60 mA |
| Size | 68 x 36 mm |

The listing gives no open-circuit voltage. At the full 60 mA, R5 drops 1.32 V and dissipates about
80 mW, well inside its 0.25 W rating. PA0 then sees about 0.8 V, about 26 % of the 3.0 V full scale.
Keep this in mind when calibrating: a larger R5 would use more of the ADC range, but it also raises
the panel voltage, so it is not changed yet.

## Power consumption

Datasheet values, at the supply voltage stated for each row. Rows marked **not verified** have no figure
yet: the ST and Analog Devices sites would not serve the datasheet. The board has not been measured.

| Part | Condition | Current | Source |
|---|---|---|---|
| U3 AP2112K-3.3 | Quiescent, no load, VIN = 2.5 V | 55 uA typ, 80 uA max | Diodes AP2112 datasheet |
| SHT41 | Idle | 0.1 uA typ, 1.0 uA max (25 C) | Sensirion SHT4x datasheet |
| SHT41 | While measuring | 350 uA typ, 500 uA max | Sensirion SHT4x datasheet |
| SHT41 | Average, one measurement per second | 0.4 uA (medium repeatability) to 2.4 uA (high) | Sensirion SHT4x datasheet |
| BMP581 | Standby (25 C) | 1.0 uA | Bosch BMP581 datasheet |
| BMP581 | Low-power mode, 1 Hz | 1.3 uA | Bosch BMP581 datasheet |
| U1 STM32C011F6P3TR | Run at 12 MHz | **not verified** | ST datasheet |
| U2 MAX3485EPA | Supply current, driver on and off | **not verified** | Analog Devices datasheet |
| D3 green LED | Always on, R10 1 k | about 1.3 mA | Calculated, assumes 2 V forward voltage |
| D4, D5 yellow LEDs | Only while TX or RX is low, R8 and R9 1 k | about 1.3 mA each while lit | Calculated, assumes 2 V forward voltage |

All parts except the 5 V pair are fed from 3.3 V through U3, so the 5 V current at J1 is the sum of
these plus U3's own quiescent current (a linear regulator passes the load current through).

The total is not computed because U1 and U2 are missing. Measure it on the built board with a meter in
series with J1 pin 3, and set the F1 fuse rating from that (F1 is 500 mA for now, not based on a
measurement).

## RS-485 link

Modbus RTU, 9600 baud, 8N1, matching `../../components/hmodbus/include/hmodbus.h` on the C6
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
(I2C1, ADC1, USART2) are not configured yet. Program through J5 (3.3 V, SWDIO, SWCLK, NRST, GND).

## Open items

* Register map and slave address.
* Peripheral configuration in the `.ioc` to match the pin map.
* Confirm the 4.7 uF capacitor on U1 (C2) against the ST datasheet.
* Solar shunt calibration.
