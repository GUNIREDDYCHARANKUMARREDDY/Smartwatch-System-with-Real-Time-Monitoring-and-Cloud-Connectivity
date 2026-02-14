# Multi-Sensor-Smartwatch-with-Real-Time-Monitoring-Through-Cloud



## Overview

The Multi-Sensor Smartwatch is a wearable embedded system designed to monitor health, environmental, and location parameters in real time. The system collects data from multiple sensors and uploads it to the cloud, allowing users to monitor the data remotely.

This project integrates health monitoring, environmental sensing, location tracking, and cloud connectivity into a single smartwatch system.

---

## Features

- Real-time heart rate monitoring (BPM)
- Blood oxygen level (SpO₂) monitoring
- Temperature monitoring
- Atmospheric pressure monitoring
- Compass direction tracking
- GPS location tracking
- Real-time cloud data upload
- Local display of all sensor data
- Menu-based user interface
- Remote monitoring through cloud

---

## System Architecture
   
![Flowchart](images/flowchart.png)

---

The system consists of a controller connected to multiple sensors, display, GPS module, and cloud connectivity. The controller collects data from sensors, processes it, displays it on the screen, and uploads it to the cloud.

---

### I2C Protocol (Inter-Integrated Circuit)

I2C is used to connect multiple sensors using only two wires.

I2C Lines:
- SDA (Serial Data Line)
- SCL (Serial Clock Line)

Sensors connected using I2C:

- MAX30100 – Heart rate and SpO₂ sensor
- BMP280 – Temperature and pressure sensor
- BMC5883L – Compass sensor
- DS1307 – Real-time clock module

## I2C Bus Pin Configuration (Parallel Connection Architecture)

| ESP32 Pin | Signal | MAX30100 (Heart Rate & SpO₂) | BMP280 (Temp & Pressure) | BMC5883L (Compass) | DS1307 (RTC) | Description |
|-----------|--------|-------------------------------|----------------------------|--------------------|--------------|-------------|
| 3.3V      | VCC    | VCC                           | VCC                        | VCC                | VCC          | Power supply to all sensors |
| GND       | GND    | GND                           | GND                        | GND                | GND          | Common ground |
| GPIO 21   | SDA    | SDA                           | SDA                        | SDA                | SDA          | I2C Data Line (shared) |
| GPIO 22   | SCL    | SCL                           | SCL                        | SCL                | SCL          | I2C Clock Line (shared) |

**Protocol:** I2C (Inter-Integrated Circuit)  
**Configuration:** Parallel shared bus architecture  
**Master Device:** ESP32  
**Slave Devices:** MAX30100, BMP280, BMC5883L, DS1307 


All sensors are connected in parallel using the I2C communication protocol. The ESP32 acts as the master, and all sensors act as slave devices sharing the same SDA and SCL lines. Pull-up resistors are used to ensure proper communication.

---

### SPI Protocol (Serial Peripheral Interface) – TFT Display

The TFT display uses SPI protocol for fast communication.

SPI Lines:

- MOSI – Data line
- SCLK – Clock line
- CS – Chip select
- DC – Data/Command
- RST – Reset

## SPI Bus Pin Configuration (TFT Display)


| ESP32 Pin | Signal | TFT Display Pin | Description |
|-----------|--------|------------------|-------------|
| GPIO 23   | MOSI   | SDA / MOSI      | Serial Data Line (Master → Display) |
| GPIO 18   | SCLK   | SCL / SCK       | Serial Clock Line |
| GPIO 5    | CS     | CS              | Chip Select (Enables display communication) |
| GPIO 2    | DC     | DC              | Data / Command Control |
| GPIO 4    | RST    | RESET           | Display Reset |
| 3.3V      | VCC    | VCC             | Power Supply |
| GND       | GND    | GND             | Ground |

**Protocol:** SPI (Serial Peripheral Interface)  
**Master Device:** ESP32  
**Slave Device:** TFT Display  

SPI protocol is used because it provides faster communication compared to I2C, which is required for updating graphics, text, and sensor data on the display in real time.



### UART Protocol – GPS Module

UART is used to communicate with the GPS module.

UART Lines:

- TX – Transmitter
- RX – Receiver

## UART Communication Pin Configuration (GPS Module)

| ESP32 Pin | Signal | GPS Module Pin | Description |
|-----------|--------|----------------|-------------|
| GPIO 17   | TX     | RX             | Transmits data from ESP32 to GPS |
| GPIO 16   | RX     | TX             | Receives data from GPS to ESP32 |
| 3.3V      | VCC    | VCC            | Power Supply |
| GND       | GND    | GND            | Ground |

**Protocol:** UART (Universal Asynchronous Receiver Transmitter)  
**Master Device:** ESP32  
**Slave Device:** GPS Module  

The GPS module continuously sends location data such as latitude and longitude to the ESP32 through UART communication. The ESP32 receives this data and processes it for display and cloud upload.


---

## Hardware Components


![Flowchart](images/flowchart.png)

---

- Controller unit (ESP32)
- Heart rate and SpO₂ sensor (MAX30100/MAX30102)
- Temperature and pressure sensor (BMP280)
- Compass sensor (HMC5883L)
- GPS module (NEO-6M)
- Real-time clock module (DS1307)
- Display module (ST7789 TFT)
- Push buttons
- Power supply

---

## Working Principle

1. The system initializes all sensors and modules.
2. Sensors collect real-time data continuously.
3. The controller processes the sensor data.
4. The data is displayed on the display.
5. The data is uploaded to the cloud.
6. Users can monitor the data remotely.
7. The process repeats continuously.

---

## Cloud Integration

The smartwatch uploads the following data to the cloud:

- Heart rate
- SpO₂ level
- Temperature
- Pressure
- Compass direction
- GPS location
- Time and date

This allows real-time remote monitoring.

---

## Applications

- Health monitoring
- Fitness tracking
- Patient monitoring
- Remote healthcare
- Personal safety
- Navigation assistance

---

## Advantages

- Real-time monitoring
- Remote cloud access
- Multi-sensor integration
- Portable design
- Easy to use

---




## Author  

Software Design: Siddharth Reddy

Hardware Design: GUNI REDDY CHARAN KUMAR REDDY
