# Battery add-on board

Schematic: `battery.kicad_sch` (one sheet, all blocks wired pin to pin). The sheet carries only the wire-colour legend and the block titles.
All the notes are here. This folder has no PCB yet.

The battery board is an add-on for the battery stage. V1 of the system runs without it: the mainboard takes 24 V DC from an
external supply at its J1 and makes its own +5V and 24 VAC (`hardware/mainboard/README.md`). When the add-on is
installed, the external 24 V DC supply moves to the add-on J1, and the add-on J5 feeds the mainboard J1.
Nothing is on both boards: the add-on holds the battery, its gauge and protector, its charger and the 24 V DC bus that
keeps the mainboard running on battery.

## Rails and sources

| Output | Connector | Value | Source |
|---|---|---|---|
| 24 V DC to the mainboard | J5 | 23.9 V typical, rated 2 A | LM5176 buck-boost (U6) |
| Pack charge | J2 | 20.82 V typical, 0.498 A | BQ24610 charger (U4) |

## How power flows

```
24 V PSU -- J1 -- F1 -- ideal diode U1/Q1 --+-- BUS (14 to 25 V)
                                             |
pack 5S -- J2 -- F2 -- Q3 (CHG) -- Q4 (DSG) -- PACKP -- ideal diode U2/Q2 --+
   B- (VSS) -- R69 (2 mohm) -- GND   (BQ40Z80 U3 drives Q3 and Q4, counts the current in R69)
                                             |
BUS --+-- U4 charger (21 V, 0.5 A) ----------> PACKP -> pack
      +-- U6 buck-boost -------------------> +24V (J5) -> mainboard J1
```

* The pack is 5S so that its full voltage (5 x 4.2 V = 21 V) stays below the 24 V supply. The two ideal diodes then switch sources by themselves, with no relay.
* BUS range is 14 V to 25 V. The lower limit is the cell under-voltage cut (2.8 V x 5 cells = 14 V); the BQ40Z80 limits are set in its data flash.
* U6 holds +24V at 23.9 V over the whole bus range, so the mainboard sees the same 24 V on mains and on battery.
* The charger is fed from BUS (after U1), not from the PSU node. It has no power-path mode, so the pack never feeds the PSU node through the charger.
* On mains, the charger takes only the 0.5 A set by its sense resistor R18. U6 takes the rest from the PSU through the same bus.
* Q3 and Q4 are high-side N-FETs with their drains joined in the middle (`MID`), as in the BQ40Z80 datasheet: CHG drives Q3 against the cell-stack side, DSG drives Q4 against the pack side.

## Connectors

| Ref | Pins | Use |
|---|---|---|
| J1 | 1 = 24 V in, 2 = GND | external 24 V DC supply |
| J2 | 1 = pack B+ (`BP`), 2 = pack B- (`VSS`) | pack power |
| J3 | 1 = B-, 2 to 5 = cell taps 1 to 4, 6 = B+, 7 = gauge `TS1`, 8 = charger `TS`, 9 = GND | pack sense (1x9 pin header, 2.54 mm): balance taps and the two 10 k NTCs (103AT) |
| J4 | 1 = +5V (from the mainboard), 2 = GND, 3 = +3V3 (from the mainboard), 4 = SDA, 5 = SCL, 6 = `MAINS_OK_N`, 7 = `STAT1`, 8 = `STAT2` | link to mainboard J16 (1x8 pin header, 2.54 mm, same pin order) |
| J5 | 1 = +24V, 2 = GND | 24 V DC out to mainboard J1 |

Pack B- (`VSS`) is not the same net as GND: the 2 mohm sense resistor R69 sits between them.
The gauge NTC returns to J3 pin 1 (B-), the charger NTC to J3 pin 9 (GND).

## What the hardware reports to the mainboard

| Metric | Source | Reaches the mainboard on |
|---|---|---|
| Each cell voltage (5 cells), pack voltage | BQ40Z80, cell taps VC1 to VC5 | SMBus: J4 pins 4 and 5 |
| Pack current, both directions | BQ40Z80, coulomb counter on R69 | SMBus |
| Charge amount (remaining capacity, state of charge, full-charge capacity, cycle count) | BQ40Z80, Impedance Track gas gauge | SMBus (datasheet: "available capacity, voltage, current, temperature, and other critical parameters") |
| Pack temperature | BQ40Z80 `TS1` (the NTC on J3 pin 7); `TS2` to `TS4` are free | SMBus |
| Protection status (OV, UV, overcurrent, short circuit, temperature) | BQ40Z80 | SMBus |
| Charger state: charging, done, fault | BQ24610 `STAT1` and `STAT2` (open drain) | J4 pins 7 and 8 (pull-ups R65 and R66 are on the mainboard) |
| Mains present | BQ24610 `PG` (open drain) | J4 pin 6 (`MAINS_OK_N`, pull-up R64 on the mainboard) |

* `MAINS_OK_N`: low means BUS is above the pack, which means the supply is present. High means battery. If the pin sticks high, firmware sees "battery", which is the safe side.
* The BQ40Z80 SMBus pins (`SMBD`, `SMBC`) accept 1.3 V as high and 0.8 V as low (datasheet, electrical characteristics), so the 3.3 V pull-ups on the mainboard I2C bus are enough. Its SMBus speed is 10 to 100 kHz in slave mode, so the shared mainboard bus has to run at 100 kHz or below.
* The BQ40Z80 SMBus slave address is in its technical reference manual (SLUUBT5), which I have not read.
* Firmware items that follow from this (not changed here): read `MAINS_OK_N`, `STAT1` and `STAT2`, apply the 2-valve limit while `MAINS_OK_N` is high, and read the BQ40Z80 over SMBus.

## Block values

Source tags: D = value or part taken from the datasheet. C = computed with that datasheet's equation. T = start value, tune on the bench.

### Input and bus
* F1 is a 10 A 5x20 fuse at the input. U1/Q1 (PSU) and U2/Q2 (pack) are LM74700-Q1 ideal-diode controllers with DMT6007LFG FETs. C1 to C4 are the 100 nF charge-pump and anode capacitors (D).
* Bus capacitors: C5 220 uF polymer and C6 to C9, 4 x 10 uF.

### Battery gauge and protector (U3, BQ40Z80)
* 2-series to 6-series pack manager with gas gauging, protection, cell balancing and SMBus, VQFN-32 (datasheet SLUSBV4D). Used as 5S: `VC6` is tied to `VC5` ("if not used, connect to VC5", D).
* Cell taps: R64 to R68 (100 R) and C94 to C98 (100 nF) on `VC1` to `VC5`, as in the datasheet (D). The internal balancing FETs are 200 R, so the 100 R series resistors set the balance current (D).
* Current: R69 is a 2 mohm 1 W 50 ppm sense resistor in the pack minus lead, between `VSS` (the cell stack) and GND. The datasheet supports 1 to 3 mohm and wants 50 ppm or better (D). R70 and R71 (100 R) with C99 (100 pF) and C100/C101 (100 nF) filter `SRP` and `SRN` (D).
* Supply: `BAT` from `BP` through D5 and R72 (100 R); `VCC` from the middle of the FETs through D6 and R73 (100 R); `PACK` from the pack side through R74 (100 R). C102 (2.2 uF) on `PBI` (D). The two diodes and the series values are start values (T).
* FETs: Q3 (charge) and Q4 (discharge) are DMT6007LFG, 60 V. Gate resistors R75 and R77 (10 k) and gate-source resistors R76 and R78 (10 M), as in the datasheet (D). C103 and C104 (100 nF in series) across the pack terminals are the datasheet ESD pair.
* Temperature: the pack NTC (J3 pin 7) is on `TS1`. The BQ40Z80 has an internal 18 k pull-up for a 10 k 103 NTC (D), so no bias resistor is fitted.
* Unused pins: `TS2`, `TS3`, `TS4`, `DISP`, `PDSG`, `PRES`, `FUSE` are tied to `VSS` ("connect directly to VSS and configure data flash accordingly", D). `LEDCNTLA/B/C` go to `VSS` through R79, R80 and R81, 20 k each ("if not used, connect to VSS with a 20-kΩ resistor", D). `PCHG` is left open ("leave floating if not used", D). The pre-charge FET and the chemical fuse are not used.
* F2 is the 10 A fuse in the pack plus lead, between `BP` and the charge FET.
* One-time setup: the chip must be configured with TI bqStudio (cell count, chemistry, calibration; datasheet section 7.3.1) before the gauge readings are valid.

### Charger (U4, BQ24610)
* Charge voltage: R19 (909 k) and R20 (102 k), both 0.1 %, give 20.82 V typical, which is 4.16 V per cell. The worst case with tolerances is 20.96 V (4.19 V per cell). (C)
* Charge current: R18 (100 m) with ISET1 at 1.00 V gives about 0.50 A (R21 100 k over R22 43.2 k set 0.498 A). (C)
* Termination and pre-charge: R23 100 k over R24 17.8 k set about 49.9 mA. (C)
* C30 (100 nF) on `TTC` sets the safety timer to about 9.3 h. (D)
* NTC window 0 to 45 C: R25 9.31 k, R26 430 k, NTC at J3 pin 8. (D)
* L1 6.8 uH, Q5 (dual DMN6040SSD) as the two switches, D1 and C26 as the bootstrap.
* `STAT1` and `STAT2` (open drain) go to J4. The power-path pins and the input-current regulation are not used. This follows the BQ24610 datasheet's typical application without power path (section 10.2.2).

### 24 V DC (U6, LM5176)
* Input 14 V to 25 V, output 23.9 V (R36 309 k over R37 10.7 k), switching about 297 kHz (R32 27.4 k). L3 15 uH. (C)
* R29 is the 15 m sense resistor. The minimum boost current limit is about 6.7 A against a peak need of about 4.6 A at 2 A load. (C)
* Compensation R33 5.76 k, C40 82 nF, C41 680 pF, slope C38 390 pF, soft start C39 100 nF are start values. (T)
* Under-voltage lock-out: R34 249 k over R35 29.4 k turns it on at about 11.55 V (1.22 V typical `EN/UVLO` threshold). R38 (93.1 k) sets the hiccup mode.
* Q6 to Q9 are the four DMT6007LFG switches, D2 and D3 with C45 and C46 are the two bootstraps.

### Test points, LEDs and mounting holes
* Test points (2.0 mm through-hole pads, as on the mainboard): TP1 GND, TP2 `PSU_IN`, TP3 `BUS`, TP4 `PACKP` (pack +), TP5 `VSS` (pack minus, so the drop across R69 can be read against GND), TP6 `+24V` (output to the mainboard).
* Power-on LED DS7 (5 mm green): `BUS` through R88 (10 k) to GND, about 2 mA. `BUS` is live from either the PSU or the pack, so it shows that the board is powered. It stays on, and on battery it draws about 2 mA.
* SW1 "SHOW LEDs" lights all the rail LEDs while it is pressed. The cathodes of DS1 to DS6 are joined, and SW1 connects that node to GND. Nothing is drawn while the button is up. Each LED has a series resistor from its rail:

| LED (0805 red) | Rail | Resistor |
|---|---|---|
| DS1 | `PSU_IN` | R82, 10 k |
| DS2 | `BUS` | R83, 10 k |
| DS3 | `PACKP` | R84, 10 k |
| DS4 | `+24V` | R85, 10 k |
| DS5 | `+5V` (from the mainboard, J4) | R86, 1 k |
| DS6 | `+3V3` (from the mainboard, J4) | R87, 470 R |

* The resistor values are start values (T). I did not check the LED brightness on the bench.
* H1 to H4 are four M3 mounting holes (`MountingHole_3.2mm_M3`), with no net. The schematic has no PCB yet, so their positions are not set.
* The rails reach this block through net labels (`PSU_IN`, `BUS`, `PACKP`, `VSS`, `+24V`); `+5V` and `+3V3` are power symbols.

## Bill of materials (grouped by value)

Counts come from the schematic. Resistors and capacitors are 0805 unless the footprint says otherwise.
Reference numbers have gaps: the 5 V, 12 V, 45 V boost and inverter parts moved to the mainboard, and the BQ7791501 parts were replaced.

| Parts | Value / part | Footprint |
|---|---|---|
| U1, U2 | LM74700QDBVRQ1 | SOT-23-6 |
| U3 | BQ40Z80RSMR | VQFN-32 4x4 |
| U4 | BQ24610RGER | VQFN-24 |
| U6 | LM5176PWPR | HTSSOP-28 |
| Q1, Q2, Q3, Q4, Q6, Q7, Q8, Q9 | DMT6007LFG | PowerDI3333-8 |
| Q5 | DMN6040SSD-13 (dual) | SOIC-8 |
| D1, D2, D3, D5, D6 | MBR0540 | SOD-123 |
| L1 | 6.8 uH Coilcraft XAL1010-682ME | XAL1010 |
| L3 | 15 uH Coilcraft XAL1010-153ME | XAL1010 |
| F1, F2 | 10 A fuse, 5x20 holder | fuse holder |
| J1, J2, J5 | 2-pin screw terminal | 5.00 mm |
| J3 | 1x9 pin header | 2.54 mm |
| J4 | 1x8 pin header | 2.54 mm |
| TP1 to TP6 | test point pad | 2.0 mm THT, 1.0 mm drill |
| DS1 to DS6 | LED, red | 0805 |
| DS7 | LED, green | 5 mm THT |
| SW1 | push button "SHOW LEDs" | 6 mm THT |
| H1 to H4 | M3 mounting hole | 3.2 mm |

Capacitors:

| Parts | Value |
|---|---|
| C1 C2 C3 C4 C18 C19 C20 C25 C26 C42 C44 C103 C104 | 100 nF 50 V X7R (13) |
| C39 C45 C46 C94 C95 C96 C97 C98 C100 C101 | 100 nF 25 V X7R (10) |
| C30 | 100 nF 25 V X7R, 5 % |
| C27 C28 | 1 uF 25 V X7R |
| C17 | 1 uF 50 V X7R |
| C43 | 1 uF 16 V X7R |
| C102 | 2.2 uF 50 V X7R |
| C6 C7 C8 C9 C47 C48 C49 C50 C51 C52 C53 | 10 uF 50 V X7R, 1210 (11) |
| C21 C22 | 10 uF 35 V X7R, 1210 |
| C23 C24 | 4.7 uF 50 V X7R, 1210 |
| C5 | 220 uF 35 V polymer, 8x10 |
| C54 | 100 uF 35 V polymer, 8x10 |
| C40 | 82 nF |
| C37 C38 C41 | 47 pF / 390 pF / 680 pF |
| C29 / C99 | 22 pF / 100 pF |

Resistors:

| Parts | Value |
|---|---|
| R69 | 2 m 1 % 1 W 50 ppm, 2512 |
| R29 | 15 m 1 % 1 W, 2512 |
| R18 | 100 m 1 % 0.5 W, 1206 |
| R17, R39 | 10 R |
| R30 R31 R64 R65 R66 R67 R68 R70 R71 R72 R73 R74 | 100 R (12) |
| R87 | 470 R |
| R86 | 1 k |
| R33 | 5.76 k |
| R25 | 9.31 k |
| R75, R77, R82, R83, R84, R85, R88 | 10 k |
| R37 | 10.7 k |
| R24 | 17.8 k |
| R79 R80 R81 | 20 k |
| R32 | 27.4 k |
| R35 | 29.4 k |
| R22 | 43.2 k |
| R38 | 93.1 k |
| R21 R23 | 100 k |
| R34 | 249 k |
| R36 | 309 k |
| R26 | 430 k |
| R76, R78 | 10 M |
| R19 | 909 k, 0.1 % |
| R20 | 102 k, 0.1 % |

The tolerance is the one written in the schematic value field. These resistors have no tolerance written yet: R17, R30, R31, R39, R64 to R68, R70 to R88. The off-board parts are the 5 cells (5S1P) and the two 10 k NTCs (103AT).

## Checks done

* ERC on a copy of this schematic with `fp-lib-table` and `battery.kicad_pro`: 0 violations.
* Netlist compared with the previous version: every kept part has the same connections; the changes are the BQ40Z80 block, the 8-pin J4 and the STAT nets.
* Layout of the schematic is auto-routed and not tidy; some text overlaps wires. It is electrically correct.

## Not verified, open concerns

* The BQ40Z80 register map (which command returns each cell voltage, capacity and state of charge) and its SMBus slave address are in the technical reference manual SLUUBT5, which I have not read.
* How the BQ40Z80 wakes from shutdown or deep under-voltage with the charger behind the discharge FET is not checked. It senses a charger on `PACK`; the BQ24610 charges through the body diode of Q4.
* The pre-charge path of the BQ40Z80 (`PCHG`) is not used. A deeply discharged pack is charged by the BQ24610 pre-charge current (R24, 49.9 mA).
* Q3 and Q4 are 60 V parts used because the project already uses them; the datasheet example uses 40 V and 60 V TI FETs. Gate-drive voltage is 10.5 to 12.5 V (datasheet), enough for DMT6007LFG.
* The mainboard on battery draws its 24 VAC inverter and 5 V from U6, which is rated 2 A. The mainboard load at 2 valves on battery is not measured.
* Loop-compensation values for U6 are start values. A bench step-load test is needed.
* Charge voltage is 4.16 V per cell typical, 4.19 V worst case, set by resistor values. The BQ40Z80 over-voltage limit is set in data flash and has to stay above that.
* Cell model is unknown, so allowed charge current and cut-off voltage have to be checked against the real cells. Pack runtime depends on that choice.
* J4 pin 1 (+5V from the mainboard) has no load on this board yet.

## Bench checks (before connecting the mainboard)

1. J5 at 2 A load on mains, and on battery at 15 V and 21 V bus.
2. Remove mains with the mainboard running 2 valves: the valves stay open and the MCU does not reset.
3. Charger: 0.5 A limit, stops at the set voltage, no cell above 4.2 V.
4. The BQ40Z80 answers on SMBus; compare its cell voltages, current and temperature with a meter.
5. Calibrate the gauge with bqStudio, then check the charge counter against a measured charge and discharge.
6. Runtime with 2 valves until the cell under-voltage cut-off.
