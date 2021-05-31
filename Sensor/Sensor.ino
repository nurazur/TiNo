// RFM69CW Sender for temperature / humidity Sensors with Watchdog.
// Supports Sensors with HTU21D, SHT20, SHT21, SHT25, SHT30, SHT31, SHT35, BME280, DS18B20
// built for AVR ATMEGA328P
// optional possibility to use  a 32.768 crystal to reduce sleep current to ~1.5 µA

// **********************************************************************************
// Copyright nurazur@gmail.com
// **********************************************************************************
// License
// **********************************************************************************
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Licence can be viewed at
// http://www.fsf.org/licenses/gpl.txt

// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// *********************************************************************************

/*****************************************************************************/
/***  Third Party libraries used in this project                           ***/
/*****************************************************************************/
// Uses LowPower from  https://github.com/rocketscream/Low-Power under creative commons attribution-sharealike 3.0 unported license. see https://creativecommons.org/licenses/by-sa/3.0/
// Uses HTU21D_SoftI2C modified from https://github.com/enjoyneering/HTU21D under BSD licence, rewritten by nurazur for use of SoftwareWire
// Uses SoftwareWire Version 1.4.1. from https://github.com/Testato/SoftwareWire under GNU General Public License v3.0
// Uses a RFM69 Driver originally from LowPowerlab.com However this driver has been changed so much so that it is barely recognizable.
// Uses PinChangeInterrupt from https://github.com/NicoHood/PinChangeInterrupt under open source license, exact type is not specified but looks like GNU
// uses a modified BME280 Driver from https://github.com/finitespace/BME280 under GNU General Public License version 3 or later
// Uses OneWire             // license terms not clearly defined.
// Uses DallasTemperature under GNU LGPL version 2.1 or any later version.
// Uses SHT3x_sww.h - license: "Created by Risele for everyone's use (profit and non-profit)."
// Uses MAX31865 from Ole Wolf <wolf@blazingangles.com> at https://github.com/olewolf/arduino-max31865.git under GNU General Public License version 3 or later
//
/* Revision history:
    Build 3:
        Support for EEPROM selected MAC functions (Encryption, FEC, Interleaving)
        Support for external Crystal, compiler option (because of interfering ISR's)
        RFM69CW TX Gauss shaping adjustable
    Build 4:
        Support for RFM69HCW, P1 Mode, P1+2 Mode, Pa Boost mode.
    Build 5:
        Add Serial port enable in Config settings
    Build 6:
        Implement MAC Class
        cleanup code, remove obsolete defines and comments. Support ACK's
    Build 7:
        FEI alignment in calibration
        EEPROM encrypted
    Build 9: various improvements. VCC is now tested during TX Burst. Support DS18B20 Temperature Sensor, SHT30x, BME280
*/

#define USE_RADIO

#define FILENAME "TiNo 2.2.0 Sensor.ino 28/05/2020"
#define BUILD 9

/*****    which Sensor  do you want to use? select 1 sensor only (2 sensors at your own risk)     *****/
#define USE_HTU21D
//#define USE_DS18B20
//#define USE_BME280
//#define USE_SHT3X
//#define USE_MAX31865
//#define USE_PIR_ON_PCI1
/****************************************************************************/

#define NO_TEMPERATURE_SENSOR (not defined USE_HTU21D) && (not defined USE_SHT3X) && (not defined USE_BME280) && (not defined USE_DS18B20) &&(not defined USE_MAX31865)
#define IS_HUMIDITY_SENSOR  (defined USE_HTU21D) || (defined USE_SHT3X) || (defined USE_BME280)
/****************************************************************************/


#include "pins_arduino.h"


/*****************************************************************************/
/***   MAC                                                                 ***/
/*****************************************************************************/
#include <datalinklayer.h>

/*****************************************************************************/
/***   Radio Driver Instance                                               ***/
/*****************************************************************************/
#include <RFM69.h>  // select this radio driver for RFM69HCW and RFM69CW Modules from HopeRF
//#include <CC1101.h>  // select this driver for CC1101 Modules
RADIO radio;

/*****************************************************************************/
/***  EEPROM Access  and device calibration                                ***/
/*****************************************************************************/
#include "configuration.h"
#include "calibrate.h"

/*****************************************************************************/
/***                            Encryption                                 ***/
/*****************************************************************************/
//encryption is OPTIONAL by compilation switch
//to enable encryption you will need to:
// - provide a 16-byte encryption KEY (same on all nodes that talk encrypted)
// - set the varable ENCRYPTION_ENABLE to 1 in the EEPROM, at runtime in Cfg.EncryptionEnable

#define KEY     "TheQuickBrownFox"


/*****************************************************************************/
/***                            Serial Port                                ***/
/*****************************************************************************/
// SoftwareSerial is NOT by default compatible with PinchangeInterrupt! see http://beantalk.punchthrough.com/t/pinchangeinterrupt-with-softwareserial/4381/4
// only Hardwareserial is supported. Baud rate must be 4800 because Avr runs on 1MHz only and RC-oscillator clock
#define SERIAL_BAUD     4800
HardwareSerial *mySerial = &Serial;




/*****************************************************************************/
/***                   Device specific Configuration                       ***/
/*****************************************************************************/
Configuration Config;


/*****************************************************************************/
/***                   Data Link Controller                                ***/
/*****************************************************************************/
myMAC Mac(radio, Config, (uint8_t*) KEY);


/*****************************************************************************/
/***                   Calibration Module                                  ***/
/*****************************************************************************/
Calibration CalMode(Config, mySerial, &Mac, BUILD, (uint8_t*) KEY);



/*****************************************************************************/
/*** Sleep mode choice ***/
/*****************************************************************************/
uint16_t watchdog_counter;
bool watchdog_expired = false;

// USE_CRYSTAL is a command-line option, do not change here unless you compile from the command line
#ifdef USE_CRYSTAL
    #include "LowPower.h"
    // interrupt on Timer 2 compare "A" completion
    // requires a 32.768kHz Crystal
    ISR(TIMER2_COMPA_vect){watchdog_counter++; }
#else
    #include <avr/sleep.h>
    ISR(WDT_vect) {watchdog_counter++; }
    #define watchdog_wakeup 9  // Wake up after 9 sec
#endif


/*****************************************************************************/
/***                       Pin Change Interrupts                           ***/
/*****************************************************************************/
#include <PinChangeInterrupt.h>
uint8_t event_triggered = 0;

// ISR for the Pin change Interrupt
void wakeUp0() { event_triggered |= 0x1; }
void wakeUp1() { event_triggered |= 0x2; }
void wakeUp2() { event_triggered |= 0x4; }
void wakeUp3() { event_triggered |= 0x8; }


/*****************************************************************************/
/***                        AM312 PIR Parameters                           ***/
/*****************************************************************************/
#ifdef USE_PIR_ON_PCI1
bool pir_dead_time_expired = false;
bool pir_is_off = true;
#define PIR_DEAD_TIME 3
#define PIRVCCPIN 6
// Data pin goes to Config.PCI1Pin
#endif


/*****************************************************************************/
/***                            Sensor Drivers                             ***/
/*****************************************************************************/
// SHT21/HTU21D connects through SoftwareWire, so that SCL and SDA can be any pin
// Add pullup resistors between SDA/VCC and SCL/VCC if not yet provided on Module board
#include "SoftwareWire.h"
SoftwareWire *i2c = NULL;

// if I2C bus has no pullups externally, you can use internal Pullups instead.
// Internal pullup resistors are ~30kOHm. Since we are clocking slowly,
// it works. However, they need to be disabled in sleep mode, because SoftwareWire
// keeps them enabled even if i2c is ended with end()
#define USE_I2C_PULLUPS  true // use this for HTU21D in case no Pullups are mounted.
//#define USE_I2C_PULLUPS false // use this in case external Pullups are used.

/**********      HTU21D       ******/
#ifdef USE_HTU21D
#include "HTU21D_SoftwareWire.h"
HTU21D_SoftI2C *myHTU21D =NULL;
Payload tinytx; // data structure for air interface
#endif

static void Start_HTU21D(void)
{
    #ifdef USE_HTU21D
    i2c->beginTransmission (HTDU21D_ADDRESS);
    if (i2c->endTransmission() == 0)
    {
        if (Config.SerialEnable) mySerial->println ("Found HTU21D");
        myHTU21D = new HTU21D_SoftI2C(i2c);
        delay(100);
        myHTU21D->begin();
        myHTU21D->setResolution(HTU21D_RES_RH10_TEMP13);
    }
    else
    {
        //blink(Config.LedPin, 3);
        if (Config.SerialEnable) mySerial->println ("Could not find a  HTU21D");
    }
    #endif
}

#ifdef USE_HTU21D
static void Measure_HTU21D(float* temperature)
{

    if (myHTU21D)
    {
        pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
        digitalWrite(Config.I2CPowerPin, HIGH); // turn Sensor on
        i2c->begin();
        myHTU21D->begin();
        delay(50);
        *temperature = myHTU21D->readTemperature();
        tinytx.humidity = myHTU21D->readCompensatedHumidity(*temperature) * 2;
        if (USE_I2C_PULLUPS)
        {
            pinMode(Config.SDAPin, INPUT);
            pinMode(Config.SCLPin, INPUT);
        }
        digitalWrite(Config.I2CPowerPin, LOW); // turn Sensor off
    }

}
#endif

/**********     One-Wire and DS18B20     **********/
#ifdef USE_DS18B20
#include <DallasTemperature.h>       // GNU Lesser General Public License v2.1 or later
#include <OneWire.h>                // license terms not clearly defined.

#define ONE_WIRE_BUS Config.SDAPin
#define ONE_WIRE_POWER Config.I2CPowerPin

OneWire *oneWire=NULL;
DallasTemperature *ds18b20=NULL;
#if (defined USE_HTU21D) || (defined USE_SHT3X) || (defined USE_BME280)
    // packet type already defined.
#else
    PacketType4 tinytx;
#endif
static uint8_t start_ds18b20(DallasTemperature *sensor, byte PowerPin);


// One-Wire DS18B20 start-up sequence
static uint8_t start_ds18b20(DallasTemperature *sensor, byte PowerPin)
{
    pinMode(PowerPin, OUTPUT); // set power pin for DS18B20 to output
    digitalWrite(PowerPin, HIGH); // turn DS18B20 sensor on
    delay(10); // Allow 10ms for the sensor to be ready
    sensor->begin();
    //sensor->setResolution(10); //Resolutiuon is 0.125 deg, absolutely sufficient!
    delay(10); // Allow 10ms for the sensor to be ready
    return sensor->getDeviceCount();
}

static void stop_ds18b20(byte PowerPin)
{
    digitalWrite(PowerPin, LOW); // turn Sensor off to save power
}
#endif

static void Start_DS18B20(void)
{
    #ifdef USE_DS18B20
    //--------------------------------------------------------------------------
    // test if 1-wire devices are present
    //--------------------------------------------------------------------------
    pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
    digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on
    delay(10); // Allow 10ms for the sensor to be ready
    // Start up the library
    oneWire= new OneWire(ONE_WIRE_BUS);
    ds18b20 = new DallasTemperature(oneWire);

    uint8_t num_devices = start_ds18b20(ds18b20, ONE_WIRE_POWER);
    if (num_devices == 0)
    {
        delete ds18b20;
        ds18b20 = NULL;
        stop_ds18b20(ONE_WIRE_POWER);
        if (Config.SerialEnable) mySerial->print("no Dallas DS18B20 found\n\r");

    }
    else
    {
        if (Config.SerialEnable)
        {
            mySerial->print(num_devices, DEC);
            mySerial->println(" DS18B20 devices found.");
            mySerial->flush();
        }
    }
    #endif
}

#ifdef USE_DS18B20
static void Measure_DS18B20(float*temperature)
{

    if (ds18b20)
        {
            uint8_t num_devices = start_ds18b20(ds18b20, ONE_WIRE_POWER);
            if (num_devices > 0)
            {
                ds18b20->requestTemperatures();
                *temperature = ds18b20->getTempCByIndex(0);
                mySerial->print("DS18B20 Temp: ");mySerial->print(*temperature); mySerial->println(" degC");
                stop_ds18b20(ONE_WIRE_POWER); // Turn off power Pin for DS18B20
            }
            delay(65); // add delay to allow wdtimer to increase in case its erroneous
        }
}

static void Measure_DS18B20(PacketType4 &tinytx, float *t)
{
    if (ds18b20)
    {
        float temperature;
        uint8_t num_devices = start_ds18b20(ds18b20, ONE_WIRE_POWER);
        if (num_devices > 0)
        {
            ds18b20->requestTemperatures();
            switch (num_devices)
            {
                case 3:
                    temperature = ds18b20->getTempCByIndex(2);
                    tinytx.temp2 = floor(temperature * 25 + 1000.5);
                    mySerial->print("Temp2: ");mySerial->print(temperature); mySerial->println(" degC");
                case 2:
                    temperature = ds18b20->getTempCByIndex(1);
                    tinytx.temp1 = floor(temperature * 25 + 1000.5);
                    mySerial->print("Temp1: ");mySerial->print(temperature); mySerial->println(" degC");
                case 1:
                    *t = ds18b20->getTempCByIndex(0);
                    //tinytx.temp = floor(temperature * 25 + 1000.5);
                    mySerial->print("Temp0: ");mySerial->print(*t); mySerial->println(" degC");
                    break;
            }

            //*temperature = ds18b20->getTempCByIndex(0);
            //tinytx.temp = floor(*temperature * 25 + 1000.5);
            //mySerial->print("Temp: ");mySerial->print(*temperature); mySerial->println(" degC");
            //if (tinytx.count == 0) tinytx.humidity++;   // use humidity as a second counter byte, so we get a 16 bit counter

            //for (uint8_t i =0; i< num_devices; i++)
            //{
            //     mySerial->print("Temp"); mySerial->print(i); mySerial->print(": "); mySerial->print(ds18b20->getTempCByIndex(i)); mySerial->println(" degC");
            //}
            stop_ds18b20(ONE_WIRE_POWER); // Turn off power Pin for DS18B20
        }
        delay(65); // add delay to allow wdtimer to increase in case its erroneous


    }
}

#endif

/**********      BME280     **********/
#ifdef USE_BME280
#include <BME280_SoftwareWire.h>
BME280_SoftwareWire *bme=NULL;
PacketType3 tinytx; // special data type for air interface
#endif

static void Start_BME280(void)
{
    #ifdef USE_BME280
    i2c->beginTransmission (0x76);

    if (i2c->endTransmission() == 0)
    {
        if (Config.SerialEnable) mySerial->println ("Found BME280");
        bme = new BME280_SoftwareWire(i2c);
        //mySerial->println ("BME280 instance created.");
    }
    #endif
}

#ifdef USE_BME280
static void Measure_BME280(float &temperature)
{
    if(bme)
        {
            float hum(NAN), pres(NAN);
            BME280b::TempUnit tempUnit(BME280b::TempUnit_Celcius);
            BME280b::PresUnit presUnit(BME280b::PresUnit_hPa);

            pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
            digitalWrite(Config.I2CPowerPin, HIGH);
            i2c->begin();
            delay(20);
            bme->begin();
            delay(125);

            bme->read(pres, temperature, hum, tempUnit, presUnit);
            digitalWrite(Config.I2CPowerPin, LOW);
            //tinytx.temp = (temperature * 25.0 + 1000.5);
            tinytx.humidity = (hum*2);
            tinytx.pressure = pres *100.0;
            mySerial->print("Pressure: ");
            mySerial->print(pres);
            mySerial->print(" hPa, ");
            mySerial->flush();

            digitalWrite(Config.I2CPowerPin, LOW);
        }
}
#endif

/**********      SHT30, SHT31, SHT35     **********/
#ifdef USE_SHT3X
#include <SHT3x_sww.h>
SHT3x *mySHT3x=NULL;
Payload tinytx;
#endif

static void Start_SHT3X(void)
{
    #ifdef USE_SHT3X
    i2c->beginTransmission (0x44);
    if (i2c->endTransmission() == 0)
    {
        if (Config.SerialEnable) mySerial->println ("Found SHT3x");
        mySHT3x = new SHT3x(i2c);
        delay(50);
        mySHT3x->Begin();
    }
    #endif
}

#ifdef USE_SHT3X
static void Measure_SHT3x(float *temperature)
{
    if (mySHT3x)
        {
            pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
            digitalWrite(Config.I2CPowerPin, HIGH); // turn Sensor on
            delay(250); // wait for the Sensor to power up completely.
            //i2c->begin();
            mySHT3x->Begin();
            mySHT3x->GetData();
            *temperature = mySHT3x->GetTemperature();
            //tinytx.temp = (*temperature * 25.0 + 1000.5);
            tinytx.humidity = (mySHT3x->GetRelHumidity()*2);
            digitalWrite(Config.I2CPowerPin, LOW); // turn Sensor off
        }
}
#endif

/**********      MAX31865     **********/
#ifdef USE_MAX31865
#include "MAX31865.h"
#include <SPI.h>
#define PT100

// Chip Select Pin
#define RTD_CS_PIN  A0
#define RTD_PWR_PIN A1

#define FAULT_HIGH_THRESHOLD  0x9304  /* +350C */
#define FAULT_LOW_THRESHOLD   0x2690  /* -100C */

MAX31865_RTD *rtd=NULL;
#if not (IS_HUMIDITY_SENSOR) && (not defined USE_DS18B20)
PacketType4 tinytx;
#endif


static void Start_MAX31865(void)
{
    pinMode(RTD_CS_PIN, INPUT_PULLUP);  // SPI SS (avoid becoming SPI Slave)

    Serial.println("starting MAX31865");

    #ifdef PT1000
    // For PT 1000 (Ref on breakout board = 3900 Ohms 0.1%)
    rtd= new MAX31865_RTD( MAX31865_RTD::RTD_PT1000, RTD_CS_PIN, 3900 );
    #endif

    #ifdef PT100
    // For PT 100  (Ref Ref on breakout board = 430 Ohms 0.1%)
    rtd= new MAX31865_RTD( MAX31865_RTD::RTD_PT100, RTD_CS_PIN, 430 );
    //Serial.println("created new instance of Max31865 class.");
    #endif
}


static uint8_t Measure_MAX31865(float* pt100_temp)
{
    uint8_t status = 0xFF;
    if(rtd)
    {
        SPI.setDataMode( SPI_MODE1 );
        pinMode(RTD_PWR_PIN,OUTPUT);
        digitalWrite(RTD_PWR_PIN, 1);
        delay(10);
        //Serial.print(F("Checking MAX31865..."));
        rtd->configure( VBIAS_ENABLED, CONVERSION_MODE_OFF, ONESHOT_DISABLED, USE_4WIRES, MAX31865_FAULT_DETECTION_NONE, true, true, FAULT_LOW_THRESHOLD, FAULT_HIGH_THRESHOLD );
        //Serial.println(F(" Found!"));
        //Serial.println(F("Fault detection cycle."));Serial.flush();
        rtd->configure( VBIAS_ENABLED, CONVERSION_MODE_OFF, ONESHOT_DISABLED, MAX31865_FAULT_DETECTION_MANUAL_1 ); //0x8
        delay(60);
        rtd->configure( VBIAS_ENABLED, CONVERSION_MODE_OFF, ONESHOT_DISABLED, MAX31865_FAULT_DETECTION_MANUAL_2 );

        status = rtd->fault_status();
        if (status==0 )
        {
            //Serial.println(F("Starting conversion..."));Serial.flush();

            // Start 1 shot measure
            // V_BIAS enabled , No Auto-conversion, 1-shot enabled, No Fault detection
            rtd->configure( VBIAS_ENABLED, CONVERSION_MODE_OFF, ONESHOT_ENABLED, MAX31865_FAULT_DETECTION_NONE );
            delay(70);
            status = rtd->read_all();
            *pt100_temp = rtd->temperature();

            Serial.print(F( "RTD:"));Serial.print( rtd->resistance());
            Serial.print(F( " Ohms => Temp:"));
            Serial.print( *pt100_temp,2); Serial.println(F(" C" ));
        }
        else
        {
            Serial.println("MAX31865 Failure.");
        }

        digitalWrite(RTD_PWR_PIN, 0);
    }

    return status;
}



#else
    static void Start_MAX31865(void)
    {
    }
#endif

/*****************************************************************************/
/****                         blink                                       ****/
/*****************************************************************************/
void activityLed (unsigned char state, unsigned int time = 0)
{
  if (Config.LedPin) {
    pinMode(Config.LedPin, OUTPUT);
    if (time == 0) {
      digitalWrite(Config.LedPin, state);
    } else {
      digitalWrite(Config.LedPin, state);
      delay(time);
      digitalWrite(Config.LedPin, !state);
    }
  }
}

// blink led
static void blink (byte pin, byte n = 3)
{
  if (pin)
  {
    pinMode(pin, OUTPUT);
    for (byte i = 0; i < (2 * n)-1; i++)
    {
      digitalWrite(pin, !digitalRead(pin));
      delay(100);
    }
    digitalWrite(pin, !digitalRead(pin));
  }
}

/*****************************************************************************/
/*
     Read VCC by taking measuring the 1.1V reference and taking VCC as reference voltage.
     set the reference to Vcc and the measurement to the internal 1.1V reference
*/
/*****************************************************************************/
long readVcc() {

  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
      ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
      ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
      ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high<<8) | low;
  return result;
}


float getVcc(long vref)
{
    return (float)vref / readVcc();
}

/*****************************************************************************/
/***                        Power Save Functions                           ***/
/*****************************************************************************/
#ifdef USE_CRYSTAL

static void setup_timer2()
{
  // clock input to timer 2 from XTAL1/XTAL2
  ASSR = bit (AS2);
  while (ASSR & 0x1f);

  // set up timer 2 to count up to 256 * 1024  (32768) = 8s
  TCCR2A = bit (WGM21);     //WGM20 and WGM22 in TCCR2B is set 0, Mode of operation: CTC
  TCCR2B = bit (CS22) | bit (CS21) | bit (CS20);    // Prescaler of 1024
  //TCCR2B = bit (CS21) | bit (CS20);    // Prescaler of 32
  //TCCR2B = bit (CS20);    // Prescaler of 1
  //TCCR2B &= 0xF8; // timer off, CS0=0, CS1=0, CS2=0

  while (ASSR & 0x08);   // wait until OCR2A can be updated
  OCR2A =  255;         // count to 255 (zero-relative)


  while (ASSR & 0x1f);   // update is busy

  // enable timer interrupts
  TIMSK2 |= bit (OCIE2A);
}

#else

// Enable / Disable ADC, saves ~230uA
static void enableADC(bool b)
{
  if (b == true){
    bitClear(PRR, PRADC); // power up the ADC
    ADCSRA |= bit(ADEN);  // enable the ADC
    delay(10); // what is the delay good for?
  } else {
    ADCSRA &= ~ bit(ADEN); // disable the ADC
    bitSet(PRR, PRADC);    // power down the ADC
  }
}

// send Avr into Power Save Mode
static void goToSleep()
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode.
  sleep_enable(); // Enable sleep mode.
  sleep_mode(); // Enter sleep mode.

  // After waking from watchdog interrupt the code continues to execute from this point.

  sleep_disable(); // Disable sleep mode after waking.
  // Re-enable the peripherals.
  //power_all_enable();
}


static void setup_watchdog(int timerPrescaler) {
  if (timerPrescaler > 9 ) timerPrescaler = 9; //Correct incoming amount if need be
  byte bb = timerPrescaler & 7;
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary
  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watchdog reset
  WDTCSR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCSR = bb; //Set new watchdog timeout value
  WDTCSR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}
#endif




static const char* show_trigger(byte trigger)
{
    const char* triggerstr="";
    switch (trigger & 0x3)
    {
        case 0:
            triggerstr = "LOW";
            break;
        case 1:
            triggerstr = "CHANGE";
            break;
        case 2:
            triggerstr = "FALLING";
            break;
        case 3:
            triggerstr = "RISING";
            break;
        default:
            break;
    }

    return triggerstr;
}

static const char* show_pinmode(byte pinmode)
{
    const char* pinmodestr;
    switch (pinmode >> 2)
    {
        case 0:
            pinmodestr = "INPUT";
            break;
        case 1:
            pinmodestr = "OUTPUT";
            break;
        case 2:
            pinmodestr = "INPUT_PULLUP";
            break;
        default:
            pinmodestr = "UNKNOWN";
    }

    return pinmodestr;
}
/*****************************************************************************/
// should go into configuration class
static void print_eeprom(Stream *serial)
{
    serial->print("Nodeid = ");        serial->print(Config.Nodeid);          serial->println();
    serial->print("Networkid = ");     serial->print(Config.Networkid);       serial->println();
    serial->print("Gatewayid =  ");    serial->print(Config.Gatewayid);       serial->println();
    serial->print("VccAtCalmV  = ");   serial->print(Config.VccAtCalmV);      serial->println();
    serial->print("AdcCalValue = ");   serial->print(Config.AdcCalValue);     serial->println();
    serial->print("Senddelay = ");     serial->print(Config.Senddelay);       serial->println();
    serial->print("Frequencyband = "); serial->print(Config.Frequencyband);   serial->println();
    serial->print("frequency = ");     serial->print(Config.frequency);       serial->println();
    serial->print("TxPower = ");       serial->print(Config.TxPower);         serial->println();
    serial->print("RequestAck = ");    serial->print(Config.RequestAck);      serial->println();
    serial->print("LedCount = ");      serial->print(Config.LedCount);        serial->println();
    serial->print("LedPin = ");        serial->print(Config.LedPin);          serial->println();
    serial->print("RxPin = ");         serial->print(Config.RxPin);           serial->println();
    serial->print("TxPin = ");         serial->print(Config.TxPin);           serial->println();
    serial->print("SDAPin = ");        serial->print(Config.SDAPin);          serial->println();
    serial->print("SCLpin = ");        serial->print(Config.SCLPin);          serial->println();
    serial->print("I2CPowerPin = ");   serial->print(Config.I2CPowerPin);     serial->println();
    serial->print("checksum = ");      serial->print(Config.checksum);        serial->println();
    serial->print("Interrupt 0 Pin = ");serial->print(Config.PCI0Pin,HEX);        serial->println();
    serial->print("Interrupt 1 Pin = ");serial->print(Config.PCI1Pin,HEX);        serial->println();
    serial->print("Interrupt 2 Pin = ");serial->print(Config.PCI2Pin,HEX);        serial->println();
    serial->print("Interrupt 3 Pin = ");serial->print(Config.PCI3Pin,HEX);        serial->println();
    serial->print("Interrupt 0 Type = ");serial->print(show_trigger(Config.PCI0Trigger)); serial->print(", ");  serial->print(show_pinmode(Config.PCI0Trigger)); serial->println();
    serial->print("Interrupt 1 Type = ");serial->print(show_trigger(Config.PCI1Trigger)); serial->print(", ");  serial->print(show_pinmode(Config.PCI1Trigger)); serial->println();
    serial->print("Interrupt 2 Type = ");serial->print(show_trigger(Config.PCI2Trigger)); serial->print(", ");  serial->print(show_pinmode(Config.PCI2Trigger)); serial->println();
    serial->print("Interrupt 3 Type = ");serial->print(show_trigger(Config.PCI3Trigger)); serial->print(", ");  serial->print(show_pinmode(Config.PCI3Trigger)); serial->println();
    serial->flush();
    serial->print("Use Crystal = ");     serial->print(Config.UseCrystalRtc);  serial->println();
    serial->print("Encryption = ");     serial->print(Config.EncryptionEnable);  serial->println();
    serial->print("FEC = ");            serial->print(Config.FecEnable);  serial->println();
    serial->print("Interleave = ");     serial->print(Config.InterleaverEnable);  serial->println();
    serial->print("EEPROM Version= ");  serial->print(Config.EepromVersionNumber);  serial->println();
    serial->print("Software Version = ");  serial->print(Config.SoftwareversionNumber);  serial->println();
    serial->print("Gauss shaping= ");   serial->print(Config.TXGaussShaping);  serial->println();
    serial->print("Serial Port Enable = "); serial->print(Config.SerialEnable);  serial->println();
    serial->print("RF Chip = "); Config.IsRFM69HW ?    serial->print("RFM69HCW") : serial->print("RFM69CW");  serial->println();
    serial->print("PA Boost = ");      serial->print(Config.PaBoost);  serial->println();
    serial->print("Fdev (Steps) = ");      serial->print(Config.FedvSteps);  serial->println();
}

#if NO_TEMPERATURE_SENSOR
    Payload tinytx;
#endif


// init Setup
void setup()
{
  #if F_CPU == 4000000L
    // Change to 4 MHz by changing clock prescaler to 2
    cli(); // Disable interrupts
    CLKPR = (1<<CLKPCE); // Prescaler enable
    CLKPR = (1<<CLKPS0); // Clock division factor 2 (0001)
    sei(); // Enable interrupts
  #endif

  #ifdef SOFTSERIAL
        pinMode(rxPin, INPUT);
        pinMode(txPin, OUTPUT);
        mySerial = new SoftwareSerial(rxPin, txPin);
  #endif

    mySerial->begin(SERIAL_BAUD);

    // output of filename without condition for debug purposes
    {
        mySerial->print(FILENAME); mySerial->print(" Build: "); mySerial->print(BUILD);mySerial->println();
    }

    /***                     ***/
    /***     CALIBRATE?      ***/
    /***                     ***/
    CalMode.configure();

    //if (Config.SerialEnable) print_eeprom(mySerial);

    /*
    PCIOxTrigger bits 0 and 1:
    0bxx00 LOW
    0bxx01 CHANGE
    0bxx10 FALLING (normal use case)
    0bxx11 RISING

    PCIOxTrigger bits 2 and 3:
    0b00xx INPUT
    0b01xx OUTPUT
    0b10xx INPUT_PULLUP (normal use case)
    */

    if (Config.PCI0Pin >=0)
    {
        pinMode(Config.PCI0Pin, (Config.PCI0Trigger >>2) );  // set the pin to input or input with Pullup
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(Config.PCI0Pin), wakeUp0, Config.PCI0Trigger&0x3);
    }
    if (Config.PCI1Pin >=0)
    {
        pinMode(Config.PCI1Pin, (Config.PCI1Trigger >>2));  // set the pin to input
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(Config.PCI1Pin), wakeUp1, Config.PCI1Trigger&0x3);
    }
    if (Config.PCI2Pin >=0)
    {
        pinMode(Config.PCI2Pin, (Config.PCI2Trigger >>2));  // set the pin to input
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(Config.PCI2Pin), wakeUp2, Config.PCI2Trigger&0x3);
    }
    if (Config.PCI3Pin >=0)
    {
        pinMode(Config.PCI3Pin, (Config.PCI3Trigger >>2));  // set the pin to input
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(Config.PCI3Pin), wakeUp3, Config.PCI3Trigger&0x3);
    }

    #ifndef USE_CRYSTAL
        if (Config.SerialEnable) mySerial->println ("USING WDT");
    #else
        if (Config.SerialEnable) mySerial->println ("USING CRYSTAL TIMER");
    #endif

    pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
    digitalWrite(Config.I2CPowerPin, HIGH);
    delay(5);
    i2c = new SoftwareWire( Config.SDAPin, Config.SCLPin, USE_I2C_PULLUPS );
    i2c->setClock(10000L);
    i2c->begin();


    Start_HTU21D();
    Start_BME280();
    Start_SHT3X();
    Start_DS18B20();
    Start_MAX31865();


    digitalWrite(Config.I2CPowerPin, LOW);
    if (USE_I2C_PULLUPS)
    {
        pinMode(Config.SDAPin, INPUT);
        pinMode(Config.SCLPin, INPUT);
    }
    #ifdef USE_RADIO
    //mySerial->println ("Start Radio.");
    Mac.radio_begin(); // puts radio to sleep to save power.

    //mySerial->println ("Radio running.");
    // for debug: this makes sure we can read from the radio.
    //byte version_raw = radio.readReg(REG_VERSION);
    //mySerial->print ("Radio chip ver: "); mySerial->print(version_raw>>4, HEX); mySerial->print (" Radio Metal Mask ver: "); mySerial->print(version_raw&0xf, HEX); mySerial->println();
    #endif


    tinytx.targetid = Config.Gatewayid;
    tinytx.nodeid =   Config.Nodeid;
    tinytx.flags =    1; // LSB means "heartbeat"
    #if (defined USE_HTU21D) || (defined USE_BME280) || (defined USE_SHT3X)
    tinytx.humidity = 0;
    #endif

    #ifndef USE_CRYSTAL
    if (Config.Senddelay != 0)
    {
        setup_watchdog(watchdog_wakeup);    // Wake up after 8 sec
        PRR = bit(PRTIM1);                  // only keep timer 0 going
    }
    enableADC(false);           // power down/disable the ADC
    #else
    setup_timer2();
    #endif
    analogReference(INTERNAL);  // Set the aref to the internal 1.1V reference
    watchdog_counter = 500;     // set to have an initial transmission when starting the sender.


    #ifdef USE_PIR_ON_PCI1
    pinMode(PIRVCCPIN,OUTPUT);
    digitalWrite(PIRVCCPIN, HIGH);
    pir_is_off = false;

    // avoid triggering the PCINT when turning on power on PIR
    #ifndef USE_CRYSTAL
        setup_watchdog(5); // 0.5 s
        goToSleep();
        setup_watchdog(7); // 2s
        goToSleep();
        setup_watchdog(watchdog_wakeup);
    #else
        LowPower.powerDown(SLEEP_500MS , ADC_OFF, BOD_OFF);
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
    #endif

    event_triggered &= 0xFD; //1111 1101 clear bit 1, this is PCI1 from PIR Sensor
    #endif

    if (Config.LedPin)
    {
        activityLed(1,1000); // LED on
    }
}

void loop()
{
    #ifndef USE_CRYSTAL
    goToSleep(); // goes to sleep for about 9 seconds and continues to execute code when it wakes up
    #else
    static bool timer2_setup_done = false;
    if (Config.Senddelay != 0)
        LowPower.powerSave(SLEEP_FOREVER, ADC_OFF, BOD_OFF, TIMER2_ON);
    else
    {
        timer2_setup_done = true;
        TCCR2B &= 0xF8; // timer 2 off, CS0=0, CS1=0, CS2=0
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }
    #endif

    #ifdef USE_PIR_ON_PCI1
    pir_dead_time_expired = (watchdog_counter >= PIR_DEAD_TIME) && pir_is_off; // if true, turn on VCC of PIR and wait 2.5s
    if (pir_dead_time_expired)
    {
        pir_is_off = false;
        digitalWrite(PIRVCCPIN, HIGH);
        pir_dead_time_expired = false;
        if (Config.SerialEnable) Serial.println("PIR active."); Serial.flush();

        #ifndef USE_CRYSTAL
        setup_watchdog(5);
        goToSleep();
        setup_watchdog(7);
        goToSleep();
        setup_watchdog(watchdog_wakeup);

        #else
        // when PIR ist switched ON, event triggers and we need to wait for 2.5s because data pin goes high for 2.5s
        LowPower.powerDown(SLEEP_500MS , ADC_OFF, BOD_OFF);
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
        #endif

        event_triggered &= 0xFD; //1111 1101 clear bit 1, this is PCI1 from PIR Sensor
    }
    #endif

    watchdog_expired = ((watchdog_counter >= Config.Senddelay) && (Config.Senddelay != 0)); // when Senddelay equals 0, sleeep forever and wake up only for events.
    if (watchdog_expired || event_triggered)
    {
        #ifndef USE_CRYSTAL
        enableADC(true); // power up/enable the ADC
        #endif
        tinytx.flags = 0;
        float temperature=0;
        if (watchdog_expired)
        {
            tinytx.flags |= 0x1; // heartbeat flag
            watchdog_counter = 0;
        }
        tinytx.targetid = Config.Gatewayid;
        if(event_triggered)
        {
            tinytx.flags |= (event_triggered << 1);

            //um Fernbedienungen mit mehreren Kanaelen herzustellen, muss der Node
            //Pakete an verschiedene Nodeid's schicken koennen. Diese ID's koennten
            //im EEPROM gespeichert sein. Um aber kompatibel mit TiNo 2.1.0 zu bleiben
            //habe ich das mit einem Offset geloest der in den oberen 4 Bits des
            //Trigger-Bytes liegt. Wenn also die GatewayID 22 ist und im
            // PCIxTrigger z.B. 0b0111xxxx steht, ist die neue TargetID 22+7 = 29.
            switch (event_triggered)
            {
                case 0x01:
                    tinytx.targetid += (Config.PCI0Trigger >> 4);
                    break;
                case 0x02:
                    tinytx.targetid += (Config.PCI1Trigger >> 4);
                    break;
                case 0x04:
                    tinytx.targetid += (Config.PCI2Trigger >> 4);
                    break;
                case 0x08:
                    tinytx.targetid += (Config.PCI3Trigger >> 4);
                    break;
                default:
                    break;
            }
        }

        // PIR Sensor triggered: turn off Sensor for PIR_DEAD_TIME seconds
        #ifdef USE_PIR_ON_PCI1
        if (event_triggered & 0x2)//PCI1
        {
            pir_is_off = true;
            digitalWrite(PIRVCCPIN, LOW);
            watchdog_counter =0;
            if (Config.SerialEnable)
            {
                Serial.println("PIR triggered."); Serial.flush();
            }
            blink(Config.LedPin, 3);
        }
        #endif

        long Vcal_x_ADCcal = (long)Config.AdcCalValue * Config.VccAtCalmV;
        #ifdef USE_RADIO
            tinytx.supplyV  = Vcal_x_ADCcal / radio.vcc_dac; // the VCC measured during last TX Burst.
        #else
            tinytx.supplyV  = Vcal_x_ADCcal / readVcc();
        #endif

        tinytx.count++;
        #if defined USE_DS18B20 || ((defined USE_MAX31865) && (not IS_HUMIDITY_SENSOR))
        if (tinytx.count == 0) tinytx.count_msb++;
        #endif

        #ifdef USE_HTU21D
        Measure_HTU21D(&temperature);
        #endif

        #ifdef USE_SHT3X
        Measure_SHT3x(&temperature);
        #endif

        #ifdef USE_BME280
        Measure_BME280(temperature);
        tinytx.flags |= 0x20; // mark as alternate protocol
        #endif

        #ifdef USE_DS18B20
        #if (defined USE_HTU21D) || (defined USE_SHT3X) || (defined USE_BME280)
            Measure_DS18B20(&temperature);
        #else
            Measure_DS18B20(tinytx, &temperature);
        #endif
        tinytx.flags |= 0x20; // mark as alternate protocol
        #endif

        #ifdef USE_MAX31865
            uint8_t status = Measure_MAX31865(&temperature);
            Serial.print("RTD status: "); Serial.println(status);
            #if not (IS_HUMIDITY_SENSOR) && (not defined USE_DS18B20)
                tinytx.flags |= 0x20; // mark as alternate protocol
            #endif
        #endif

        #if NO_TEMPERATURE_SENSOR
            #ifdef USE_RADIO
                temperature = radio.readTemperature(0);
            #else
                temperature = 12.34;
            #endif
            if (Config.Senddelay != 0) delay(65); // add delay to allow wdtimer to increase in case its erroneous

        #endif

        tinytx.temp = floor (temperature * 25 + 1000.5);


        if (Config.SerialEnable)
        {
            //mySerial->print("Sensor measurement: ");
            #if IS_HUMIDITY_SENSOR
            mySerial->print(tinytx.humidity/2.0);
            mySerial->print(" %RH\t");
            #endif
            mySerial->print(tinytx.temp/25.0-40.0); mySerial->println(" degC");
        }

        #ifdef USE_CRYSTAL
        // workaround for timer2 setup failure (registers read OK but timer runs fast)

        if (watchdog_counter>0  && !timer2_setup_done)
        {
            setup_timer2(); // set it up again. Cures the problem.
            watchdog_counter = 0;
            timer2_setup_done =  true;
        }
        #endif



        bool ackreceived = false;
        #ifdef USE_RADIO
        temperature<0?  temperature-=0.5 : temperature+=0.5;

        /****        uncomment if you want to use frequency correction                        ****/
        //ackreceived = Mac.radio_send((uint8_t*) &tinytx, sizeof(tinytx), Config.RequestAck, int(temperature));

        /****         use this line if you don't need frequency correction (default)          ****/
        ackreceived = Mac.radio_send((uint8_t*) &tinytx, sizeof(tinytx), Config.RequestAck);

        if (Config.RequestAck && Config.SerialEnable)
            (ackreceived) ? mySerial->println("Ack received.") : mySerial->println(" No Ack received.");
        #endif

        #ifndef USE_CRYSTAL
        enableADC(false); // power down/disable the ADC
        #endif

        if (event_triggered)
        {
            event_triggered = 0;
            if (ackreceived)
                activityLed (1, 100);
        }
        else if (Config.LedPin && Config.LedCount >0)
        {
            Config.LedCount--;
            blink(Config.LedPin, 2); // blink LED
        }
        if (Config.SerialEnable) mySerial->flush();
    }
}
