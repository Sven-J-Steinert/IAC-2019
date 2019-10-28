# Showcase for Satellite Routines
Developed for Telespazio VEGA. Presented on the International Astronautical Congress (October 2019 | Washington, D.C. , USA)<br>
This setup consists of 3 parts: The [Orbiter](#orbiter) that is circling around the [Groundstation](#groundstation) and the [Node](#node) that connects both to an internet endpoint. Sending commands to the Orbiter as well as pulling the Telemetry from it is only possible while it is in sight of view of the satellite dish. This creates a connected and disconnected timespan in the periodic circling.

# Orbiter
![alt text](https://abload.de/img/drawing_isometric-1kqkhj.png)
![alt text](https://abload.de/img/img_20191014_1454398wjwb.jpg)

## Orbiter Partlist
 
|QTY|	DESCRIPTION	|PART NAME|
| :---   | :---   | :---   |
|1|	DevBoard|	ESP32-CAM OV2640 |
|1|	AVR Microcontroller|	Atmel Attiny85|
|2|	Distance Sensor|VL53L0X 940nm laser|
|2|	Solar Cell|	5.5V 90mm x 30mm 65mA polycrystalline|
|1|	Battery|	Samsung INR18650-35E|
|1|	Votage Converter|	Pololu S9V11F3S5C3 |
|1|	Motor Driver| DRV8833 2-Channel H-Bridge|
|1|	DC Motor| Mini Gearmotor 4,5V 80mA 32rpm|
|1|	Ball-Bearing| L-0123|
|1|	PCB| 30mm x 70mm|
|1|	Switch| ON OFF Toggle|
|4|	Resistor| 10 kΩ|
|1| Case| 3D SLA Printed |
|2| Wheels| Lasercutted 3mm MDF, rubber coated |

## Orbiter Wiring

![alt text](https://abload.de/img/sjcjx55cc4ke6.png)

# Groundstation

## Groundstation Partlist
 
|QTY|	DESCRIPTION	|PART NAME|
| :---   | :---   | :---   |
|1|	DevBoard|	ESP8266 NodeMCU |
|1| Servo| MG90S metal gear |
|1|	Batteryholder | SMD 18650|
|1|	Battery|	Samsung INR18650-35E|
|1|	Votage Converter|	Pololu S9V11F3S5C3 |
|1| Case | 3D FDM Printed |
|1| Globe | Ø 14cm, white striped |

# Node

![alt text](https://abload.de/img/img_20191014_234754a8j5m.jpg)

## Node Partlist
 
|QTY|	DESCRIPTION	|PART NAME|
| :---   | :---   | :---   |
|1|	single-board Computer|	Raspberry Pi 3 B+ |
|1| Display| 5 inch Display with case |
|1|	Wifi Dongle | Aoyool WiFi Adapter, AP mode|
|1|	Powersupply |	WEYO 5V 3A |
|1|	micro sd card |	SanDisk 32Gb |
|1|	Wireless Keyboard  |	Rii i8 Mini |
