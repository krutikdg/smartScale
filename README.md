# smartScale

A handheld tool that shows how level a surface is and measures distance with lasers on both sides. An IMU gives the tilt, two time-of-flight sensors give the distances, and the span between two walls is the two readings plus the width of the device. Readings appear on a small OLED, and one button cycles through the modes.
I built it to learn how sensors talk over I2C and how to combine accelerometer and gyro data.

## Built with

- Seeed Studio XIAO ESP32-C3
- MPU9250 (accelerometer and gyro for tilt)
- 2x VL53L1X time-of-flight distance sensors
- 2x KY-008 laser modules (visible aim dot)
- 0.96" SSD1306 OLED display
- 3.7 V LiPo battery
- Arduino IDE (C++)
- Fusion 360 (case design)
- PLA, 3D printed

## Bill of Materials

| # | Component | Qty | Price/Unit (USD) | Purpose | Purchase Link |
|---|-----------|-----|------------------|---------|---------------|
| 1 | Seeed Studio XIAO ESP32-C3 | 1 | 4.73 | Main controller with built-in LiPo charging | [Link](https://robu.in/product/seeed-studio-xiao-esp32c3-tiny-mcu-board-with-wi-fi-and-ble-battery-charge-supported-power-efficiency-and-rich-interface/) |
| 2 | MPU9250 9-Axis Module | 1 | 23.26 | Accelerometer and gyroscope for tilt sensing (magnetometer unused) | [Link](https://robu.in/product/mpu9250-9-axis-attitude-gyro-accelerator-magnetometer-sensor-module-2/) |
| 3 | PIMORONI VL53L1X Time of Flight (ToF) Sensor Breakout | 2 | 16.83 | Laser distance sensing on each side | [Link](https://robu.in/product/pimoroni-vl53l1x-time-of-flight-tof-sensor-breakout/) |
| 4 | KY-008 650nm Laser Module | 2 | 1.05 | Visible aim dot for each distance sensor | [Link](https://www.amazon.in/JIVITH-KY-008-Transmitter-Microcontroller-Electronics/dp/B0HF4XXZ2Y/ref=sr_1_1?dib=eyJ2IjoiMSJ9.yfmMPcdk0BHq5JQM1wyHHw1pQBCNZZwQGMGGo66YAPuLUr4x1N7K3bsNWZ7n1syKIDTw-uKheJ6NpLwDyxtFmkJAPyth-NKqMD3FKtM6D5JBDxccUeeYmyGBn7Re83Xd8D6rCDimV3Hd_mEnh4t4PNN0GtageTsfUXYm2aVnE_-WCN9J4Jnhy_mpXvSrnsUFUp0_KNP_xbKG_qL2yF5SaE3bqgikVF1zkGiAJBfzLv2x32AqxcKrxoucp5xPUEsyI_P9bAZEZBvXKxABk2FkyBuefgDvqQz2uo9t2MzjBvE.BPExnz-N4HXVQpnSDje26nJrYC8ex7yLlPApVpjd6JU&dib_tag=se&keywords=KY-008+laser+module&qid=1793504930&sr=8-1) |
| 5 | 0.96 Inch I2C OLED Display Module (SSD1306) | 1 | 3.36 | Shows the readings | [Link](https://robu.in/product/0-96-inch-i2c-iic-4pin-oled-display-module-blue-with-green-mark-module/) |
| 6 | 6x6x5mm Tactile Push Button Switch (Pack of 10) | 1 | 0.18 | Switches between modes | [Link](https://robu.in/product/6x6x5-tactile-push-button-switch/) |
| 7 | Slide Switch SS-12D00 1P2T | 1 | 0.02 | Main power on/off switch | [Link](https://robu.in/product/1-month-warranty-255/) |
| 8 | 3.7V 250mAh LiPo Battery (502030) | 1 | 1.33 | Rechargeable power source | [Link](https://www.amazon.in/SP-Li-Polymer-Rechargeable-APL502030-Electronics/dp/B0H6G4TZLL/ref=sr_1_1?dib=eyJ2IjoiMSJ9.mpydqNQ36lE-XLuZvouShaKKooty6OmCCTzhF3EnIB8mwSDNXpV82DdiTZe5xEvDZ6Z3GqOvDbpDpO-Zfr-tDI3Z90VkaY3VkBi5hMPzTRdqedDkvDSX4PRb0nAZeVUebpKYdYkLep4xYQGW0s7UXhDayV3t6MlP6IKmp02EA8d_5ly05Tx2cA8rnE3mEkekWo2OaISMxvmW3f8Qe3bSIdezTiFBJJS72FR7eHm2CeA.lPdF8uNxKP0z7kdFXU1ZHyfX1mboi7IIORvMjX64hfQ&dib_tag=se&keywords=3.7V+250mAh+502030+lipo+battery&qid=1793505325&sr=8-1) |
| 9 | BC547 NPN Transistor (Pack of 20) | 1 | 0.41 | Switches the laser modules on and off | [Link](https://robu.in/product/bc547-npn-transistor-pack-of-20/) |
| 10 | 1K and 100K Ohm 1/4W Resistors (Pack of 10 each) | 2 | 0.13 | Laser switch base resistors and battery-sense divider | [Link](https://robocraze.com/collections/resistors) |
| 11 | Ceramic Capacitor Assorted Kit (incl. 0.1uF) | 1 | 1.80 | Decoupling for sensors and battery sense | [Link](https://robu.in/product/ceramic-capacitor-assorted-kit-30-kinds-from-2pf-0-1uf/) |
| 12 | 10uF 16V Electrolytic Capacitor (Pack of 10) | 1 | 1 | Smooths the 3.3V rail | [Link](https://www.amazon.in/AVS-COMPONENTS-Radial-Electrolytic-Capacitor/dp/B09GB29NMK/ref=sr_1_3?dib=eyJ2IjoiMSJ9.66sHmPgpbOrPzdQeNkveuYg_vkXovJ09cGvlsJR2HhJ1HQYIzV26KY1leHhgw6mCRXE1SF9kxkBGrxiaJKui80mX54Q4Jke40j2L97GZgCimgqIkKAPeikHOt3CdcOpbJQog5Xwn7Qwn8nKSkx-cFytBxTHp9cJHYDD2h1zabiw6i8rppjnfzdVLmqN2xMOoT_CK8CMfSdlrBj_eIlAdyIKAJFQgKfKM4QA4DpyHFMY.okDGP74U4wugJxp2kPhOzxuwUU3tiKMHRetVpIZrZKc&dib_tag=se&keywords=10uF+16V+electrolytic+capacitor&qid=1793505503&sr=8-3) |
| 13 | 7x9 cm Universal PCB Prototype Board (Single Sided) | 1 | 0.39 | Mounts the laser switching and divider circuit | [Link](https://robu.in/product/single-side-prototype-paper-copper-pcb-universal-experiment-matrix-circuit-board-7x9cm/) |
| 14 | Majasavi M2 Phillips Cross Flathead Black Micro Screw - 25 pcs pack | 1 | 2.63 | Closes the case and mounts the boards | [Link](https://www.amazon.in/Majasavi-Phillips-Cross-Flathead-Black/dp/B0CKFMHC6T/ref=sr_1_2_sspa?dib=eyJ2IjoiMSJ9.3imsYWdIRufrs8d-Uy_8gIiaKLUqtiwcEOIGGF2XVju3YJr5y5JmiE3tNTORJJozsUezWZ6afLE7Yk--HBGozQs57Z9wE5hyBXjlbgXpTPbZMHpiSWChot5q40Ib_6GNvP_87Z-yKlvzio8Up7X34bQR2UuuosMdsbyqjq6j5vKU3LTTpROXRNGtbFMS3R-eBez4kUoih96P2t-nM6Y-MZEXQIExUzD1UUhzBT9TJJOSl8BEn8EABO3RUrkeanHbFoQVjCRGhDjKwYdz0quoxijbrdun5ziHBimajHMLlhU.seGkL64A2k2Scdt23UtNcJzceHIcfXEfbhxJ44-0GU0&dib_tag=se&keywords=M2+self+tapping+screw+assortment+kit&qid=1793505686&sr=8-2-spons&aref=fuGGxXUkF9&sp_csd=d2lkZ2V0TmFtZT1zcF9hdGY&psc=1) |
| 15 | Male to Female Jumper Wires 40pcs 20cm | 1 | 0.46 | Prototyping connections between boards | [Link](https://robu.in/product/male-to-female-jumper-wires-40pcs-20cm/) |
| 16 | 3-in-1 Soldering Iron Kit (25W iron + solder + flux) | 1 | 1.68 | Making permanent connections once the circuit is final | [Link](https://www.amazon.in/gp/product/B07W32JT5N/ref=ox_sc_act_title_1?smid=A3LR6E2CIRD8HG&psc=1) |
| 17 | eSUN PLA Matte Deep Black 1.75mm Filament | 1 | 12.09 | 3D printing the case | [Link](https://robu.in/product/esun-epla-gloss-1-75mm-3d-printing-filament-1kg/) |
| | **Total** | | **89.36** | | |

## Circuit Diagram

## Pin Map

| Function | XIAO ESP32-C3 Pin | GPIO |
|----------|-------------------|------|
| I2C SDA | D4 | 6 |
| I2C SCL | D5 | 7 |
| VL53L1X A (XSHUT) | D10 | 10 |
| VL53L1X B (XSHUT) | D2 | 4 |
| Laser A switch | D3 | 5 |
| Laser B switch | D6 | 21 |
| Mode button | D7 | 20 |
| Battery voltage (ADC) | D1 | 3 |

## Build steps

1. Print the base (`base.stl`) and lid (`lid.stl`) from `/CAD`.
2. Test each module on a breadboard before wiring anything permanently: the OLED first, then the MPU9250 (`WHO_AM_I` should read `0x71`), then each VL53L1X, then each laser through its transistor.
3. Wire everything following the pin map above.
4. Solder the laser switching circuit (2x BC547, 2x 1 kΩ) and the battery divider (2x 100 kΩ, 100 nF) onto the prototype board.
5. Fit the parts into the case: battery, XIAO, MPU9250, OLED, ToF sensors, lasers, button and slide switch.
6. Mount the MPU9250 **rigidly** and flat, and note its orientation. A loose sensor gives bad angle data.
7. Install the board package and libraries, select `XIAO_ESP32C3` with USB CDC On Boot set to Enabled, and flash the firmware.
8. Close the case with the screws.

## Calibration

1. **Gyro:** it calibrates itself at every power-on, so keep the device still on the bench for the first couple of seconds.
2. **Tilt direction:** tilt the device left/right and forward/back. If the angle on the OLED moves the wrong way, flip `PITCH_SIGN` or `ROLL_SIGN` in the code (`1` or `-1`).
3. **Level zero:** set the device on a surface you know is level (a spirit level or a phone level app will do). In LEVEL mode, long-press the button to store the zero offset. It is saved to flash, so it survives power-off.
4. **Distance:** point each sensor at a flat wall at a known distance, measured with a tape (500 mm and 1000 mm, for example). If the reading is off by a constant amount, adjust the offset. Repeat on both sensors.
5. **Span length:** measure the distance between the two sensor faces on the finished device and set `SENSOR_SPAN_MM` to it (default 100 mm). Check it against a known length, such as a door frame.
6. **Battery:** compare the OLED battery percentage with a multimeter reading across the battery. Adjust `BATT_DIV_RATIO` until they agree, and tune `BATT_EMPTY_V` and `BATT_FULL_V` if needed.
7. **Level tolerance:** `LEVEL_TOL_DEG` (default 0.5°) sets when the display shows "level". Tighten or loosen it to suit you.

## Safety

The KY-008 is rated 5 V / 5 mW and runs here from 3.3 V. Never look into the beam or point it at anyone's eyes, and keep it away from reflective surfaces.

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| Board not detected by the computer | Charge-only USB cable. Try a data cable |
| No serial output | `USB CDC On Boot` is not set to Enabled in the Arduino IDE |
| OLED stays blank | SDA/SCL swapped or loose, or the display is not at address `0x3C` |
| MPU9250 not found | The firmware expects `WHO_AM_I = 0x71`. Some clone chips report a different ID |
| Only one ToF sensor works | XSHUT wiring on D10 / D2 is wrong, so both sensors still answer at the default address `0x29` |
| Distance reads max or invalid | Target is beyond the 4000 mm limit set in the firmware (`TOF_MAX_MM`) |
| Tilt angle moves the wrong way | Flip `PITCH_SIGN` or `ROLL_SIGN` in the code |
| Angle drifts after power-on | The device was moving during the gyro calibration at boot. Power-cycle it while it sits still |
| "Level" never shows | Zero offset not set (long-press in LEVEL mode on a level surface), or `LEVEL_TOL_DEG` is too tight |
| Battery percentage is wrong | Divider values or the `BATT_DIV_RATIO`, `BATT_EMPTY_V`, `BATT_FULL_V` constants need adjusting against a multimeter reading |
| Laser does not light | BC547 wired backwards or the base resistor is missing, or the laser is wired to the wrong side of the transistor |
| Device turns itself off | Idle sleep after 60 s with no input. Press the button to wake it |