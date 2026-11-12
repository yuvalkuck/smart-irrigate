# Heat and Environment

[← Back to the main README](../README.md)

Summer here can reach 36 C ambient with humidity as low as 30%. A box in direct sun can reach 55-65 C inside, so the boxes and components must stand that load. The ratings below are typical datasheet figures; check them against the exact parts used.

| Part | Typical limit | Heat risk | Action |
|---|---|---|---|
| Lead-acid batteries | Best at 20-25 C | Life roughly halves for every 8-10 C above that | Shaded, ventilated battery box, never in the sun |
| ESP32-C6 and ESP32-S3 modules | About 85 C | Low, but Wi-Fi activity adds a few degrees | Ventilate the main box |
| PSU and converters | Often rated 40-50 C at full load | They derate, and run hot in a closed box | Size at 50% load or less, keep them away from other heat sources |
| 8-channel relay board | About 70-85 C | Coils add heat, and contacts age faster when hot | Mount low in the box with airflow |
| LCD2004 | About 0-50 C, some -20 to 70 C | Washes out or fails first | Keep it in the shade, and off when not in use |
| SHT41, BMP581, DS18B20 | 80-125 C or more | Wrong readings, not damage | Radiation shield, away from walls |

Low humidity is easy on the electronics, but dry air raises the static risk, so ground the metal parts and avoid plastic rubbing against plastic near the boards. Condensation can still form when the boxes cool at night.

**Measures**
* Light-coloured enclosures in the shade, or a sun shield about 5 cm above each box.
* Vents with insect and dust filters, and a small fan if the main box still passes about 50 C.
* A temperature sensor inside the main box, so the firmware can warn and reduce load.
* A heat test during the live run (see "Project Progress Plan").
