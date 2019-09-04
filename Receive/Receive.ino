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



// Gateway for the TiNo Wireless Sensor System
//
// Based originally on work from Nathan Chantrell
// modified by meigrafd @2013 - for UART on RaspberryPI
// extended by nurazur nurazur@gmail.com 2014 - 2019
// the RF protocol is completely binary, see https://github.com/nurazur/tino


#define SKETCHNAME "TiNo 2.0.0 receive.ino Ver 02/09/2019"
#define BUILD 9


#include <avr/sleep.h>

#define SERIAL_BAUD 38400
#ifdef SOFTSERIAL
    #define RX_PIN 0
    #define TX_PIN 1
    #include <SoftwareSerial.h>
    SoftwareSerial swserial(RX_PIN, TX_PIN);
    SoftwareSerial *mySerial = &swserial;
#else
    HardwareSerial *mySerial = &Serial;
#endif


#include "configuration.h"
#include <datalinklayer.h>
#include <RFM69.h>
#include <EEPROM.h>
#include "calibrate.h"

#define KEY  "TheQuickBrownFox"

/*****************************************************************************/
/***                       Actions                                         ***/
/*****************************************************************************/
typedef struct {
  uint8_t node;      // the node to trigger
  uint8_t port;      // the port to trigger
  uint8_t mask;      // to which bit of the flag byte it is mapped
  uint8_t onoff;    // type of action:  OFF (00) ON(01) TOGGLE (10) PULSE (11); Pulse length = 2^B, where B = bits [23456], B= (action[i] >>2) &0xf, C = default on power up, C = bit[7] 1= on, 0 = off
} action;

#define ADR_NUM_ACTIONS 318
#define ADR_ACTIONS   ADR_NUM_ACTIONS + 1
#define MAX_NUM_ACTIONS 40
action *actions;
uint8_t num_actions;
/*
if action.node is equal to this node, then the flag word in the telegram is evaluated.
4 flags can be set (bits 1 to bit 4 in the flag byte). Each flag is mapped to a port using the mask byte.
the port is defined in the port[i] array (up to 4 different ports for the four flags)
Actions are: ON, OFF, TOGGLE Port, PULSE with length 2^B * 0.5 seconds.

Example: Node 15,  Port 7 shall turn on on flag 02, port 7 shall turn off on flag 04. Port 8 shall be toggled on flag 0x08, Port 5 shall be pulsed for 2 seconds on flag 0x10

node = 15;
port = 7;
mask = 0x02;
onoff = 0x1;

node = 15;
port = 7;
mask = 0x04;
onoff = 0x0;

node = 15;
port = 8;
mask = 0x08;
onoff = 0x10;

node = 15;
port = 5;
mask = 0x10;
onoff = 0x3 | (0x02 << 2); // B=2, 2^B = 4 * 0.5s = 2s
*/

unsigned char pulse_port=0;  // the port for which a pulse is defined and active.
unsigned int pulse_duration=0; // counter


void doaction (uint8_t nodeid, uint8_t flags, action actions[], uint8_t num_actions)
{
    for (uint8_t i=0; i< num_actions; i++)
    {
        if(actions[i].node == nodeid) // nodeid matches the actions's nodeId
        {
            if (actions[i].mask & flags ) // the action's trigger bit is set
            {
                uint8_t act = actions[i].onoff &0x3; // Action mode
                switch (act)
                {
                    case 0:     // OFF
                        digitalWrite(actions[i].port, 0);
                        break;
                    case 1:     // ON
                        digitalWrite(actions[i].port, 1);
                        break;
                    case 2:     // Toggle
                        digitalWrite(actions[i].port, !digitalRead(actions[i].port));
                        break;
                    case 3:     //Pulse
                        digitalWrite(actions[i].port, 1);
                        pulse_port = actions[i].port;

                        uint8_t exp = (actions[i].onoff>>2) &0x7F;

                        if (exp <= 4) // less than or equal 8 seconds
                        {
                            pulse_duration  =1;
                            setup_watchdog(5+exp);
                        }
                        else // more than  8s
                        {
                            pulse_duration = pow(2,exp-4);
                            setup_watchdog(9); //8s
                        }
                        break;
                }
            }
        }
    }
}

void setup_watchdog(int timerPrescaler)
{
    if (timerPrescaler > 9 ) timerPrescaler = 9; //Correct incoming amount if need be
    byte bb = timerPrescaler & 7;
    if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary
    //This order of commands is important and cannot be combined
    MCUSR &= ~(1<<WDRF); //Clear the watchdog reset
    WDTCSR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
    WDTCSR = bb; //Set new watchdog timeout value
    WDTCSR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}

void stop_watchdog(void)
{
    noInterrupts();
    //This order of commands is important and cannot be combined
    MCUSR &= ~(1<<WDRF); //Clear the watchdog reset
    WDTCSR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
    WDTCSR = 0; //Set new watchdog timeout value
    interrupts();
}

ISR(WDT_vect)
{
    pulse_duration--;
    if (pulse_duration == 0)
    {
        if (pulse_port >0) digitalWrite(pulse_port, 0);
        pulse_port = 0;
        stop_watchdog();
    }
}

/*****************************************************************************/
/***                      Class instances                                  ***/
/*****************************************************************************/
RFM69 radio;
Configuration config;
myMAC Mac(radio, config, (uint8_t*) KEY, mySerial);
Calibration CalMode(config, mySerial, &Mac, BUILD, (uint8_t*) KEY);
/*****************************************************************************/




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
/****                         blink                                       ****/
/*****************************************************************************/
void activityLed (unsigned char state, unsigned int time = 0)
{
  if (config.LedPin)
  {
    pinMode(config.LedPin, OUTPUT);
    if (time == 0)
    {
      digitalWrite(config.LedPin, state);
    }
    else
    {
      digitalWrite(config.LedPin, state);
      delay(time);
      digitalWrite(config.LedPin, !state);
    }
  }
}


/*****************************************************************************/
/*
     Read VCC by  measuring the 1.1V reference and taking VCC as reference voltage.
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
  //result = 1125300L / result; // Back-Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  //result = VREF / result;
  return result;
}


float getVcc(long vref)
{
    return (float)vref / readVcc();
}

/**********************************************************************/





// init Setup
void setup()
{
    /***  INITIALIZE SERIAL PORT ***/

    #ifdef SOFTSERIAL
        pinMode(RX_PIN, INPUT);
        pinMode(TX_PIN, OUTPUT);
    #endif

    mySerial->begin(SERIAL_BAUD);
    mySerial->println(SKETCHNAME);


    /***                     ***/
    /***     CALIBRATE?      ***/
    /***                     ***/
    CalMode.configure();

    /*************  INITIALIZE ACTOR MODULE ******************/
    EEPROM.get(ADR_NUM_ACTIONS, num_actions);


    if (num_actions > 0 && num_actions <= MAX_NUM_ACTIONS)
    {
        mySerial->print("number of actions: ");mySerial->println(num_actions);
        actions = new action[num_actions];
    }
    else
    {
        num_actions = 0;
        actions = NULL;
    }

    for(int i=0; i< num_actions; i++)
    {
        EEPROM.get(ADR_ACTIONS + i*sizeof(action), actions[i]);
    }

    // verify the checksum
    uint16_t cs_from_eeprom;
    EEPROM.get(ADR_ACTIONS + MAX_NUM_ACTIONS * sizeof(action), cs_from_eeprom);
    uint16_t cs_from_data = CalMode.checksum_crc16((uint8_t*) actions, sizeof(action) * num_actions);

    if ((cs_from_eeprom ^ cs_from_data) != 0)
    {
        mySerial->println("Checksum incorrect for Actions.");
        num_actions =0;
    }

    for(int i=0; i< num_actions; i++)
    {
        mySerial->print("action "); mySerial->print(i);mySerial->print("for Node: "); mySerial->print(actions[i].node);
        mySerial->print(", Port: "); mySerial->print(actions[i].port);
        pinMode(actions[i].port, OUTPUT);

        digitalWrite(actions[i].port, actions[i].onoff>>7);
        mySerial->print(", Mask: "); mySerial->print(actions[i].mask);
        mySerial->print(", OnOff: "); mySerial->print(actions[i].onoff);

        mySerial->println();
    }

    /*************  INITIALIZE PIN CHANGE INTERRUPTS ******************/
    if (config.PCI0Pin >=0)
    {
        pinMode(config.PCI0Pin, (config.PCI0Trigger >>2) );  // set the pin to input or input with Pullup
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(config.PCI0Pin), wakeUp0, config.PCI0Trigger&0x3);
    }
    if (config.PCI1Pin >=0)
    {
        pinMode(config.PCI1Pin, (config.PCI1Trigger >>2));  // set the pin to input
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(config.PCI1Pin), wakeUp1, config.PCI1Trigger&0x3);
    }
    if (config.PCI2Pin >=0)
    {
        pinMode(config.PCI2Pin, (config.PCI2Trigger >>2));  // set the pin to input
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(config.PCI2Pin), wakeUp2, config.PCI2Trigger&0x3);
    }
    if (config.PCI3Pin >=0)
    {
        pinMode(config.PCI3Pin, (config.PCI3Trigger >>2));  // set the pin to input
        attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(config.PCI3Pin), wakeUp3, config.PCI3Trigger&0x3);
    }

    /***********************************************************/
    // normal initialization starts here
    // Dont load eeprom data here, it is now done in the configuration (boot) routine.EEPROM could be encrypted.
    //EEPROM.get(0, config);

     /***  INITIALIZE RADIO MODULE ***/
    mySerial->print("RF Chip = "); config.IsRFM69HW ?    mySerial->print("RFM69HCW") : mySerial->print("RFM69CW");  mySerial->println();
    mySerial->print ("FDEV_STEPS: ");mySerial->print(config.FedvSteps);mySerial->println();

    Mac.radio_begin();  // re-initialize radio

  if (config.LedPin)
  {
      activityLed(1, 1000); // LED on
  }
}


void loop()
{
    Mac.radio_receive(false); // non- blocking
    {
        //if (Mac.rxpacket.success || Mac.rxpacket.payload[NODEID] !=0) // show all packets, even erroneous ones
        if (Mac.rxpacket.success) // only show goos packets
        {
            // common for all formats
            mySerial->print(Mac.rxpacket.payload[NODEID],DEC);mySerial->print(" ");

            // find out which protocol format is used
            if (!(Mac.rxpacket.payload[FLAGS] & 0x20)) // bit 5 in Flags is 0, flags is xx0x xxxx
            {
                // This is the standard protocol for TiNo Senors or actors
                Payload *pl = (Payload*) Mac.rxpacket.payload;
                mySerial->print("v=");  mySerial->print(pl->supplyV);
                mySerial->print("&c=");  mySerial->print(pl->count);
                mySerial->print("&t=");  mySerial->print((pl->temp - 1000)*4);
                mySerial->print("&h=");  mySerial->print(int(pl->humidity/2.0*100));

                mySerial->print("&intr=0x");
                int intpts =0;
                if (pl->flags & 0x2) intpts |= 0x1;
                if (pl->flags & 0x4) intpts |= 0x1<<2;
                if (pl->flags & 0x8) intpts |= 0x1<<4;
                if (pl->flags & 0x10) intpts |= 0x1<<6;
                mySerial->print(intpts,HEX);
                if (Mac.rxpacket.errorcode >=0) doaction(Mac.rxpacket.payload[NODEID], Mac.rxpacket.payload[FLAGS], actions, num_actions);
            }
            else
            {
                // it is another packet type.
                // Only data in the switch statement are interesting.
                mySerial->print(Mac.rxpacket.payload[TARGETID]); mySerial->print(";");
                mySerial->print(Mac.rxpacket.payload[NODEID]); mySerial->print(";");
                mySerial->print("0x");mySerial->print(Mac.rxpacket.payload[FLAGS],HEX); mySerial->print(";");

                switch (Mac.rxpacket.payload[FLAGS] & 0x0f)
                {
                    case 1:
                        // string packet with length 16 (so we've got 13 bytes effective)
                        config.FecEnable ? Mac.rxpacket.payload[8] = 0 : Mac.rxpacket.payload[16] = 0;
                        mySerial->print((char*)(Mac.rxpacket.payload+3)); mySerial->print(";");
                        break;
                    case 2:
                        // string packet with length 24 (so we've got 21 bytes effective)
                        Mac.rxpacket.payload[24] = 0;
                        mySerial->print((char*)(Mac.rxpacket.payload+3)); mySerial->print(";");
                        break;
                    default:
                        mySerial->print("oups!;");
                }

            }
            mySerial->print("&rssi=");     mySerial->print(int(Mac.rxpacket.RSSI*10));
            mySerial->print("&fo=");    mySerial->print(Mac.rxpacket.FEI, DEC);
            mySerial->println("");
            Mac.rxpacket.payload[NODEID] =0;
        }
        else
        {
         /* Errorcodes:
             -1:  could not decode FEC data (too many bit errors in codes)
             -2:  data length does not match
             -3:  not my message or address corrupted
         */
        //mySerial->print("Error Code: "); mySerial->print(Mac.rxpacket.errorcode);
        //mySerial->print(" NodeId,"); mySerial->print(Mac.rxpacket.payload[NODEID],DEC);
        //mySerial->print(",s=");     mySerial->print(Mac.rxpacket.RSSI); mySerial->println();
        }
    }
    if (event_triggered) // a local Port change interrupt occured.
    {
        //mySerial->print("Event triggered: "); mySerial->print(event_triggered);
        //mySerial->print(", Nodeid: "); mySerial->print(config.Nodeid);
        //mySerial->println();

        doaction(config.Nodeid, event_triggered, actions, num_actions);
        event_triggered =0;
    }

}




