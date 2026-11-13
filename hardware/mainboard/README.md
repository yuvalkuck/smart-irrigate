# Mainboard

Schematic: `mainboard.kicad_sch`. The sheet carries only the wire-colour legend. All the notes are here.

## Valve path

* The valve GPIOs come from the board manager file `components/board_manager/board_peripherals.yaml`
  (`gpio_valve_0` to `gpio_valve_7` = GPIO 1, 14, 18, 21, 22, 16, 17, 5), with `CONFIG_DEVICE_MAX_VALVES` = 8.
  All 8 relay channels are valves.
* GPIO 16/17 are UART0, so the console runs on USB Serial/JTAG.
* No safe GPIO is left on the FireBeetle 2 header. More valves, the 24 VAC converter enable and the
  mains-detect input use the I2C expander U5.
* The 3.3 V valve GPIOs go through two SN74AHCT125N quad buffers: U10 for valves 1-4 and U11 for
  valves 5-8. The buffer outputs go to the relay inputs IN1..IN8, which get 5 V logic. All `/OE` pins are
  tied to GND, and each chip has a 100 nF decoupling capacitor (C9, C10). Nets: `VALVE1`-`VALVE8` on the
  C6 side, `RLY1`-`RLY8` on the relay side.
* The buffer outputs have no pull resistors. The state of the relay inputs while the C6 GPIOs float at
  reset is unknown. It depends on the input circuit of the relay module.
* The relay contacts are not in the relay module symbol: J2 pin 1 goes to COM1..8, NO1..8 go to
  valves 1..8, and J2 pin 2 goes to the valve common.

## Power

* J1 takes +24 V DC and floats BT1+BT2 (via the J11 fuse and U2).
* The battery connects at J14 (1 = BAT+, 2 = BAT-). BAT- passes through the shunt R31 to GND.
* U1 makes +5V. U3 makes 24 VAC.
* The U3 enable comes from U5 P12, with the R32 pull-down. It is assumed active-high until the
  converter is chosen.
* U6 INA226 reads the bus voltage and the battery current through the shunt R31.
* The ESP32-S3 (A3) runs from +5V (DevKit 5V0 pin).

## ESP32-S3 link

* IO6/IO7 are the UART1 link to A3: A3 IO17 TX goes to IO7, and IO6 goes to A3 IO18 RX. J4 taps the same
  wires.

## RS-485 to the sensor cluster

* J5: 1 = A, 2 = B, 3 = +5V out, 4 = GND.
* U4 is on the UART: IO9 TX goes to DI, and IO8 takes RO.
* DE and /RE are on IO15, the on-board LED, which also shows TX. R7 holds DE low at boot.
* JP1 fits the 120R terminator. R8/R9 are the 390R bias, which sits at this end only.
* SHT41, BMP581 and the solar panel are on the sensor cluster and are read over RS-485 (J5).

## I2C

* The bus is on IO19/IO20 (header pins 28/29; pins 9/10 are the same signals).
* U5 TCA9555 is at 0x20 and U6 INA226 is at 0x40. R17/R18 are the pull-ups.

## Analog inputs

* The 1.8k/3.3k dividers scale the 0-5 V sensor outputs to 0-3.24 V.
* Sensor cable TVS: D3 DS18B20, D4 XDB401, D5 wind OUT, D6 wind +24V. R11 limits the IO4 current.

## LCD and keys on the ESP32-S3

* J6 LCD2004 I2C: 1 = GND, 2 = VCC, 3 = SDA, 4 = SCL. On A3: IO8 is SDA, IO9 is SCL, and VCC comes from
  the A3 3V3 pin 1.
* The key panel J7 (1 = KEYS, 2 = GND) holds SW2-SW5 and R12-R14, a resistor ladder into A3 IO4
  (ADC1 channel 3). R15 is a 10k pull-up to A3 3V3.

| Key | Voltage |
|---|---|
| SW2 WAKE/OK | 0 V |
| SW3 UP | 0.60 V |
| SW4 DOWN | 1.18 V |
| SW5 BACK | 1.98 V |
| No key | 3.30 V |

A3 IO5-IO7 are free.

## Not drawn yet

* The switch that turns the ESP32-S3 (A3) off on battery.
* The mains-detect input.
