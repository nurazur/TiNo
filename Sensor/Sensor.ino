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

    Build 10:
        one firmware for all sensors
        support for LDR (brightness reading), requires a analog pin (A0 or A1)
        choice of sleep mode can be set in configuration - WDT vs crystal timer
        possible External Heartbeat using TPL5110
        for each PCINT a individual Gateway ID (multi-channel remote control)
        calibration mode: 't' sends a dummy packet
        calibration mode: 'to' starts den OOK Modus (send a CW signal)
        calibration mode: 'tt' retrieves the Tepmerature reading from the RFM69 chip
        calibration mode: Offset for RFM69 temperature in configuration
        Receiver can output locally measured sensor data (HTU21D only in this release)
*/
#include "pins_arduino.h"
#include "HardwareSerial.h"

#define USE_RADIO

#define FILENAME "TiNo 3.0.1 Sender.ino 03/03/2022"
#define BUILD 10

/*****************************************************************************/
/***   MAC                                                                 ***/
/*****************************************************************************/
#include <datalinklayer.h>



/*****    which Sensor  do you want to use? select 1 sensor only (2 sensors at your own risk)     *****/
#define USE_HTU21D
//#define USE_BME280
#define USE_DS18B20
#define USE_SHT3X
#define USE_MAX31865


typedef struct
{
    byte HTU21D:1;
    byte DS18B20:1;
    byte BME280:1;
    byte SHT3X:1;
    byte MAX31865:1;
    byte BRIGHTNESS:1;
    byte Reserved:2;

} UseBits;


class sensors
{
    public:
        UseBits use;


        sensors (UseBits sensor_cfg)
        {
            this->use = sensor_cfg;
            PacketLen =  0;
            this->calc_packettype();
            this->count(0);
            this->temp1(0);
            this->temp2(0);
        }

        int8_t calc_packettype(void)
        {
            // prefer SHT3x over HTU21D in case both are present.
            // currently there is no Packet type for 2 humidity sensors.
            if (use.HTU21D && use.SHT3X)
            {
                use.HTU21D=0;
            }

            // no packet type available for 2 humidity sensors. BME280 preferred.
            if (use.BME280 && (use.HTU21D || use.SHT3X))
            {
                PacketType = 0;
                use.HTU21D =0;
                use.SHT3X =0; // don't use the humidity sensors in this case!
            }

            this->is_temperature_sensor = use.HTU21D || use.BME280 || use.DS18B20 || use.SHT3X || use.MAX31865;
            this->is_humidity_sensor = use.HTU21D || use.BME280 || use.SHT3X;


            // no explicit temperature sensor on board, i.e for Interrupts only like a remote control
            if (!is_temperature_sensor)
            {
                PacketType = 0;
            }
            else if (use.HTU21D || use.SHT3X)
            {
                if (use.DS18B20 || use.MAX31865)
                {
                    PacketType = 5;  // 1 additional external Temp sensor
                }
                else
                {
                    // default
                    PacketType = 0;
                }
            }

            // air pressure
            else if (use.BME280)
            {
                PacketType = 0;
            }

            // Temperature sensors only: up to 3
            else if (use.DS18B20 || use.MAX31865)
            {
                if (!is_humidity_sensor)
                {
                    PacketType = 4;
                }
            }
            else
                PacketType =-1;

            switch (PacketType)
            {
                case 0:
                    pData = (uint8_t*) &t0;
                    if (!use.BME280  && !use.BRIGHTNESS){
                        PacketLen = 8;
                    }
                    else{
                        PacketLen = sizeof(Payload);
                    }
                    break;
                case 4:
                    pData  = (uint8_t*) &t4;
                    PacketLen = sizeof(PacketType4);
                    t4.packet_type=4;
                    use.BRIGHTNESS=0; // brightness not possible with packet type 4
                    break;
                case 5:
                    pData= (uint8_t*) &t5;
                    t5.packet_type=5;
                    PacketLen = sizeof(PacketType5);
                    break;
                default:
                    break;
            }

            return PacketType;
        }

        void humidity(uint8_t h)
        {
            switch(this->PacketType)
            {
                case 0:
                    t0.humidity = h;
                    break;
                case 5:
                    t5.humidity = h;
                    break;
                default:
                    break;
            }
        }

        void pressure(uint32_t p)
        {
            if (this->PacketType == 0)
            {
                t0.pressure = p;
            }
        }

        void targetid(uint8_t t) {pData[0] = t;}
        void nodeid  (uint8_t n) {pData[1] = n;}
        void flags   (uint8_t f)
        {
            pData[2] = f;
            if (this->PacketType !=0)
                pData[2] |= 0x20;
        }

        uint8_t flags(void)
        {
            return pData[2];
        }

        void count(uint8_t c)
        {
            switch(this->PacketType)
            {
                case 0:
                    t0.count = c;
                    break;
                case 4:
                case 5:
                    pData[3] = c;
                    break;
                default:
                    break;
            }
        }

        void increment_count(void)
        {
            switch(this->PacketType)
            {
                case 0:
                    t0.count++;
                    break;
                case 4:
                    t4.count++;
                    if (t4.count == 0) t4.count_msb++;
                    break;
                case 5:
                    t5.count++;
                    break;
                default:
                    break;
            }
        }

        void supplyV(uint16_t v)
        {
            switch(this->PacketType)
            {
                case 0:
                    t0.supplyV = v;
                    break;
                case 4:
                    t4.supplyV = v;
                    break;
                case 5:
                    t5.supplyV = v;
                    break;
                default:
                    break;
            }
        }

        void temp(uint16_t t)
        {
            switch(this->PacketType)
            {
                case 0:
                    t0.temp = t;
                    break;
                case 4:
                    t4.temp = t;
                    break;
                case 5:
                    t5.temp = t;
                    break;
                default:
                    break;
            }
        }

        void temp1(uint16_t t)
        {
            switch(this->PacketType)
            {
                case 4:
                    t4.temp1 = t;
                    break;
                case 5:
                    t5.temp1 = t;
                    break;
                default:
                    break;
            }
        }

        void temp2(uint16_t t)
        {
            if (this->PacketType == 4)
            {
                t4.temp2 = t;
            }
        }

        void brightness(uint16_t b)
        {
            switch(this->PacketType)
            {
                case 0:
                    t0.brightness = b;
                    break;
                case 5:
                    t5.brightness = b;
                    break;
                default:
                    break;
            }
        }

        int8_t PacketType;
        uint8_t PacketLen;
        uint8_t is_temperature_sensor;
        uint8_t is_humidity_sensor;

        union
        {
            Payload t0;
            PacketType4 t4;
            PacketType5 t5;
        };

        uint8_t *pData;
};


sensors *Sensor=NULL;




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
// SoftwareSerial is NOT by default compatible with PinchangeInterrupt!
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
#include <LowPower.h>

ISR(TIMER2_COMPA_vect)
{
}

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
bool pir_dead_time_expired = false;
bool pir_is_off = true;
// Data pin goes to Config.PCI1Pin



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
//#define USE_I2C_PULLUPS  true // use this for HTU21D in case no Pullups are mounted.
#define USE_I2C_PULLUPS false // use this in case external Pullups are used.

/**********      HTU21D       ******/

#include <HTU21D_SoftwareWire.h>
HTU21D_SoftI2C *myHTU21D =NULL;


static void Start_HTU21D(bool enable)
{
    if (enable)
    {
        #if defined USE_HTU21D || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
        i2c->beginTransmission (HTDU21D_ADDRESS);
        if (i2c->endTransmission() == 0)
        {
            if (Config.SerialEnable) mySerial->print ("HTU21D ");
            myHTU21D = new HTU21D_SoftI2C(i2c);
            delay(100);
            myHTU21D->begin();
            myHTU21D->setResolution(HTU21D_RES_RH10_TEMP13);
        }
        else
        {
            if (Config.SerialEnable) mySerial->print ("not ");
        }
        mySerial->println ("found ");
        #endif
    }
}


static bool Measure_HTU21D(float &temperature, uint16_t &h, Configuration &Config)
{
    #if defined USE_HTU21D || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    if (myHTU21D)
    {
        pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
        digitalWrite(Config.I2CPowerPin, HIGH); // turn Sensor on
        i2c->begin();
        myHTU21D->begin();
        delay(50);
        temperature = myHTU21D->readTemperature();
        h = (uint16_t) floor(myHTU21D->readCompensatedHumidity(temperature) *2 +0.5);

        if (USE_I2C_PULLUPS)
        {
            pinMode(Config.SDAPin, INPUT);
            pinMode(Config.SCLPin, INPUT);
        }
        digitalWrite(Config.I2CPowerPin, LOW); // turn Sensor off
        return true;
    }
    #endif
    return false;
}


/**********     One-Wire and DS18B20     **********/

#include <DallasTemperature.h>       // GNU Lesser General Public License v2.1 or later
#include <OneWire.h>                // license terms not clearly defined.

//#define ONE_WIRE_BUS Config.SDAPin
#define ONE_WIRE_BUS 6
#define ONE_WIRE_POWER Config.I2CPowerPin

OneWire *oneWire=NULL;
DallasTemperature *ds18b20=NULL;


static uint8_t start_ds18b20(DallasTemperature *sensor, byte PowerPin);


// One-Wire DS18B20 start-up sequence
static uint8_t start_ds18b20(DallasTemperature *sensor, byte PowerPin)
{
    #if defined USE_DS18B20 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    pinMode(PowerPin, OUTPUT); // set power pin for DS18B20 to output
    digitalWrite(PowerPin, HIGH); // turn DS18B20 sensor on
    delay(10); // Allow 10ms for the sensor to be ready
    sensor->begin();
    //sensor->setResolution(10); //Resolutiuon is 0.125 deg, absolutely sufficient!
    delay(10); // Allow 10ms for the sensor to be ready
    return sensor->getDeviceCount();
    #else
    return 0;
    #endif
}

static void stop_ds18b20(byte PowerPin)
{
    digitalWrite(PowerPin, LOW); // turn Sensor off to save power
}




static void Start_DS18B20(bool enable)
{
    #if defined USE_DS18B20 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    if(enable)
    {
        //--------------------------------------------------------------------------
        // test if 1-wire devices are present
        //--------------------------------------------------------------------------
        pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
        digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on
        delay(10); // Allow 10ms for the sensor to be ready
        // Start up the library
        oneWire= new OneWire(Config.OneWireDataPin);
        ds18b20 = new DallasTemperature(oneWire);

        uint8_t num_devices = start_ds18b20(ds18b20, ONE_WIRE_POWER);
        if (num_devices == 0)
        {
            delete ds18b20;
            ds18b20 = NULL;
            stop_ds18b20(ONE_WIRE_POWER);
            if (Config.SerialEnable) mySerial->print("no ");

        }
        else
        {
            if (Config.SerialEnable)
            {
                mySerial->print(num_devices, DEC);

            }
        }
        mySerial->println(" DS18B20 found.");
        mySerial->flush();
    }
    #endif
}

uint16_t encode_temp(float t_raw)
{
    return floor(t_raw * 25 + 1000.5);
}

static void Measure_DS18B20(float *temp, uint16_t *temp1=NULL, uint16_t *temp2=NULL)
{
    #if defined USE_DS18B20 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    float temperature=-40;
    if (ds18b20)
    {
        uint8_t num_devices = start_ds18b20(ds18b20, ONE_WIRE_POWER);
        mySerial->print("Num DS18B20: "); mySerial->println(num_devices);

        if (num_devices > 0)
        {
            ds18b20->requestTemperatures();
            switch (num_devices)
            {
                case 3:
                    if (temp2 != NULL)
                    {
                        temperature = ds18b20->getTempCByIndex(2);
                        //*temp2 = floor(temperature * 25 + 1000.5);
                        *temp2 = encode_temp(temperature);
                        mySerial->print("T2: ");mySerial->print(temperature); mySerial->println("º");
                    }
                case 2:
                    if (temp1 != NULL)
                    {
                        temperature = ds18b20->getTempCByIndex(1);
                        *temp1 = encode_temp(temperature);
                        mySerial->print("T1: ");mySerial->print(temperature); mySerial->println("º");
                    }
                case 1:
                    *temp = ds18b20->getTempCByIndex(0);
                    //*temp = floor(temperature * 25 + 1000.5);
                    mySerial->print("T0: ");mySerial->print(*temp); mySerial->println("º");
                    break;
            }

            stop_ds18b20(ONE_WIRE_POWER); // Turn off power Pin for DS18B20
        }
        delay(65); // add delay to allow wdtimer to increase in case its erroneous
    }
    #endif
}



/**********      BME280     **********/
#include <BME280_SoftwareWire.h>
BME280_SoftwareWire *bme=NULL;

static void Start_BME280(bool enable)
{
    #if defined USE_BME280 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    if (enable)
    {
        i2c->beginTransmission (0x76);

        if (i2c->endTransmission() == 0)
        {
            bme = new BME280_SoftwareWire(i2c);
            if (Config.SerialEnable) mySerial->println ("Found BME280");
        }
    }
    #endif
}


static void Measure_BME280(float &temperature, uint16_t &humidity, uint32_t &pressure, byte I2CPowerPin)
{
    #if defined USE_BME280 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    if(bme)
        {
            float hum(NAN), pres(NAN);
            BME280b::TempUnit tempUnit(BME280b::TempUnit_Celcius);
            BME280b::PresUnit presUnit(BME280b::PresUnit_hPa);

            pinMode(I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
            digitalWrite(I2CPowerPin, HIGH);
            i2c->begin();
            delay(20);
            bme->begin();
            delay(125);

            bme->read(pres, temperature, hum, tempUnit, presUnit);
            digitalWrite(I2CPowerPin, LOW);

            humidity = floor(hum +0.5) * 2;
            pressure = long(floor(pres*100));

            if (Config.SerialEnable)
            {
                mySerial->print("Pressure: ");
                mySerial->println(pressure);
                mySerial->flush();
            }
            digitalWrite(I2CPowerPin, LOW);
        }
    #endif
}


/**********      SHT30, SHT31, SHT35     **********/
#if defined USE_SHT3X || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
#include <SHT3x_sww.h>
SHT3x *mySHT3x=NULL;

#endif

static void Start_SHT3X(bool enable)
{
    if (enable)
    {
        #if defined USE_SHT3X || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
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
}


static void Measure_SHT3x(float &temperature, uint16_t &humidity)
{
    #if defined USE_SHT3X || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    if (mySHT3x)
        {
            pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
            digitalWrite(Config.I2CPowerPin, HIGH); // turn Sensor on
            delay(250); // wait for the Sensor to power up completely.
            //i2c->begin();
            mySHT3x->Begin();
            mySHT3x->GetData();
            temperature = mySHT3x->GetTemperature();
            humidity = (mySHT3x->GetRelHumidity()*2);
            digitalWrite(Config.I2CPowerPin, LOW); // turn Sensor off
        }
    #endif
}


/**********      MAX31865     **********/
#include "MAX31865.h"
#include <SPI.h>
#define PT100

// Chip Select Pin
#define RTD_CS_PIN  A0
#define RTD_PWR_PIN A1

#define FAULT_HIGH_THRESHOLD  0x9304  /* +350C */
#define FAULT_LOW_THRESHOLD   0x2690  /* -100C */

MAX31865_RTD *rtd=NULL;



static void Start_MAX31865(bool enable)
{
    #if defined USE_MAX31865 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
    if (enable)
    {
        pinMode(RTD_CS_PIN, INPUT_PULLUP);  // SPI SS (avoid becoming SPI Slave)

        if (Config.SerialEnable) mySerial->println("start MAX31865");

        #ifdef PT1000
        // For PT 1000 (Ref on breakout board = 3900 Ohms 0.1%)
        rtd= new MAX31865_RTD( MAX31865_RTD::RTD_PT1000, RTD_CS_PIN, 3900 );
        #endif

        #ifdef PT100
        // For PT 100  (Ref Ref on breakout board = 430 Ohms 0.1%)
        rtd= new MAX31865_RTD( MAX31865_RTD::RTD_PT100, RTD_CS_PIN, 430 );
        #endif
    }
    #endif
}

#if defined USE_MAX31865 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
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

            mySerial->print(F( "RTD:")); mySerial->print(rtd->resistance());
            mySerial->print(F( " Ohm, Temp: "));
            mySerial->print( *pt100_temp,2); mySerial->println("º");
        }
        else
        {
            mySerial->println("MAX31865 Failure.");
        }

        digitalWrite(RTD_PWR_PIN, 0);
    }

    return status;
}

#endif


/*****************************************************************************/
/***      Brightness  with LDR                                             ***/
/*****************************************************************************/
static uint16_t brightness_with_LDR(int8_t LDRPin) // must ba a analog Pin!
{
    uint16_t sensorValue=0;

    if (LDRPin >= 0)
    {
        pinMode(LDRPin, INPUT_PULLUP);
        delay(20);
        sensorValue = (uint16_t)(1023 - analogRead(LDRPin));
        pinMode(LDRPin, INPUT);
    }
    return sensorValue;
}



/*****************************************************************************/
/****                         blink                                       ****/
/*****************************************************************************/
void activityLed (unsigned char state, unsigned int time = 0)
{
  if (Config.LedPin)
  {
    pinMode(Config.LedPin, OUTPUT);
    if (time == 0)
    {
      digitalWrite(Config.LedPin, state);
    }
    else
    {
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
static bool setup_timer2(period_t st=SLEEP_8S);
static bool setup_timer2(period_t st)
{
    unsigned long t = millis()+1200;
  // clock input to timer 2 from XTAL1/XTAL2
  ASSR = bit (AS2);

  // wait for ASSR with timeout = 1200ms
  while ( (ASSR & 0x1f) && (millis() < t) );

  if (ASSR & 0x1f) // still busy, probably no clock from XTAL1/XTAL2
  {
      return false;  // no crystal, cannot setup the timer
  }

  TCCR2A = bit (WGM21);     //WGM20 and WGM22 in TCCR2B is set 0, Mode of operation: CTC
  switch (st)
  {
    case SLEEP_FOREVER:
        TCCR2B  &= 0xF8; // timer off, CS0=0, CS1=0, CS2=0
        break;

    case SLEEP_8S:
        TCCR2B = bit (CS22) | bit (CS21) | bit (CS20);   //111b
        break;

    case SLEEP_2S:
        TCCR2B = bit (CS22) | bit (CS21);   // 110b
        break;

    case SLEEP_1S:    // 128
        TCCR2B = bit (CS22) | bit (CS20); //101b
        break;

    case SLEEP_500MS:// 64
        TCCR2B = bit (CS22); //100b
        break;

    case SLEEP_250MS:// 32
        TCCR2B = bit (CS21) | bit (CS20); //011b
        break;
    case SLEEP_60MS: // 8
        TCCR2B = bit (CS21); //010b
        break;

    case SLEEP_15MS: // 1
        TCCR2B = bit (CS21) | bit (CS20); //001b
        break;
  }

  while (ASSR & 0x08);   // wait until OCR2A can be updated
  OCR2A =  255;         // count to 255 (zero-relative)
  while (ASSR & 0x1f);   // update is busy

  // enable timer interrupts
  TIMSK2 |= bit (OCIE2A);
  //mySerial->flush();

  return true;
}


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

/*****************************************************************************/
// should go into configuration class
/*
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
*/
uint8_t tinytx_size;


#define SENDDELAY_M
#define TEMP_DELTA_ALARM 10 // 1 degC


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
        //mySerial->print("OSCCAL: 0x"); mySerial->println(OSCCAL,HEX);
    }

    /***                     ***/
    /***     CALIBRATE?      ***/
    /***                     ***/
    CalMode.configure();

    //mySerial->print("Nodeid = "); mySerial->println(Config.Nodeid);
    //if (Config.SerialEnable) print_eeprom(mySerial); // use this only for debug purposes!

    //UseBits* u;
    //u = (UseBits*)&Config.SensorConfig;
    //Sensor = new sensors(*u);
    Sensor = new sensors(*((UseBits*)&Config.SensorConfig));

    // for debug purposes
    mySerial->print("type: "); mySerial->println(Sensor->PacketType);
    mySerial->print("leng: "); mySerial->println(Sensor->PacketLen);


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


    if (Config.SerialEnable)
    {
        mySerial->print("USING ");
        Config.UseCrystalRtc ? mySerial->println ("CRYSTAL TIMER") : mySerial->println ("WDT");
    }

    pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
    digitalWrite(Config.I2CPowerPin, HIGH);
    delay(5);
    i2c = new SoftwareWire( Config.SDAPin, Config.SCLPin, USE_I2C_PULLUPS );
    i2c->setClock(10000L);
    i2c->begin();


    Start_HTU21D  (Sensor->use.HTU21D);
    Start_BME280  (Sensor->use.BME280);
    Start_SHT3X   (Sensor->use.SHT3X);
    Start_DS18B20 (Sensor->use.DS18B20);
    Start_MAX31865(Sensor->use.MAX31865);


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

    Sensor->nodeid(Config.Nodeid);
    Sensor->targetid(Config.Gatewayid);
    //Sensor->flags(1); // LSB means "heartbeat"
    Sensor->humidity(0);

    if(!Config.UseCrystalRtc)
        enableADC(false);           // power down/disable the ADC
    else
        setup_timer2();

    analogReference(INTERNAL);  // Set the aref to the internal 1.1V reference
    watchdog_counter = 6500;     // set to have an initial transmission when starting the sender.



    if (Config.PirPowerPin >=0)
    {
        pinMode(Config.PirPowerPin,OUTPUT);
        digitalWrite(Config.PirPowerPin, HIGH);
        pir_is_off = false;

        // avoid triggering the PCINT when turning on power on PIR
        LowPower.powerDown(SLEEP_500MS , ADC_OFF, BOD_OFF);
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);

        event_triggered &= 0xFD; //1111 1101 clear bit 1, this is PCI1 from PIR Sensor
    }

    mySerial->flush();

    if (Config.LedPin)
    {
        activityLed(1,1000); // LED on
    }
}

void loop()
{
    static bool timer2_setup_done = false;

    // go to sleep. When waking up, restart from here.
    if (Config.Senddelay != 0)
    {
        if (Config.UseCrystalRtc)
        {
            LowPower.powerSave(SLEEP_FOREVER, ADC_OFF, BOD_OFF, TIMER2_ON); // wake up on timer2 counter overflow
        }
        else // use WDT
        {
            LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        }
    }
    else // Senddelay is 0, external timer or no timer (like in a remote control)
    {
        if (Config.UseCrystalRtc)
        {
            timer2_setup_done = true;
        }
        TCCR2B &= 0xF8; // timer 2 off, CS0=0, CS1=0, CS2=0
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }


    if (!event_triggered)
    {
        watchdog_counter++;
    }


    // activate PIR after it has triggered some time ago (the dead time)
    if (Config.PirPowerPin >=0)
    {
        pir_dead_time_expired = (watchdog_counter >= Config.PirDeadTime) && pir_is_off; // if true, turn on VCC of PIR and wait 2.5s
        if (pir_dead_time_expired)
        {
            pir_is_off = false;
            digitalWrite(Config.PirPowerPin, HIGH);
            pir_dead_time_expired = false;
            if (Config.SerialEnable) { Serial.println("PIR active."); Serial.flush(); }

            // when PIR ist switched ON, event triggers and we need to wait for 2.5s because data pin goes high for 2.5s
            LowPower.powerDown(SLEEP_500MS , ADC_OFF, BOD_OFF);
            LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);

            event_triggered &= 0xFD; //1111 1101 clear bit 1, this is PCI1 from PIR Sensor
        }
    }

    watchdog_expired = ((watchdog_counter >= Config.Senddelay) && (Config.Senddelay != 0)); // when Senddelay equals 0, sleeep forever and wake up only for events.
    if (watchdog_expired || event_triggered)
    {
        uint8_t Flags = 0;
        float temperature=0;
        uint16_t humidity =0;

        if (!Config.UseCrystalRtc)
        {
            enableADC(true); // power up/enable the ADC
        }

        if (watchdog_expired)
        {
            // reset the watchdog counter
            watchdog_counter = 0;

            // there is a strange behaviour of the Atmega328, we need to redo the timer 2 setup once.
            if (Config.UseCrystalRtc && !timer2_setup_done)
            {
                timer2_setup_done =true;
                setup_timer2(); // this restarts the timer
            }

            // set the heartbeat flag when the watchdog counter is expired
            Flags |= 0x01;
        }

        Sensor->targetid(Config.Gatewayid);

        if(event_triggered)
        {
            Flags |= (event_triggered << 1);
            //um Fernbedienungen mit mehreren Kanaelen herzustellen, muss der Node
            //Pakete an verschiedene Nodeid's schicken koennen.
            switch (event_triggered)
            {
                case 0x01:
                    Sensor->targetid(Config.PCI0Gatewayid);
                    break;
                case 0x02:
                    Sensor->targetid(Config.PCI1Gatewayid);
                    break;
                case 0x04:
                    Sensor->targetid(Config.PCI2Gatewayid);
                    break;
                case 0x08:
                    Sensor->targetid(Config.PCI3Gatewayid);
                    break;
                default:
                    break;
            }
        }
        Sensor->flags(Flags);

        // PIR Sensor triggered: turn off Sensor for PIR_DEAD_TIME seconds
        if (Config.PirPowerPin >=0)
        {
            if (event_triggered & 0x2)//PCI1
            {
                pir_is_off = true;
                digitalWrite(Config.PirPowerPin, LOW);
                watchdog_counter =0;
                if (Config.SerialEnable)
                {
                    Serial.println("PIR triggered."); Serial.flush();
                }
                blink(Config.LedPin, 3);
            }
        }

        long Vcal_x_ADCcal = (long)Config.AdcCalValue * Config.VccAtCalmV;
        #ifdef USE_RADIO
            Sensor->supplyV( Vcal_x_ADCcal / radio.vcc_dac );  // the VCC measured during last TX Burst.
        #else
            Sensor->supplyV( Vcal_x_ADCcal / readVcc() );
        #endif

        Sensor->increment_count();

        if(Sensor->use.HTU21D)
        {
            Measure_HTU21D(temperature, humidity, Config);
        }

        if (Sensor->use.SHT3X)
            Measure_SHT3x(temperature, humidity);

        if(Sensor->use.BME280)
        {
            uint32_t p;
            Measure_BME280(temperature, humidity, p, Config.I2CPowerPin);
            Sensor->pressure(p);
        }

        if (Sensor->use.DS18B20)
        {
            switch (Sensor->PacketType)
            {
                case 5:
                    {
                        float t;
                        Measure_DS18B20(&t);
                        Sensor->temp1(encode_temp(t));
                    }
                    break;
                case 4:
                    {
                        uint16_t t1,t2;
                        Measure_DS18B20(&temperature, &t1, &t2);
                        Sensor->temp1(t1);
                        Sensor->temp2(t2);
                    }
                    break;
                default:
                    break;
            }
        }


        if (Sensor->use.MAX31865) // PT100 is not by default available in classic TiNo with Atmega328P
        {
            #if defined USE_MAX31865 || defined __AVR_ATmega644P__ || defined (__AVR_ATmega1284P__)
            float t=0;
            uint8_t status = Measure_MAX31865(&t);
            Sensor->temp1(encode_temp(t));
            if(Config.SerialEnable)
            {
                mySerial->print("RTD status: ");
                mySerial->println(status);
            }
            #endif
        }

        if(Sensor->use.BRIGHTNESS && Config.LdrPin >= 0)
        {
            uint16_t brightness = brightness_with_LDR(Config.LdrPin);
            Sensor->brightness( brightness );
            mySerial->print("LDR: ");
            mySerial->println( brightness );
        }

        if (!Sensor->is_temperature_sensor)
        {
            mySerial->println("no temp sensor");
            #ifdef USE_RADIO
                temperature = radio.readTemperature(0) + Config.radio_temp_offset/10.0;
            #else
                temperature = 12.34;
            #endif
            if (Config.Senddelay != 0) delay(65); // add delay to allow wdtimer to increase in case its erroneous
        }

        Sensor->temp( encode_temp(temperature) );
        Sensor->humidity(humidity);

        if (Config.SerialEnable)
        {
            if (Sensor->is_humidity_sensor)
            {
                mySerial->print(humidity/2.0);
                mySerial->print(" %RH\t");
            }

            mySerial->print(temperature, 2); mySerial->println("º");
        }


        bool ackreceived = false;
        #ifdef USE_RADIO
        temperature<0?  temperature-=0.5 : temperature+=0.5;

        if(Config.UseRadioFrequencyCompensation)
        {
            ackreceived = Mac.radio_send(Sensor->pData, Sensor->PacketLen, Config.RequestAck, int(temperature));
        }
        else
        {
            ackreceived = Mac.radio_send(Sensor->pData, Sensor->PacketLen, Config.RequestAck);
        }

        if (Config.RequestAck && Config.SerialEnable)
            (ackreceived) ? mySerial->println("Ack received.") : mySerial->println(" No Ack received.");
        #endif

        //#ifndef USE_CRYSTAL
        if (!Config.UseCrystalRtc)
            enableADC(false); // power down/disable the ADC
        //#endif

        event_triggered = 0;


        if (ackreceived)
        {
            activityLed (1, 100);
        }

        if (Config.LedPin && Config.LedCount >0)
        {

            if (Config.LedCount != 0xff) Config.LedCount--;
            blink(Config.LedPin, 2); // blink LED
        }
        if (Config.SerialEnable) mySerial->flush();
    }
}
