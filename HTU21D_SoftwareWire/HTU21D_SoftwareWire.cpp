/**************************************************************************/
/*
This is an Arduino library for HTU21D, Si7021 and SHT21 Digital Humidity &
Temperature Sensor

  written by enjoyneering79
  Modified by nurazur@gmail.com
  Using SoftwareWire so that any Pins can be used for SCL and SDA

  These sensor uses I2C to communicate, 2 pins are required to  
  interface

  Connect HTU21D to pins :  SDA  SCL
  Uno, Redboard, Pro:       A4   A5
  Mega2560, Due:            20   21
  Leonardo:                 2    3

  BSD license, all text above must be included in any redistribution
*/
 /**************************************************************************/

#include "HTU21D_SoftwareWire.h"
#if defined(__AVR__)
#include <util/delay.h>
#endif

HTU21D_SoftI2C::HTU21D_SoftI2C(SoftwareWire* _i2c) {
    i2c = _i2c;
    _HTU21D_Resolution = HTU21D_RES_RH12_TEMP14;
}

/**************************************************************************/
/*
    Constructor
*/
/**************************************************************************/
HTU21D_SoftI2C::HTU21D_SoftI2C(HTU21D_Resolution it)
{
  _HTDU21Dinitialisation = false;
  _HTU21D_Resolution = it;
}

/**************************************************************************/
/*
    Initializes I2C and configures the sensor (call this function before
    doing anything else)

    i2c.endTransmission():
    0 - success
    1 - data too long to fit in transmit data16
    2 - received NACK on transmit of address
    3 - received NACK on transmit of data
    4 - other error
*/
/**************************************************************************/
bool HTU21D_SoftI2C::begin(void)
{

  //softReset();

  /* Make sure we're actually connected */
  i2c->beginTransmission(HTDU21D_ADDRESS);
  if (i2c->endTransmission() != 0)
  {
    return false;
  }

   _HTDU21Dinitialisation = true;

  setResolution(_HTU21D_Resolution);
  setHeater(OFF);

  return true;
}



/**************************************************************************/
/*
    Sets sensor's resolution
*/
/**************************************************************************/
void HTU21D_SoftI2C::setResolution(HTU21D_Resolution it)
{
  uint8_t userRegisterData;

  if (_HTDU21Dinitialisation != true)
  {
    begin();
  }

  /* Get the current register state */
  userRegisterData = read8(USER_REGISTER_READ);

  /* Replace current resolution bits with "0" */
  userRegisterData &= 0x7E;
  /* Add new resolution bits to userRegisterData */
  userRegisterData |= it;

  /* Write updeted userRegisterData to sensor */
  write8(USER_REGISTER_WRITE, userRegisterData);

  /* Update value placeholders */
  _HTU21D_Resolution = it;
}

/**************************************************************************/
/*
    Soft reset.
 
    Switch sensor OFF & ON again. Takes about 15ms

    NOTE: All registers set to default exept heater bit.
*/
/**************************************************************************/
void HTU21D_SoftI2C::softReset(void)
{
  if (_HTDU21Dinitialisation != true)
  {
    begin();
  }

  i2c->beginTransmission(HTDU21D_ADDRESS);
  #if ARDUINO >= 100
    i2c->write(SOFT_RESET);
  #else
    i2c->send(SOFT_RESET);
  #endif
  i2c->endTransmission();
  delay(15);
}

/**************************************************************************/
/*
    Checks Battery Status

    if VDD>2.25v -+0.1v return TRUE
    if VDD<2.25v -+0.1v return FALSE
*/
/**************************************************************************/
bool HTU21D_SoftI2C::batteryStatus(void)
{
  uint8_t userRegisterData;
  
  if (_HTDU21Dinitialisation != true)
  {
    begin();
  }

  userRegisterData = read8(USER_REGISTER_READ);

  userRegisterData &= 0x40;

  if (userRegisterData == 0x00)
  {
    return true;
  }

  return false;
}

/**************************************************************************/
/*
    Turn ON/OFF build-in Heater

    The heater consumtion 5.5mW. Temperature increase of 0.5-1.5 deg.C
    Relative humidity drops upon rising temperature
    
    NOTE:
    Mostly used for diagnostic of sensor's functionality
*/
/**************************************************************************/
void HTU21D_SoftI2C::setHeater(toggleHeaterSwitch it)
{
  uint8_t userRegisterData;
  
  if (_HTDU21Dinitialisation != true)
  {
    begin();
  }

  userRegisterData = read8(USER_REGISTER_READ);

  switch(it)
  {
    case ON:
      userRegisterData |= it;
    break;
    case OFF:
      userRegisterData &= it;
    break;
  }

  write8(USER_REGISTER_WRITE, userRegisterData);
}

/**************************************************************************/
/*
    Reads Humidity, %RH

    Max. measurement time about 16ms.
    Accuracy +-2%RH in range 20%RH - 80%RH at 25deg.C only

    NOTE:
    "operationMode" could be set up as:
    - TRIGGER_HUMD_MEASURE_NOHOLD mode, allows communication on I2C bus while
      sensor is measuring. Could create collision and miscommunications if
      more than one slave device connected to the same bus.
    - TRIGGER_HUMD_MEASURE_HOLD mode, blocks communication on I2C bus while
      sensor is measuring.
*/
/**************************************************************************/
float HTU21D_SoftI2C::readHumidity(humdOperationMode it)
{
  uint8_t   Checksum;
  uint16_t  rawHumidity;
  float     Humidity;

  /* Request a humidity reading */
  i2c->beginTransmission(HTDU21D_ADDRESS);
  #if ARDUINO >= 100
    i2c->write(it);
  #else
    i2c->send(it);
  #endif
  delay(20);
  if (i2c->endTransmission() != 0)
  {
    return(-1.0);
  }

  /* Measurement dalay */
  switch(_HTU21D_Resolution)
  {
    case HTU21D_RES_RH12_TEMP14:
      delay(16);
    break;
    case HTU21D_RES_RH8_TEMP12:
      delay(8);
    break;
    case HTU21D_RES_RH10_TEMP13:
      delay(5);
    break;
    case HTU21D_RES_RH11_TEMP11:
      delay(3);
    break;
  }

  i2c->requestFrom(HTDU21D_ADDRESS,3);


  /* Reads MSB byte, LSB byte & Checksum */
  #if ARDUINO >= 100
    rawHumidity   = i2c->read();  /* Reads MSB byte */
    rawHumidity <<= 8;
    rawHumidity  |= i2c->read();  /* reads LSB byte and sum. with MSB byte */
    Checksum      = i2c->read();
  #else
    rawHumidity   = i2c->receive();
    rawHumidity <<= 8;
    rawHumidity  |= i2c->receive();
    Checksum      = i2c->receive();
  #endif

  if (checkCRC8(rawHumidity) != Checksum)
  {
    return(0.00);
  }

  /* clear two last status bits */
  rawHumidity ^=  0x02;

  Humidity = -6 + 0.001907 * (float)rawHumidity;

  return(Humidity);
}

/**************************************************************************/
/*
    Reads Temperature, deg.C

    Max. measurement time about 50ms.
    Accuracy +-0.3deg.C in range 0deg.C - 60deg.C

    NOTE:
    "operationMode" could be set up as:
    - TRIGGER_TEMP_MEASURE_NOHOLD mode, allows communication on I2C bus while
      sensor is measuring. Could create collision and miscommunications if
      more than one slave device connected to the same bus.
    - TRIGGER_TEMP_MEASURE_HOLD mode, blocks communication on I2C bus while
      sensor is measuring. 
*/
/**************************************************************************/
float HTU21D_SoftI2C::readTemperature(tempOperationMode it)
{
  uint8_t   Checksum;
  uint16_t  rawTemperature;
  float     Temperature;

  /* Request a humidity reading */
  i2c->beginTransmission(HTDU21D_ADDRESS);
  #if ARDUINO >= 100
    i2c->write(it); 
  #else
    i2c->send(it);
  #endif
  delay(20);
  if (i2c->endTransmission() != 0)
  {
    return(-1.00);
  }

  /* Measurement dalay */
  switch(_HTU21D_Resolution)
  {
    case HTU21D_RES_RH12_TEMP14:
      delay(50);
    break;
    case HTU21D_RES_RH8_TEMP12:
      delay(25);
    break;
    case HTU21D_RES_RH10_TEMP13:
      delay(13);
    break;
    case HTU21D_RES_RH11_TEMP11:
      delay(7);
    break;
  }

  /* poll to check the end of the measurement */
  i2c->requestFrom(HTDU21D_ADDRESS,3);
  
  /* Reads MSB byte, LSB byte & Checksum */
   #if ARDUINO >= 100
    rawTemperature   = i2c->read(); /* reads MSB byte */
    rawTemperature <<= 8;
    rawTemperature  |= i2c->read(); /* reads LSB byte and sum. with MSB byte */
    Checksum         = i2c->read();
  #else
    rawTemperature   = i2c->receive();
    rawTemperature <<= 8;
    rawTemperature  |= i2c->receive();
    Checksum         = i2c->receive();
  #endif

  if (checkCRC8(rawTemperature) != Checksum)
  {
    return(0.00);
  }

  Temperature = -46.85 + 0.002681 * (float)rawTemperature;

  return(Temperature);  
}

/**************************************************************************/
/*
    Calculates Compensated Humidity, %RH

    Max. measurement time about 70ms.
    Accuracy +-2%RH in range 0%RH - 100%RH at tmp. range 0deg.C - 80deg.C
*/
/**************************************************************************/
float HTU21D_SoftI2C::readCompensatedHumidity(void)
{
  float humidity;
  float temperature;
  float CompensatedHumidity;

  humidity    = readHumidity();
  temperature = readTemperature();

  if (humidity == 0.00 || temperature == 0.00)
  {
    return (0.00);
  }

  CompensatedHumidity = humidity + (25 - temperature) * TEMP_COEFFICIENT;

  return CompensatedHumidity;
}

float HTU21D_SoftI2C::readCompensatedHumidity(float temperature)
{
  float humidity;
  float CompensatedHumidity;

  humidity    = readHumidity();
  
  if (humidity == 0.00 || temperature == 0.00)
  {
    return (0.00);
  }

  CompensatedHumidity = humidity + (25 - temperature) * TEMP_COEFFICIENT;

  return CompensatedHumidity;
}

/**************************************************************************/
/*
    Writes 8 bit value to the register over I2C
*/
/**************************************************************************/
//void HTU21D_SoftI2C::write8(uint8_t reg, uint32_t value)
void HTU21D_SoftI2C::write8(int reg, int value)
{
  i2c->beginTransmission(HTDU21D_ADDRESS);
  #if ARDUINO >= 100
    i2c->write(reg);
    i2c->write(value);
  #else
    i2c->send(reg);
    i2c->send(value);
  #endif
  i2c->endTransmission();
}

/**************************************************************************/
/*
    Reads 8 bit value over I2C
*/
/**************************************************************************/
uint8_t HTU21D_SoftI2C::read8(uint8_t reg)
{
  i2c->beginTransmission(HTDU21D_ADDRESS);
  #if ARDUINO >= 100
    i2c->write(reg);
  #else
    i2c->send(reg);
  #endif
  i2c->endTransmission();

  i2c->requestFrom(HTDU21D_ADDRESS,1);
  #if ARDUINO >= 100
    return i2c->read();
  #else
    return i2c->receive();
  #endif
}

/**************************************************************************/
/*
    Calculates CRC8 for 16 bit received Data

    NOTE:
    For more info about Cyclic Redundancy Check (CRC) see:
    http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
*/
/**************************************************************************/
uint8_t HTU21D_SoftI2C::checkCRC8(uint16_t data)
{
  for (uint8_t bit = 0; bit < 16; bit++)
  {
    if (data & 0x8000)
    {
      data =  (data << 1) ^ CRC8_POLYNOMINAL;
    } 
    else
    {
      data <<= 1;
    }
  }

  data >>= 8;
  return data;
}
