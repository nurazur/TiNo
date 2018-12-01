// Wireless Sensor for HTU21D / SHT21
// built for AVR ATMEGA328P
// optional possibility to use  a 32.768 crystal. 
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

// Uses LowPower      from  https://github.com/rocketscream/Low-Power under creative commons attribution-sharealike 3.0 unported license. see https://creativecommons.org/licenses/by-sa/3.0/
// Uses HTU21D_SoftI2C modified from https://github.com/enjoyneering/HTU21D under BSD licence, rewritten by nurazur for use of SoftwareWire
// Uses SoftwareWire from https://github.com/Testato/SoftwareWire under GNU General Public License v3.0
// Uses a RFM69 Driver originally from LowPowerlab.com However this driver has been changed so much so that it is barely recognizable. 
// Uses PinChangeInterrupt from https://github.com/NicoHood/PinChangeInterrupt under open source license, exact type is not specified but looks like GNU 


#define FILENAME "Send_HTU21D.ino 23/11/2018"
#define BUILD 7


#include "pins_arduino.h"
#define USE_RFM69

#ifndef USE_RFM69
    #include <RFM12B_AH.h>
#else
	#include <RFM69.h>
	#include <RFM69registers.h> 
#endif

/*****************************************************************************/
/***  MAC   ***/
/*****************************************************************************/
#include <Mac.h>


/*****************************************************************************/
/*** Sleep mode choice ***/
/*****************************************************************************/
int16_t watchdog_counter;
bool watchdog_expired = false;
//#define USE_CRYSTAL // invoked by compiler option
#ifdef USE_CRYSTAL
    #include "LowPower.h"
    // interrupt on Timer 2 compare "A" completion
    // requires a 32.768kHz Crystal
    //EMPTY_INTERRUPT (TIMER2_COMPA_vect);
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
void wakeUp0()
{
	event_triggered |= 0x1;
}


void wakeUp1() 
{
	event_triggered |= 0x2;
}


void wakeUp2()
{
	event_triggered |= 0x4;
}


void wakeUp3() 
{
	event_triggered |= 0x8;
}


/*****************************************************************************/
/*** Sensor Drivers ***/
/*****************************************************************************/
#include "SoftwareWire.h" 
#include "HTU21D_SoftwareWire.h"


/*****************************************************************************/
/***  EEPROM Access  and device calibration ***/
/*****************************************************************************/
#include <configuration_dist.h>
#include <calibrate_dist.h>


/*****************************************************************************/
/***                            Encryption                                 ***/
/*****************************************************************************/
//encryption is OPTIONAL by compilation switch
//to enable encryption you will need to:
// - provide a 16-byte encryption KEY (same on all nodes that talk encrypted)
// - set the varable ENCRYPTION_ENABLE to 1 in the EEPROM, at runtime in Cfg.EncryptionEnable
#define KEY  "TheQuickBrownFox"

/*****************************************************************************/
/***  Serial Port   ***/
/*****************************************************************************/
// SoftwareSerial is NOT by default compatible with PinchangeInterrupt! see http://beantalk.punchthrough.com/t/pinchangeinterrupt-with-softwareserial/4381/4
//#define SOFTSERIAL 
#define SERIAL_BAUD     4800
#ifdef SOFTSERIAL
    #include <SoftwareSerial.h>
    #define rxPin 0 // 
    #define txPin 1 //  pin der an RXD vom PI geht.
    SoftwareSerial *mySerial;
#else
    HardwareSerial *mySerial = &Serial;
#endif


/*****************************************************************************/
/***                            Radio Driver Instance                      ***/
/*****************************************************************************/
#ifdef USE_RFM69
    RFM69 radio;
#else
    RFM12B radio;
#endif

/*****************************************************************************/
/***                   Device specific Configuration                       ***/
/*****************************************************************************/
Configuration Config;


/*****************************************************************************/
/***  MAC  Instance                                                        ***/
/*****************************************************************************/
myMAC Mac(radio, Config, (uint8_t*) KEY);



/*****************************************************************************/
/***                   Device specific Configuration                       ***/
/*****************************************************************************/
Calibration CalMode(Config, mySerial, &Mac, BUILD, (uint8_t*) KEY);



/*****************************************************************************/
/***                            I2C and HTU21D Driver Instances            ***/
/*****************************************************************************/
// SHT21/HTU21D connects through SoftwareWire, so that SCL and SDA can be any pin
// Add pullup resistors between SDA/VCC and SCL/VCC if not yet provided on Module board

HTU21D_SoftI2C *myHTU21D =NULL;
SoftwareWire *i2c = NULL;


/*****************************************************************************/
/***                            Packet Format                              ***/
/*****************************************************************************/

Payload tinytx; // the data we want to transmit


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
void blink (byte pin, byte n = 3)
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
/***  Read VCC by taking measuring the 1.1V reference and taking VCC as    ***/
/***  reference voltage. Set the reference to Vcc and the measurement      ***/
/***  to the internal 1.1V reference                                       ***/
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
  //result = 1125300L / result; // Back-Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  //result = VREF / result;
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

void setup_timer2()
{
  //cli();
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
  //sei();
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

// send ATtiny into Power Save Mode
void goToSleep() 
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode.
  sleep_enable(); // Enable sleep mode.
  sleep_mode(); // Enter sleep mode.

  // After waking from watchdog interrupt the code continues to execute from this point.

  sleep_disable(); // Disable sleep mode after waking.
}


void setup_watchdog(int timerPrescaler) {
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
 



const char* show_trigger(byte trigger)
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
   
    if (Config.SerialEnable)print_eeprom(mySerial);
    
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
    i2c = new SoftwareWire( Config.SDAPin, Config.SCLPin, 0 );
    i2c->setClock(10000L);
    i2c->begin();
    
    i2c->beginTransmission (HTDU21D_ADDRESS);
    if (i2c->endTransmission() == 0)
    {
        if (Config.SerialEnable) mySerial->println ("Found HTU21D");
        myHTU21D = new HTU21D_SoftI2C(i2c);
        delay(100);
        myHTU21D->begin();
    }
    else
    {
        blink(Config.LedPin, 3); 
        if (Config.SerialEnable) mySerial->println ("Could not find a  HTU21D");
    }
        

    digitalWrite(Config.I2CPowerPin, LOW);
    
    
    Mac.radio_begin(); // puts radio to sleep to save power.
    
    
    tinytx.targetid = Config.Gatewayid;
    tinytx.nodeid =   Config.Nodeid;
    tinytx.flags =    1; // LSB means "heartbeat"
 
 
#ifndef USE_CRYSTAL
    setup_watchdog(watchdog_wakeup);      // Wake up after 8 sec
    PRR = bit(PRTIM1);          // only keep timer 0 going
    enableADC(false);           // power down/disable the ADC
#else
    setup_timer2();
#endif
    analogReference(INTERNAL);  // Set the aref to the internal 1.1V reference
    watchdog_counter = 500;     // set to have an initial transmission when starting the sender.
  
    if (Config.LedPin && myHTU21D) 
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
    
    watchdog_expired = ((watchdog_counter >= (int16_t)Config.Senddelay) && (Config.Senddelay != 0)); // when Senddelay equals 0, sleeep forever and wake up only for events.
    
    if (watchdog_expired || event_triggered) 
    {
        #ifndef USE_CRYSTAL  
		enableADC(true); // power up/enable the ADC
        #endif
		tinytx.flags = 0;
		
		if (watchdog_expired)
		{			
			tinytx.flags |= 0x1; // heartbeat flag
            watchdog_counter = 0; 
		}
				
		if(event_triggered)
		{
			tinytx.flags |= (event_triggered << 1);
		} 
	
       
        long vref = (long)Config.AdcCalValue * Config.VccAtCalmV;
        tinytx.supplyV  = vref / readVcc(); 
        tinytx.count++;

        if (myHTU21D)
        {              
            pinMode(Config.I2CPowerPin, OUTPUT);  // set power pin for Sensor to output
            digitalWrite(Config.I2CPowerPin, HIGH); // turn Sensor on
            i2c->begin();
            myHTU21D->begin();
            delay(50); 
            
            float temperature = myHTU21D->readTemperature();
            tinytx.temp = (temperature * 25 + 1000);  
            tinytx.humidity = (myHTU21D->readCompensatedHumidity(temperature)*2);
            digitalWrite(Config.I2CPowerPin, LOW); // turn Sensor off
        }
        else
        {
            tinytx.temp = (radio.readTemperature(0) * 25 + 1000); 
            tinytx.humidity = 240;          // 120 % RH to indicate ist a false number
            if (Config.Senddelay != 0) delay(65); // add delay to allow wdtimer to increase in case its erroneous
        }
        
        if (Config.SerialEnable)
        {
            mySerial->println("HTU21D:");
            mySerial->print(tinytx.humidity/2.0);
            mySerial->print(" %RH\t");
            mySerial->print(tinytx.temp/25-40); mySerial->println(" degC");
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
        
        
        
        
        bool ackreceived = Mac.radio_send(tinytx, Config.RequestAck);
        if (Config.RequestAck && Config.SerialEnable)
            (ackreceived) ? mySerial->println("Ack received.") : mySerial->println(" No Ack received.");

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
        mySerial->flush();
	}
}

