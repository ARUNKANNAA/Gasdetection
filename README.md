# Wi-Fi-Based Gas and Smoke Monitoring System

An IoT-based monitoring prototype using **Wemos D1 Mini (ESP8266)**, MQ-2, and DHT11 sensors to detect gas and smoke levels, monitor temperature and humidity, and provide alerts through a buzzer and a Wi-Fi web dashboard.

## Features

- Gas and smoke monitoring using MQ-2 analog output
- Temperature and humidity monitoring using DHT11
- Automatic buzzer alerts when gas readings exceed a threshold
- Real-time monitoring through a web dashboard
- Built-in Wi-Fi access point; no internet required
- Remote buzzer mute and enable control

## Hardware Requirements

- Wemos D1 Mini (ESP8266)
- MQ-2 gas and smoke sensor module
- DHT11 temperature and humidity sensor
- Active buzzer module
- Jumper wires and suitable power supply

## Pin Connections

| Component | Wemos D1 Mini |
|---|---|
| MQ-2 AO | A0 |
| DHT11 DATA | D2 |
| Buzzer IN | D5 |
| Sensor VCC | Suitable supply voltage |
| GND | Common GND |

**Important:** Check your specific D1 Mini board's A0 input limit before connecting the MQ-2 analog output. The ESP8266 chip ADC is limited to 1.0 V, although many D1 Mini boards include an onboard voltage divider. Never assume the input is 5 V tolerant.

## Software Requirements

- Arduino IDE
- ESP8266 board package
- Adafruit DHT sensor library
- Adafruit Unified Sensor library

## Setup Instructions

1. Install Arduino IDE and the ESP8266 board package.
2. Install the required DHT libraries.
3. Connect the sensors and buzzer according to the pin table.
4. Select the appropriate Wemos D1 Mini board in Arduino IDE.
5. Upload the project firmware.
6. Connect your phone or computer to the `GasMonitor` Wi-Fi network.
7. Enter the password `GasSafe123`.
8. Open `http://192.168.4.1` in a web browser.

## How It Works

The MQ-2 sensor continuously provides analog readings to the ESP8266. The firmware compares these readings with a configured threshold and activates the buzzer when the threshold is exceeded. Temperature and humidity are measured using the DHT11. A locally hosted web dashboard displays sensor readings, alert status, and buzzer controls.

## Applications

- IoT learning and experimentation
- Electronics workshops and laboratories
- Gas and smoke monitoring demonstrations
- Embedded systems and web dashboard projects

## Safety Disclaimer

This project is an educational prototype, not a certified safety device. MQ-2 readings require warm-up and calibration and do not directly represent accurate gas concentrations in ppm. Do not rely on this project for life-safety protection or as a replacement for certified gas detectors and fire alarms.
