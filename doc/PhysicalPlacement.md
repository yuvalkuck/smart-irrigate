# Physical Placement: Separate Enclosures

[← Back to the main README](../README.md)

The components do not share one box. Placement is decided by what each part can survive outdoors (heat, rain, cold), and by what it needs to measure correctly. Parts that are not weatherproof go in a protected box, parts that must be outside to measure correctly get their own housing, and parts that are weatherproof as bought are mounted outside with no box.

| Box | Contents | Location | Housing and notes |
|---|---|---|---|
| **Main box** | Mainboard (ESP32-C6, ESP32-S3), 8-channel relay board, 27.6 V PSU, buck converter to 5 V, 24 VAC converter, fuses | Indoors or a sheltered spot | Sealed. 230 V side kept apart from the low-voltage side. |
| **Battery box** | 2 x 12 V batteries (series) | Next to the main box | Vented to open air, shaded and cool. Short fused cable to the main box through a gland. Lead-acid gives off hydrogen when charging, so keep it out of the main box. |
| **Sensor mast** | Platform Sensors board, SHT41 and BMP581 (ventilated radiation shield) | Outdoors, at least 1 m above the roof, clear sky view | Housings at least 30 cm apart. SHT41 and BMP581 each on a short I2C cable to the Platform Sensors board (J2, J7), which talks to the main box over RS-485. SHT41 in shade with free airflow. |
| **Roof mast (no box)** | Wind sensor | Roof, 1-2 m above the ridge, clear of the other housings | Weatherproof as bought. Pole mounted, level, with a drip loop in the cable. |
| **Soil (no box)** | DS18B20 | Irrigated zone, 5-10 cm deep | Waterproof probe in a sealed sleeve. Strain relief at the surface. |
| **Water line (no box)** | XDB401 | Tee near the manifold | Threaded fitting, upright or sideways. Add an isolation valve. |

## Connection overview

```mermaid
graph LR
    MAINS[230 V mains]

    subgraph MAIN["Main box"]
        PSU[27.6 V PSU]
        ADS[ADS1115]
        BUCK5[Buck 24 V to 5 V]
        CONV[24 VAC converter]
        C6[ESP32-C6]
        S3[ESP32-S3]
        INA[INA226]
        RELAY[8-channel relay board]
    end

    subgraph BATBOX["Battery box"]
        BAT[2 x 12 V batteries]
    end

    subgraph MAST["Sensor mast"]
        CLUSTER[Platform Sensors board]
        SHT[SHT41]
        BMP[BMP581]
    end

    subgraph ROOF["Roof mast"]
        WIND[Wind sensor]
    end

    subgraph SOIL["Soil"]
        DS[DS18B20]
    end

    subgraph PIPE["Water line"]
        XDB[XDB401]
    end

    subgraph MANIFOLD["Valve manifold"]
        VALVES[8 x Galcon 24 VAC valves]
    end

    MAINS --> PSU
    PSU -- 24 V bus --> BUCK5
    PSU -- 24 V bus --> CONV
    BAT <-- "fused cable, 24 V bus" --> PSU
    BUCK5 -- 5 V --> C6
    BUCK5 -- 5 V --> S3
    BUCK5 -- 5 V --> RELAY
    C6 <-- "UART GPIO 6 and 7" --> S3
    C6 <-- "RS-485 GPIO 8, 9 and 15" --> CLUSTER
    CLUSTER <-- "I2C, J2" --> SHT
    CLUSTER <-- "I2C, J7" --> BMP
    C6 <-- "I2C GPIO 19 and 20" --> ADS
    C6 <-- "I2C GPIO 19 and 20" --> INA
    WIND -- "analog 0-5 V" --> ADS
    XDB -- "analog" --> ADS
    BAT -- "bus voltage and shunt current" --> INA
    DS <-- "1-Wire GPIO 4" --> C6
    C6 -- "valve GPIOs 1 14 18 21 22 16 17 5" --> RELAY
    CONV -- "24 VAC, 50 Hz" --> RELAY
    RELAY -- "24 VAC, 9 conductors: 1 common + 8" --> VALVES
```

The 24 V bus carries power from the PSU to the battery, and from the battery to the loads during an outage (see "Power Architecture"). The relay board switches the 24 VAC from the converter to each valve.
