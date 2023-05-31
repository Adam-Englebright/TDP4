# Paste Application Control: Firmware for the team 2 Raspberry Pi Pico board

### Installation

This project is built using the [PlatformIO](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) extension for [VSCode](https://code.visualstudio.com/).

The `wizio-pico` platform from Georgi Angelov is used via PlatformIO, to make use of the official Pico C/C++ SDK. Unfortunately, Georgi has discontinued this platform and deleted its GitHub repository, but a backup can be found here: https://github.com/maxgerhardt/wizio-pico. Please follow the instructions there for installation.

Please note that this software was built using the original `wizio-pico` from Georgi Angelov, not the backup. As a result, the `platformio.ini` *may* need modification to work with the backup version, but it should probably be fine.

### Building & Flashing

Uncomment and change the `upload_port` field in `platformio.ini` to the Serial port the Raspberry Pi Pico connects to on your machine.

In the PlatformIO CLI shell:

```sh
$ pio run -t upload -e pico-dap
```

If needed, change `pico-dap` to whatever the environment name used in `platformio.ini` is. In this repository `[env:pico-dap]` is used.

Alternatively, click the upload button (depicted as a right facing arrow) on the bottom of the VSCode window.

### Usage

#### Connections

| GPIO Port Number | Usage | Purpose |
| --: | --- | --- |
| **0** | Digital Input | Detect input on the F button |
| **1** | Digital Input | Detect input on the B button |
| **2** | I2C1 SDA | Data line for master I2C controller |
| **3** | I2C1 SCL | Clock line for master I2C controller |
| **4** | I2C0 SDA | Data line for slave I2C controller |
| **5** | I2C0 SCL | Clock line for slave I2C controller |
| **13** | Digital Input | Detect input on the I2C button |
| **14** | Digital Output | Controlling LED 2 |
| **15** | Digital Output | Controlling LED 1 |
| **16** | Digital Input | Detect input on the MISC button |
| **17** | PWM Input | Count PWM cycles from GPIO 19 |
| **18** | Digital Output | Control direction pin on A4988 |
| **19** | PWM Output | Drive step pin on A4988 |
| **20** | Digital Output | Control sleep pin on A4988 |
| **21** | Digital Output | Control reset pin on A4988 |
| **22** | Digital Output | Control MS3 pin on A4988 |
| **26** | Digital Output | Control MS2 pin on A4988 |
| **27** | Digital Output | Control MS1 pin on A4988 |
| **28** | Digital Output | Control enable pin on A4988 |

#### Serial Monitor

In the PlatformIO CLI shell:

```sh
$ pio device monitor -p {PORT_NAME}
```