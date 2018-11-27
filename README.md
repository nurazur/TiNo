# TiNo
[German Version](https://github.com/nurazur/TiNo/blob/master/LIESMICH.md)

![looks like an old fashioned radio, but actually is not bigger than a matchbox](https://github.com/nurazur/TiNo/blob/master/P1010040.jpg)

"TIny NOde" :battery powered wireless sensor or wireless actor. 
Target of the project is the development of small size , cost effective battery powered wireless sensors. The sensors communicate with gateways, like a raspberry pi. 
The targets are:
 
- low cost (BOM under 5 Euro)
- very small size (matchbox)
- extremely low sleep current
- long battery life time (5 years and more on a CR2032 cell)
- long range (what ever this means :-), but its realy long)
- simple to build up
- Plug&Play Firmware
   
Sensors can be almost any, like temperature, relative humidity, air pressure, altitude meter, light intensity, UV Index, 
movement detectors, Reed switches, etc. However sensors have to be specified to work down to 2.2V. Otherwise the charge of the battery can not be fully used.

PCBs fit into low cost PVC boxes with the size of a matchbox that are readily available on the market.  
    

# Features
## General
- Voltage from ca. 1.8V to 3.6V
- Operates with a CR2032 cell up to 5 years
- various PCBs fitting into selected PVC boxes 
    
    
## Radio
- RFM69CW, RFM69HCW, RFM95 Module
- bidirectional communication
- ISM Band (Europe: 433MHz / 868MHz, US:315MHz / 915Mhz)
- 2GFSK Modulation
- Center frequeny adjustable
- Frequency correction can be calibrated
- Transmit power from -18 dBm to 20dBm, 10 dBm typ.
- Link budget up to 120dB
- sensitivity -105 dBm typ. 
- range t.b.d., but its looong!
- RF communication encrypted
- FEC (Forward Error Correction)
- Interleaver
    
## Baseband
- Atmel (Microchip) ATMega328p-au
- 32kByte Flash
- sleep current < 2µA mit external crystal 32.768kHz
- sleep current ca. 4µA with internal RC oscillator
- 1 MHz clock allows operating voltage down to 1.8V
- 8 MHz clock for gateway receiver
- I2C bus
- min 4 additional GPIO
    
## Sensors
- HTU21D
- SHT21, SHT20, SHT25
- SHT30, SHT31, SHT35
- I2C Bus based sensors can easily be integrated
- 4 digitale GPIOs
    
## System / Software
- Open Source Software C++
- Software can easily be adopted individually
- Programming tool is Arduino IDE
- Configuration of Nodes via serial interface (FTDA Adapter)
- configuration data and calibration data reside in EEPROM
- EEPROM encrypted 
- Flashing
  - using ISP Adapter or 
  - serial with FTDA Adapter and Bootloader
- up to 4 external interrupts (i.e. push buttons) can be configured
