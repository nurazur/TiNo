// Data Link Layer for Nodes and Gateways using RFM69CW or HCW or CC1101

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
// Version Build 9
#ifndef mymac
#define mymac

#include <Arduino.h>
#include <GenericRadio.h>

#include "codec.h"
#include <configuration.h>

#define TARGETID 0
#define NODEID 1
#define FLAGS 2

// for alternate packets, count is the fourth byte
#define ALT_COUNT 3

//in alternate Packets, the packet type is specified in the 5. byte
#define ALT_PACKET_TYPE 4

// structure defining the actual data that are transmitted/received.
// Classic Payload with 8 Bytes, possibly extended to 12 bytes
typedef struct
{
   //  8 Bytes minimum Payload
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint16_t supplyV :12;    // Supply voltage
   uint16_t count :8;
   uint16_t temp :12;   // Temperature reading
   uint8_t humidity;    // Humidity reading
   
   // optionally 4 more Bytes with pressure and brightness.
   uint32_t pressure : 22;
   uint32_t brightness : 10; // brightness or any other analog value.
}  Payload; // aka PacketType0


// Type to transmit strings, string length = 7 bytes, 
// Packet length:12 bytes
typedef struct
{
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint8_t count;
   uint8_t packet_type =1;  // must be 1
   char    data[7];
} PacketType1;


// Type to transmit strings, string length = 11 bytes,
// Packet length: 16 bytes
typedef struct
{
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint8_t count;
   uint8_t packet_type =2;  // must be 2
   char    data[11];
} PacketType2;


// structure defining alternate Packet type 3, a BME280
// temperature, humidity and pressure (pressure has 24 bits)
// Packet length: 12 Bytes
// this packet type is obsolete from TiNo Version 3.0 onwards
typedef struct
{
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint8_t count;
   uint8_t packet_type =3;  // must be 3
   uint16_t supplyV :12;    // Supply voltage
   uint16_t temp :12;   // Temperature reading
   uint8_t humidity;    // Humidity reading
   uint32_t pressure:24;
} PacketType3;


//structure defining alternate Packet type 4, up to 3 DS18B20 or PT100
// 14 bit resolution
// Packet length: 12 Bytes
typedef struct
{
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint8_t count;
   uint8_t packet_type =4;  // must be 4
   uint16_t supplyV :12;    // Supply voltage
   uint16_t temp :14;   // Temperature reading
   uint16_t temp1 :12;
   uint16_t temp2 :12;
   uint8_t count_msb :6;
} PacketType4;


//Packet type 5 with humidity sensor and an additional temperature reading (DS18B20 or MAX31865)
typedef struct
{
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint8_t count;
   uint8_t packet_type =5;  // must be 5
   uint16_t supplyV :12;    // Supply voltage
   uint16_t temp :12;   // Temperature reading of humidity sensor
   uint8_t humidity;    // Humidity reading
   uint32_t temp1:14;
   uint32_t brightness:10;
} PacketType5;

//Packet type 6 is an alarm-type packet.
// the alarm-type signals which measurement has triggered the alarm:
// 1 = temperature
// 2 = humidity
// 3 = pressure
// 4 = brightness
// 5 = temp1
// 6 = temp2
// 7 = supplyVoltage
enum alarm_t {temp=1,humidity,pressure,brightness,temp1,temp2,supplyV};

//value is the measurement value according to alarm_type
typedef struct
{
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint8_t count;
   uint8_t packet_type = 6;  // must be 6
   uint8_t alarm_type; // signals which sensor sends an alarm
   uint16_t value; 
} PacketType6;


typedef struct
{
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   int16_t fei;
   uint8_t count;
   uint8_t RSSI;
   uint8_t temp;
}  PayloadAck;


// structure that contains all important data about an received Packet.
typedef struct
{
    uint8_t     payload[64]; // max message length
    uint8_t     datalen;
    float       RSSI;
    int16_t     FEI;
    int8_t      TEMP;
    uint16_t    numerrors=0xffff;
    int8_t      errorcode =0;
    bool        success = false;
}  RadioRxPacket;




class myMAC {
    public:
        myMAC(GenericRadio &Radio, Configuration &config, uint8_t *EncryptionKey = NULL, Stream* ser = &Serial);
        void set_encryption_key(uint8_t *Key);
        void radio_begin(void);
        void radio_sleep(){radio.sleep();}
        void radio_ook_transmit_begin(){radio.OOKTransmitBegin();}
        int16_t radio_calc_temp_correction(int temp);
        bool radio_receive(bool blocking=false);
        bool radio_send(Payload &tinytx, uint8_t requestAck=0);
        bool radio_send(uint8_t *data, uint8_t datalen, uint8_t requestAck=0);
        bool radio_send(uint8_t *data, uint8_t datalen, uint8_t requestAck, int16_t temperature);
        uint8_t radio_read_temperature(uint8_t correction=0);
        
        RadioRxPacket rxpacket;
        const uint8_t *encryption_key=NULL;
    private:
        uint8_t extract_count(uint8_t *message);
        Stream *serial; // for debug only!
        GenericRadio &radio;
        Configuration &Cfg;
        fec_codec coder;

};

#endif