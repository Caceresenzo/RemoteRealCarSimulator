# Remote "Read" Car "Simulator"

#### Project Presentation

RC Car Controled by Arduino with FPV 3D Camera

![RC Car](https://i.imgur.com/fpKpMUs.jpg)

#### Story

We are a small team of 3 student that make a school project
The goal was to take a classic RC car, but control it with a steering wheel and pedals over radio signals. In addition, we paired the car with an FPV camera (normally designed for drones) and with his headset to view the video stream of the car.

#### Hardware

##### Materials used (car side)

- Arduino Leonardo
- x1 Motor for direction control
- x2 Motor for Camera axes (X, Y)
- x1 High Frequency Module (433 MHz)
- x1 Gyroscope / Accelerometer 
- x1 Motor sized 1/5
- x2 12V Batteries 

##### Materials used (head / controller side)

- Arduino Uno
- x1 High Frequency Module (433 MHz)
- x1 Gyroscope / Accelerometer 

#### Installing

##### IDE

- Arduino IDE ([website](https://www.arduino.cc/en/main/software))
- Eclipse Arduino Plugin Sloeber V4 ([github](https://github.com/Sloeber/arduino-eclipse-plugin))

##### Libraries

- Grove LoRa 433MHz & 915MHz RF ([forum](http://wiki.seeedstudio.com/Grove_LoRa_Radio/#download-library))
- I2C DEV ([github](https://github.com/jrowberg/i2cdevlib))
- MPU6050 ([github](https://github.com/jarzebski/Arduino-MPU6050))
- SoftwareSerial
- SPI
- Wire

#### Software & Programming

##### Side
- [branch:car](https://github.com/Caceresenzo/RemoteRealCarSimulator/tree/car) (controller)
- [branch:head](https://github.com/Caceresenzo/RemoteRealCarSimulator/tree/head)

#### Authors

- **Caceres Enzo** ([@Caceresenzo](https://github.com/Caceresenzo/)) - Programming work with arduino
- **Dorian Hardy** ([@thegostisdead](https://github.com/thegostisdead/)) - Code commenting and overall helper
- **Jason Trindade** - Mecanicals, technical and electics parts