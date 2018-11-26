// Media Access Layer for Nodes and Gateways using RFM69CW or HCW

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
// Version Build 7
#ifndef mymac
#define mymac

#include <Arduino.h> 
#include <RFM69.h>
#include <RFM69registers.h>
#include "codec.h"
#include "configuration_dist.h"

#define TARGETID 0
#define NODEID 1
#define FLAGS 2
// structure defining the actual data that are transmitted/received.
// Classic Payload with 8 Bytes
typedef struct 
{ 
   uint8_t targetid;
   uint8_t nodeid;
   uint8_t flags;
   uint16_t supplyV :12;    // Supply voltage
   uint16_t count :8;
   uint16_t temp :12;   // Temperature reading 
   uint8_t humidity;    // Humidity reading 
}  Payload; 



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
    float       RSSI;
    int16_t     FEI;
    int8_t      TEMP;
    uint16_t    numerrors=0xffff;
    int8_t      errorcode =0;
    bool        success = false; 
}  RadioRxPacket;




class myMAC{
    public:
        myMAC(RFM69 &Radio, Configuration &config, uint8_t *EncryptionKey = NULL, Stream* ser = &Serial);
        void set_encryption_key(uint8_t *Key);
        void radio_begin(void);
        bool radio_receive(bool blocking=false);
        bool radio_send(Payload &tinytx, uint8_t requestAck=0);
        bool radio_send(uint8_t *data, uint8_t datalen, uint8_t requestAck=0);
        
        RadioRxPacket rxpacket;
        const uint8_t *encryption_key=NULL;
    private:
        Stream *serial; // for debug only!
        RFM69 &radio;
        Configuration &Cfg;
        fec_codec coder;
    
};

#endif