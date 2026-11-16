# Mainboard

Schematic: `mainboard.kicad_sch`. The sheet carries only the wire-colour legend. All the notes are here.

## Valve path

* The valve GPIOs come from the board manager file `components/board_manager/board_peripherals.yaml`
  (`gpio_valve_0` to `gpio_valve_7` = GPIO 1, 14, 18, 21, 22, 16, 17, 5), with `CONFIG_DEVICE_MAX_VALVES` = 8.
  All 8 relay channels are valves.
* GPIO 16/17 are UART0, so the console runs on USB Serial/JTAG.
* IO2 and IO3 (header A1, A2) are the only C6 GPIOs on the FireBeetle 2 header that are not connected.
  More valves, the 24 VAC inverter enable (P12), the
  inverter fault input (P15), the mains-detect input (P11) and the charger status inputs (P16 `STAT1`,
  P17 `STAT2`) use the I2C expander U5.
* The 3.3 V valve GPIOs go through two SN74AHCT125N quad buffers: U10 for valves 1-4 and U11 for
  valves 5-8. The buffer outputs go to J15 pins 2-9 (relay inputs IN1..IN8), which get 5 V logic. All
  `/OE` pins are tied to GND, and each chip has a 100 nF decoupling capacitor (C9, C10). The wires from the
  C6 to the buffers are named `VALVE1`-`VALVE8`.
* The 8-channel relay module is off the board. J15 (1x10, 2.54 mm) is its control header: 1 = GND,
  2-9 = IN1-IN8, 10 = +5V.
* The buffer outputs have no pull resistors. The state of the relay inputs while the C6 GPIOs float at
  reset is unknown. It depends on the input circuit of the relay module.
* The relay contacts are on the relay module, not on this board: J2 pin 1 goes to COM1..8, NO1..8 go
  to valves 1..8, and J2 pin 2 goes to the valve common.

## Power

* J1 takes +24 V DC through the 5 A fuse F3. In V1 an external 24 V DC supply feeds J1. When the PSU
  add-on (`hardware/psu`) is installed, its J5 (24 V DC out) feeds J1 instead; the mainboard does not change.
* The mainboard makes its own +5V and 24 VAC from +24V. The 5S pack, its gauge and protector (BQ40Z80),
  the charger and the 24 V bus converter are on the PSU add-on only.
* BT3 (one 3.7 V 18650 cell) is on J3, the on-board JST PH2.0 battery socket of A1. J3 is part of A1,
  so BT3 and J3 have no footprint. The FireBeetle 2 on-board charger charges BT3.
* J16 (1x8 pin header, 2.54 mm) is the link to the PSU add-on J4: 1 = +5V (out, if the add-on needs it), 2 = GND,
  3 = +3V3 (out), 4 = SDA, 5 = SCL, 6 = `MAINS_OK_N` (in), 7 = `STAT1` (in), 8 = `STAT2` (in).
  `MAINS_OK_N`, `STAT1` and `STAT2` are open-drain outputs of the add-on charger, so R64, R65 and R66
  (100k to +3V3) pull them up. `MAINS_OK_N` goes to U5 P11, `STAT1` to P16 and `STAT2` to P17.
  `MAINS_OK_N` high = no mains (or no add-on).
* The battery gauge (BQ40Z80, SMBus) is on the PSU add-on and is reached through J16 SDA and SCL. It reports
  each cell voltage, pack voltage and current, remaining capacity and state of charge, temperature and
  protection status. Its SMBus speed is 10 to 100 kHz in slave mode, so the shared I2C bus has to run at
  100 kHz or below when the add-on is fitted. Its slave address is not read yet.
* The ESP32-S3 (A3) runs from the switched 5 V line (DevKit 5V0 pin). Q1 (AO3401A P-FET) connects
  +5V to it, and Q2 (BSS138) drives its gate. R34 (100k) holds Q1 off when Q2 is off. The switched 5 V
  line also feeds the LCD (J6) and the HV side of the level shifter U9.
* U5 P13 controls Q2. R35 (100k) pulls it up to +3V3, so the S3 is on by default (also while P13
  floats at boot). Firmware pulls P13 low to power the S3 down.
* The battery-state LED DS12 (with R33, 470 R) is on U5 P07 (`LED_BAT`).
* The buzzer BZ1 is an active 5 V buzzer (it needs a built-in oscillator). Q3 (BSS138) switches it from
  U5 P14, R36 (2.2k) holds the gate low, and D8 (1N4148W) is the flyback diode.
* R36 and R41 are 2.2k because the U5 (TCA9555) ports power up as inputs with an internal pull-up. The
  datasheet gives 100 uA maximum for that pull-up, so the pin stays at 0.22 V or less. That is below the
  BSS138 threshold (0.5 V minimum) and below the TPS7A1601 `EN` off level (0.3 V).

## 5 V (U12, LMR51430)

* Buck from +24V, 3 A part. R37 (100k) and R38 (13.7k) set 4.98 V. L1 6.8 uH. C15 and C16 are 2 x 22 uF
  output, C12 to C14 the input capacitors. `EN` is tied to +24V.

## 24 VAC (sine inverter)

Moved from the PSU board (design values unchanged, references renumbered). It runs whenever +24V is present
and firmware enables it.

* `INV_EN` comes from U5 P12. R41 (2.2k) pulls it low, so the inverter is off until firmware drives it high.
  It gates the 12 V LDO (U13 `EN`), the EG8010 `SPWMEN` and the DRV8412 `RESET_AB`. The 45 V boost enables
  itself from the 12 V rail, so it stops too.
* `INV_FAULT_N` is the DRV8412 `FAULT` pin (open collector, low = fault). It goes to U5 P15 with the R63
  (100k) pull-up to +3V3.
* 12 V (U13, TPS7A1601): R39 (1.15M) over R40 (127k) set about 12 V. C17 feed-forward, C18 input, C19 output.
  It feeds the DRV8412 `GVDD`/`VDD` (about 30 mA) and the boost enable divider. About 0.36 W at 24 V in.
* 45 V boost (U14, LM5155): R49 (107k) over R50 (2.43k) set 45.0 V, R47 (73.2k) about 298 kHz. L2 15 uH,
  Q5 switch, D9 PMEG060V100EPD diode, R44 12 m sense, R45 slope. Output C28/C29 (100 uF 63 V polymer) and
  C30 to C32 (4.7 uF 100 V). Compensation R48, C24, C25 are start values. Enable: R51/R52 divide the 12 V
  rail. Bias from +24V through R42 (0R) with C20.
* EG8010 (U15): bipolar mode, 50 Hz, soft start on, 12 MHz crystal Y1 with C33/C34. `SPWMOUT3` (left-high)
  goes to `PWM_A` through R53/R54, `SPWMOUT1` (right-high) to `PWM_B` through R55/R56 (about 3.5 V from 5 V).
* DRV8412 (U16): mode 000, only bridges A and B used. R62 (36k) sets `OC_ADJ` for about 7.4 A. R61 (1R)
  joins GND and AGND. C39 on `VREG`, C40 to C43 on `GVDD`, C46 to C49 on `PVDD`, C50 to C53 bootstrap.
* Feedback: 47k over 4.53k with 100 nF on each output line (R57/R58/C37, R59/R60/C38) into `VFB` and
  `VFB2`, about 24.1 V RMS. The EG8010 shuts down above 3.15 V peak feedback (300 ms) and below 2.75 V (3 s)
  (EG8010 datasheet, section 8.1).
* Output filter: L3 and L4 (1 mH, one in each line) and C54 (4.7 uF film) across the output. It feeds J2
  through the existing snubber/indicator R6, R30, C3, D7, DS11.

Open points of this block:

* Output common mode: each 24 VAC wire sits at about 22.5 V DC (`PVDD`/2) plus the AC, relative to GND. An
  isolating 50 Hz transformer between C54 and J2 is the proposed fix; not decided and not drawn.
* L3/L4: no part chosen; the footprint is a placeholder. D9: datasheet not read.
* The 45 V boost and the DRV8412 bridge switch near the ESP32 and the sensor inputs: keep them apart on the PCB.

## ESP32-S3 link

* IO6/IO7 are the UART1 link to A3: A3 IO17 TX goes to IO7, and IO6 goes to A3 IO18 RX. J4 (1x3 pin header,
  2.54 mm) is a logic-analyzer tap on the C6 side of the same wires: 1 = `S3_TX`, 2 = `S3_RX`, 3 = GND.
* Q4 (BSS138) isolates the C6 TX line (`S3_RX`, C6 side) from the S3 RX pin (A3 IO18) while the S3 is
  powered down. Its gate is on the switched 5 V line, so the line opens by itself when Q1 cuts the S3 supply. The drain is
  on the C6 side, so the body diode cannot feed the unpowered S3. The S3 TX line (`S3_TX`) needs no
  isolation, and R16 keeps it pulled up on the C6 side.

## RS-485 to the sensor cluster

* J5: 1 = A, 2 = B, 3 = +5V out, 4 = GND.
* The +5V out goes through F2 (MF-RHT050, 0.5 A resettable fuse). D2 (SMAJ5.0A) is on the J5 side of F2,
  and D1 (SM712) is on A and B.
* U4 is on the UART: IO9 TX goes to DI, and IO8 takes RO.
* DE and /RE are on IO15, the on-board LED, which also shows TX. R7 holds DE low at boot.
* R10 (120R) is the terminator, always connected across A and B (no jumper) because this board is the start of
  the bus. R8/R9 are the 390R bias, which sits at this end only.
* SHT41, BMP581 and the solar panel are on the sensor cluster and are read over RS-485 (J5).

## I2C

* The bus is on IO19/IO20 (header pins 28/29; pins 9/10 are the same signals).
* U5 TCA9555 is at 0x20 and U8 ADS1115 is at 0x48 (`ADDR` to GND). R17/R18 are the pull-ups. The bus also
  goes to J16 for the BQ40Z80 gauge on the PSU add-on.

## Analog inputs

* U8 (ADS1115) reads all the analog inputs: AIN0 = XDB401 pressure (J9), AIN1 = wind speed (J10),
  AIN2 = board temperature U7 (MCP9700A). AIN3 is free.
* The 1.8k/3.3k dividers (R2/R1 on AIN0, R4/R3 on AIN1) scale the 0-5 V sensor outputs to 0-3.24 V.
* J8 (DS18B20): 1 = +3V3, 2 = GND, 3 = DATA. R5 (4.7k) is the 1-Wire pull-up.
* J9 (XDB401): 1 = +5V, 2 = GND, 3 = OUT.
* J10 (wind sensor): 1 = +24V, 2 = GND, 3 = OUT. The footprint has 4 terminals and terminal 4 has no net.
* Sensor cable TVS: D3 DS18B20, D4 XDB401, D5 wind OUT, D6 wind +24V. R11 limits the IO4 current.

## LCD and keys on the ESP32-S3

* J6 LCD2004 I2C: 1 = GND, 2 = VCC, 3 = SDA, 4 = SCL. On A3: IO8 is SDA and IO9 is SCL. VCC comes from the
  switched 5 V line (Q1 drain), so the LCD is off when the S3 is powered down.
* J7 (1x5 pin header, 2.54 mm) takes the 4 keys: 1 = GND, 2 = OK, 3 = UP, 4 = DOWN, 5 = BACK. A key
  connects its wire to GND. The resistor ladder is on the board: OK goes straight to the `KEYS` node,
  UP through R12 (2.2k), DOWN through R13 (5.6k), BACK through R14 (15k). `KEYS` goes to A3 IO4
  (ADC1 channel 3). R15 is a 10k pull-up to A3 3V3.

| Key | Voltage |
|---|---|
| OK | 0 V |
| UP | 0.60 V |
| DOWN | 1.18 V |
| BACK | 1.98 V |
| No key | 3.30 V |

A3 IO5-IO7 are free.

## LEDs and buttons on the ESP32-C6 side

* DS1 (main status, 5 mm) is on U5 P00 and DS2-DS7 are on P01-P06. Each LED has its cathode on the port
  and a 470 R resistor (R19-R25) to +3V3, so a low output lights it.
* J12 is the external main LED, in parallel with DS1: 1 = anode (from +3V3 through R26, 470 R),
  2 = cathode (P00).
* SW6 ("SHOW LEDs") is between U5 P10 and GND. It uses the U5 internal pull-up.
* SW1 (CONFIG) is between IO23 and GND. J13 is the external CONFIG button in parallel with it:
  1 = `CFG_SW`, 2 = GND.
* DS8, DS9 and DS10 (red) show the +24V, +5V and +3V3 rails through R27 (47k), R28 (4.7k) and R29 (2.2k).

## Test points

2.0 mm through-hole pads (`TestPoint_THTPad_D2.0mm_Drill1.0mm`), in a row along the bottom edge of the PCB.

| Ref | Net |
|---|---|
| TP1 | +24V |
| TP2 | +5V |
| TP3 | +3V3 |
| TP4 | GND |
| TP5 | `VAC24_A` (24 VAC output, line A) |
| TP6 | +12V (U13 output) |
| TP7 | `VAC24_B` (24 VAC output, line B) |

* The 24 VAC output is the voltage between `VAC24_A` and `VAC24_B`. Each line sits at about 22.5 V DC above GND
  (see the open points of the 24 VAC block), so TP5 alone does not show the 24 VAC. Both lines are also on J2.
* No test point yet for the 45 V boost output (`Net-(D9-K)`, no label in the schematic) or for `INV_EN`.

## Assembly

All Dupont parts are not assembled by the FAB, so they are marked do-not-populate (`dnp yes`) in the schematic:
the pin headers J4-J9, J12, J13, J15, J16 and the female socket rows of the two modules (A1, the ESP32-C6, and
A3, the ESP32-S3). The screw terminals J1, J2 and J10 are not marked. The PCB footprints do not have the
do-not-populate attribute yet; KiCad shows a `footprint_symbol_mismatch` warning for these 12 parts until the PCB
is updated from the schematic.

## Not drawn yet

The 24 VAC output transformer (see the open points of the 24 VAC block). The sheet is A1: the 5 V and
24 VAC blocks sit right of the original drawing and are wired to it.
