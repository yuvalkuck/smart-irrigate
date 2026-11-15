# PSU board

Schematic: `psu.kicad_sch` (one sheet, all blocks wired pin to pin). The sheet carries only the wire-colour legend and the block titles.
All the notes are here. This folder has no PCB yet.

The board turns the output of an owned 24 V DC, 20 A supply (the only ready-made part) into every rail the irrigation
controller needs, and it can run from a 5-cell 18650 pack when mains is lost. Every other block is built from bare ICs on this board.

## Rails and sources

| Output | Connector | Value | Source |
|---|---|---|---|
| 5 V | J6 | 4.98 V typical | LMR51430 buck (U5), 3 A part |
| 24 V DC | J7 | 23.9 V typical, rated 2 A | LM5176 buck-boost (U6) |
| 24 V AC | J8 | sine, 50 Hz, about 24 V RMS | EG8010 (U9) + DRV8412 (U10) + LC filter |
| Pack charge | J2 | 20.82 V typical, 0.498 A | BQ24610 charger (U4) |

The inverter is on in both modes (mains and battery). Valve limit: 4 valves open at once on mains, 2 on battery. These limits are firmware, not hardware.

## How power flows

```
24 V PSU -- J1 -- F1 -- ideal diode U1/Q1 --+-- BUS (14 to 25 V)
                                             |
pack 5S -- J2 -- BMS U3 + Q3/Q4 -- F2 -- ideal diode U2/Q2 --+
                                             |
BUS --+-- U4 charger (21 V, 0.5 A) ----------> pack (J2)
      +-- U5 buck -------------------------> +5V (J6) and EG8010
      +-- U6 buck-boost -------------------> +24V (J7)
      +-- U7 LDO 12 V ---------------------> DRV8412 gate supply
      +-- U8 boost 45 V -------------------> DRV8412 PVDD -> filter L5/L6/C92 -> VAC_A/VAC_B (J8)
```

* The pack is 5S so that its full voltage (5 x 4.2 V = 21 V) stays below the 24 V supply. The two ideal diodes then switch sources by themselves, with no relay.
* BUS range is 14 V to 25 V. The lower limit is the BMS under-voltage cut (2.8 V x 5 cells = 14 V).
* The charger is fed from BUS (after U1), not from the PSU node. It has no power-path mode, so the pack never feeds the PSU node through the charger.
* On mains, the charger takes only the 0.5 A set by its sense resistor R18. The rails take the rest of their power from the PSU through the same bus.

## Connectors

| Ref | Pins | Use |
|---|---|---|
| J1 | 1 = 24 V in, 2 = GND | PSU input |
| J2 | 1 = pack B+ (`BP`), 2 = pack B- (`VSS`) | pack power |
| J3 | 1 = B-, 2 to 5 = cell taps 1 to 4, 6 = B+ | balance lead of the 5S pack |
| J4 | 1 = BMS `TS`, 2 = B- | 10 k NTC (103AT) on the pack, read by the BMS |
| J5 | 1 = charger `TS`, 2 = GND | 10 k NTC (103AT) on the pack, read by the charger |
| J6 | 1 = +5V, 2 = GND | 5 V out |
| J7 | 1 = +24V, 2 = GND | 24 V DC out |
| J8 | 1 = `VAC_A`, 2 = `VAC_B` | 24 V AC out |
| J9 | 1 = `MAINS_OK_N`, 2 = `INV_EN`, 3 = `INV_FAULT_N`, 4 = GND | signals to the mainboard |

Pack B- (`VSS`) is not the same net as GND: the BMS low-side FETs (Q3 discharge, Q4 charge) sit between them.

## Signals and firmware consequences

* `MAINS_OK_N` is the BQ24610 `PG` pin. Low means BUS is above the pack, which means the supply is present. High means battery.
  The schematic symbol and the BQ24610 datasheet mark `PG` as open drain, and the board has no pull-up, so the mainboard input needs a pull-up to 3.3 V.
  Reason for using `PG`: the PSU node decays too slowly to detect mains loss, and a plain voltage divider cannot separate 21.25 V (pack full) from 23.5 V (PSU minimum) without a comparator.
  If the pin sticks high, firmware sees "battery", which is the safe side.
* `INV_EN` (input, 3.3 V logic is enough) gates the whole inverter chain: TPS7A1601 enable, EG8010 `SPWMEN`, DRV8412 `RESET_AB`. R42 (100 k) pulls it low, so the inverter is off until firmware drives it high.
  The LM5155 boost enables itself from the 12 V rail, so it also stops.
* `INV_FAULT_N` is the DRV8412 `FAULT` pin (low = fault). The KiCad symbol marks it open collector and the board has no pull-up, so the mainboard needs a pull-up here too. `OTW` is not connected.
* Firmware items that follow from this (not changed here): a mains-detect input, a 2-valve limit while `MAINS_OK_N` is high, and an inverter-enable output.

## Block values

Source tags: D = value or part taken from the datasheet. C = computed with that datasheet's equation. T = start value, tune on the bench.

### Input and bus
* F1 is a 10 A 5x20 fuse at the input. U1/Q1 (PSU) and U2/Q2 (pack) are LM74700-Q1 ideal-diode controllers with DMT6007LFG FETs. C1 to C4 are the 100 nF charge-pump and anode capacitors (D).
* Bus capacitors: C5 220 uF polymer and C6 to C9, 4 x 10 uF.

### Battery and BMS (U3, BQ7791501)
* 5 cells, over-voltage 4.25 V, under-voltage 2.8 V, balancing inside the IC (D).
* R8 is the 5 m sense resistor. With it, the discharge trips are 7 A (OCD1) and 12 A (OCD2), the short-circuit trip is 24 A and the charge trip is 4 A. Balancing runs only while the pack charges above 0.375 A (D).
* R1 to R6 (33 R) with C10 to C14 (1 uF) filter the cell taps. R7 (1 k) with C15 filters the supply. R14 and R15 (1 M and 1.5 M) set the `PRES` level.
  `PRES` needs a pull-up to pack plus or the IC goes to HIBERNATE with both FETs off (D).
* Q3 and Q4 are the discharge and charge FETs in the common-port, low-side arrangement. R9 and R10 are the gate resistors, R11 and R12 hold the gates off.
* The NTC at J4 uses R16 (10 k) as the bias.
* F2 is the 10 A fuse in the pack plus lead.

### Charger (U4, BQ24610)
* Charge voltage: R19 (909 k) and R20 (102 k), both 0.1 %, give 20.82 V typical, which is 4.16 V per cell. The worst case with tolerances is 20.96 V (4.19 V per cell). (C)
* Charge current: R18 (100 m) with ISET1 at 1.00 V gives about 0.50 A (R21 100 k over R22 43.2 k set 0.498 A). (C)
* Termination and pre-charge: R23 100 k over R24 17.8 k set about 49.9 mA. (C)
* C30 (100 nF) on `TTC` sets the safety timer to about 9.3 h. (D)
* NTC window 0 to 45 C: R25 9.31 k, R26 430 k, NTC at J5. (D)
* L1 6.8 uH, Q5 (dual DMN6040SSD) as the two switches, D1 and C26 as the bootstrap.
* The power-path pins and the input-current regulation are not used. This follows the BQ24610 datasheet's typical application without power path (section 10.2.2).
* `PG` goes to `MAINS_OK_N`.

### 5 V (U5, LMR51430)
* R27 (100 k) and R28 (13.7 k) set 4.98 V. L2 6.8 uH. C35 and C36 are 2 x 22 uF output. `EN` is tied to BUS.

### 24 V DC (U6, LM5176)
* Input 14 V to 25 V, output 23.9 V (R36 309 k over R37 10.7 k), switching about 297 kHz (R32 27.4 k). L3 15 uH. (C)
* R29 is the 15 m sense resistor. The minimum boost current limit is about 6.7 A against a peak need of about 4.6 A at 2 A load. (C)
* Compensation R33 5.76 k, C40 82 nF, C41 680 pF, slope C38 390 pF, soft start C39 100 nF are start values. (T)
* Under-voltage lock-out: R34 249 k over R35 29.4 k turns it on at about 12.0 V. R38 (93.1 k) sets the hiccup mode.
* Q6 to Q9 are the four DMT6007LFG switches, D2 and D3 with C45 and C46 are the two bootstraps.

### 12 V (U7, TPS7A1601)
* R40 (1.15 M) over R41 (127 k) set about 12 V. C55 10 nF feed-forward, C56 input, C57 output.
* It powers the DRV8412 (`GVDD` and `VDD`, about 30 mA typical) and the LM5155 enable divider.
* Dissipation at 25 V in and 30 mA: about 0.39 W.

### 45 V boost (U8, LM5155)
* R50 (107 k) over R51 (2.43 k) set 45.0 V. About 298 kHz (R48 73.2 k). L4 15 uH. R45 is the 12 m sense resistor, R46 the slope resistor. (C)
* The 45 V is needed because 24 V RMS needs about 34 V peak across the bridge, with margin.
* Compensation R49 24.9 k, C62 39 nF, C63 220 pF are start values. (T)
* D4 PMEG060V100EPD is the boost diode. Q10 is the switch. Output capacitors: C66 and C67 (100 uF 63 V polymer) and C68 to C70 (4.7 uF 100 V).
* Enable: R52 (100 k) and R53 (30.1 k) divide the 12 V rail. The LM5155 bias comes from BUS through R43 (0 R) with C58.

### Inverter (U9 EG8010 and U10 DRV8412)
* EG8010: bipolar mode (`MODSEL` high), 50 Hz (`FRQSEL0` and `FRQSEL1` low), soft start on, 12 MHz crystal Y1 with C71 and C72 (22 pF). Dead-time pins low.
  Unipolar mode is not used: one half bridge would switch at 50 Hz, which the DRV8412 bootstrap does not support.
* Two SPWM outputs (`SPWMOUT1`, `SPWMOUT3`) go to `PWM_A` and `PWM_B` through 4.3 k over 10 k dividers (R54/R55 and R56/R57). A 5 V output becomes about 3.5 V there. The DRV8412 input needs at least 2 V and allows up to 3.6 V.
* DRV8412: `M1` to `M3` to AGND (mode 000). Bridge C and D inputs are held low and `RESET_CD` is low; only bridge A and B are used.
  R63 (36 k) sets `OC_ADJ` for about 7.4 A (tolerance about 20 %). R62 (1 R) joins GND and AGND. C77 on `VREG`. C78 to C81 on `GVDD`. C84 to C87 on `PVDD`. C88 to C91 are the bootstrap capacitors.
* Feedback: each output line has 47 k over 4.53 k with 100 nF (R58, R59, C75 and R60, R61, C76) into `VFB` and `FRQADJ/VFB2`. This gives about 24.1 V RMS. The EG8010 shuts down above +5 % and, after 3 s, below -8 %.
* Output filter: L5 and L6 (1 mH each, one in each line) and C92 (4.7 uF film) across the output. Corner about 1.64 kHz.

## Bill of materials (grouped by value)

Counts come from the schematic. Resistors and capacitors are 0805 unless the footprint says otherwise.

| Parts | Value / part | Footprint |
|---|---|---|
| U1, U2 | LM74700QDBVRQ1 | SOT-23-6 |
| U3 | BQ7791501PWR | TSSOP-24 |
| U4 | BQ24610RGER | VQFN-24 |
| U5 | LMR51430XDDCR | SOT-23-6 |
| U6 | LM5176PWPR | HTSSOP-28 |
| U7 | TPS7A1601DGNR | HVSSOP-8 |
| U8 | LM5155DSSR | WSON-12 |
| U9 | EG8010 | LQFP-32 |
| U10 | DRV8412DDW | HTSSOP-44 |
| Q1, Q2, Q3, Q4, Q6, Q7, Q8, Q9, Q10 | DMT6007LFG | PowerDI3333-8 |
| Q5 | DMN6040SSD-13 (dual) | SOIC-8 |
| D1, D2, D3 | MBR0540 | SOD-123 |
| D4 | PMEG060V100EPDZ | CFP15 |
| L1, L2 | 6.8 uH Coilcraft XAL1010-682MEB | XAL1010 |
| L3, L4 | 15 uH Coilcraft XAL1010-153ME | XAL1010 |
| L5, L6 | 1 mH, 3 A choke (part not chosen) | placeholder toroid footprint |
| Y1 | 12 MHz | 3225 |
| F1, F2 | 10 A fuse, 5x20 holder | fuse holder |
| J1, J2, J6, J7, J8 | 2-pin screw terminal | 5.00 mm |
| J3 | 6-pin JST XH | 2.50 mm |
| J4, J5 | 2-pin JST XH | 2.50 mm |
| J9 | 4-pin JST XH | 2.50 mm |

Capacitors:

| Parts | Value |
|---|---|
| C1 C2 C3 C4 C18 C19 C20 C25 C26 C34 C42 C44 C88 C89 C90 C91 | 100 nF 50 V X7R (16) |
| C31 C39 C45 C46 C73 C75 C76 | 100 nF 25 V X7R (7) |
| C84 C85 C86 C87 | 100 nF 100 V X7R (4) |
| C77 | 100 nF 16 V X7R |
| C30 | 100 nF 25 V X7R, 5 % |
| C10 C11 C12 C13 C14 C16 C27 C28 C59 C78 C79 C80 C81 C83 | 1 uF 25 V X7R (14) |
| C15 C17 | 1 uF 50 V X7R |
| C43 | 1 uF 16 V X7R |
| C6 C7 C8 C9 C47 C48 C49 C50 C51 C52 C53 C56 C64 C65 | 10 uF 50 V X7R, 1210 (14) |
| C21 C22 | 10 uF 35 V X7R, 1210 |
| C57 | 10 uF 25 V X7R, 1210 |
| C74 | 10 uF 16 V X7R |
| C23 C24 C32 C33 | 4.7 uF 50 V X7R, 1210 |
| C68 C69 C70 | 4.7 uF 100 V X7R, 1210 |
| C35 C36 | 22 uF 25 V X7R, 1210 |
| C82 | 47 uF 25 V X5R, 1210 |
| C5 | 220 uF 35 V polymer, 8x10 |
| C54 | 100 uF 35 V polymer, 8x10 |
| C66 C67 | 100 uF 63 V polymer, 10x10 |
| C92 | 4.7 uF 63 V film, through hole |
| C55, C58 | 10 nF (25 V, 50 V) |
| C61 / C62 / C40 | 220 nF / 39 nF / 82 nF |
| C37 C38 C41 C60 C63 | 47 pF / 390 pF / 680 pF / 100 pF / 220 pF |
| C29, C71, C72 | 22 pF (C71, C72 are C0G) |

Resistors:

| Parts | Value |
|---|---|
| R8 | 5 m 1 % 1 W, 2512 |
| R29 | 15 m 1 % 1 W, 2512 |
| R45 | 12 m 1 % 0.5 W, 1206 |
| R18 | 100 m 1 % 0.5 W, 1206 |
| R43, R44 | 0 R |
| R62 | 1 R |
| R17, R39 | 10 R |
| R1 R2 R3 R4 R5 R6 | 33 R |
| R30, R31, R47 | 100 R |
| R7, R10 | 1 k |
| R46 | 1.02 k |
| R51 | 2.43 k |
| R54, R56 | 4.3 k |
| R9, R59, R61 | 4.53 k |
| R33 | 5.76 k |
| R25 | 9.31 k |
| R16, R55, R57 | 10 k |
| R37 | 10.7 k |
| R24 | 17.8 k |
| R32 | 27.4 k |
| R35 | 29.4 k |
| R53 | 30.1 k |
| R63 | 36 k |
| R22 | 43.2 k |
| R58, R60 | 47 k |
| R48 | 73.2 k |
| R49 | 24.9 k |
| R38 | 93.1 k |
| R21 R23 R27 R42 R52 | 100 k |
| R50 | 107 k |
| R41 | 127 k |
| R28 | 13.7 k |
| R34 | 249 k |
| R36 | 309 k |
| R26 | 430 k |
| R13 | 470 k |
| R11, R12, R14 | 1 M |
| R15 | 1.5 M |
| R40 | 1.15 M |
| R19 | 909 k, 0.1 % |
| R20 | 102 k, 0.1 % |

The tolerance is the one written in the schematic value field. These resistors have no tolerance written yet: R1 to R7, R10 to R15, R17, R30, R31, R39, R42 to R44, R47 and R62. The off-board parts are the 5 cells (5S1P) and the two 10 k NTCs (103AT).

## Changes from the first plan

* The charger input is on BUS, not the PSU node (see above).
* Mains detect uses the BQ24610 `PG` pin, not a divider.
* The LM5155 is enabled from the 12 V rail, so one signal (`INV_EN`) stops the whole inverter chain.
* The bus lower limit is 14 V (not 15 V).
* One MOSFET type (DMT6007LFG, the part TI uses in the LM74700-Q1 datasheet) is used wherever a 60 V N-FET is needed. The four converter inductors use only two part numbers.
* There is no mains fuse on this board (no 220 V here). The 10 A input fuse F1 replaces it.
* The DRV8412 12 V load is 29.9 mA typical, and the EG8010 feedback gives about 24.1 V RMS. Both earlier figures were corrected after re-reading the datasheets.
* The charge sense resistor is 100 m (not 80 m).

## Checks done

* ERC on a copy of this schematic with `fp-lib-table` and `psu.kicad_pro`: 0 violations.
* The KiCad netlist exported from the schematic was compared with the design data (every pin of every part): 0 differences.
* Layout of the schematic is auto-routed and not tidy; some text overlaps wires. It is electrically correct.

## Not verified, open concerns

* PMEG060V100EPDZ (D4): its datasheet was not read. The part comes from TI's LM5155 table. Check it before buying.
* L5 and L6 (1 mH, 3 A): no part is chosen. A Bourns 2300LL-102V-RC was a candidate, but its saturation current was not read. The footprint in the schematic is a placeholder.
* Loop-compensation values for U6 and U8 are start values. A bench step-load test is needed.
* Output common mode: after the LC filter each field wire sits at about 22.5 V DC plus the AC around it, relative to GND. A wire-to-earth leak would carry DC, and it could cause corrosion on buried splices. Options: series non-polar capacitors in both output lines, or a 50 Hz isolating transformer. Not decided.
* Charge voltage is 4.16 V per cell typical, 4.19 V worst case, set by resistor values to stay under the BMS over-voltage trip (4.25 V). A 4.2 V typical target would reach 4.24 V worst case.
* With the inverter off on battery, the DRV8412 `PVDD` pins still see the bus through the boost diode. Datasheet idle current is 0.7 mA per half bridge, at most 2.8 mA total.
* Cell model is unknown, so allowed charge current and cut-off voltage have to be checked against the real cells. Pack runtime depends on that choice. The earlier estimate (41.8 Wh usable, 8.4 W with 2 valves, about 5 h) came from project-doc estimates, not measurements. A continuous 2 A on the 24 V DC rail while on battery would need a larger pack (about 5S4P).

## Bench checks (before connecting valves)

1. Each rail at full load on mains, and on battery at 15 V and 21 V bus.
2. 24 V AC on an oscilloscope with 1, 2 and 4 valves: sine, about 24 V RMS, 50 Hz, no clipping.
3. Remove mains with 2 valves open: the valves stay open and the MCU does not reset.
4. Charger: 0.5 A limit, stops at the set voltage, no cell above 4.2 V.
5. Runtime with 2 valves until the BMS cut-off. Compare the measured draw with the estimate.
