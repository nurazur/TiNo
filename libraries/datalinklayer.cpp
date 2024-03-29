// Media Access Layer for Nodes and Gateways using RFM69CW or HCW or CC1101

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

#include <Arduino.h>
#include "datalinklayer.h"
#include <EEPROM.h>

myMAC::myMAC(GenericRadio &R, Configuration &C, uint8_t *K, Stream* S) : encryption_key(K), serial(S), radio(R), Cfg(C)
{
}


// configure Radio
void myMAC::radio_begin(void)
{
    radio.fdev = Cfg.FedvSteps;
    //radio.Initialize(Cfg.Frequencyband, Cfg.Networkid, Cfg.TxPower);
    radio.Initialize(86, Cfg.Networkid, Cfg.TxPower); // 86 - preset the 868 MHz Band. Can be overridden by setFrequencyMHz()
    // PaBoost is for RF69HCW only.
    // PaBoost = 0 normal mode (PA1 only)
    // PaBoost = 1 for PA1 and PA2 use and activate high-Power registers.
    // PaBoost = 2 use PA1 and PA2 but do not use high-power registers
    // More details in the radio driver
    radio.setHighPower   (Cfg.IsRFM69HW, Cfg.PaBoost&0x1, Cfg.PaBoost&0x2);
    radio.setFrequencyMHz(Cfg.frequency); // no need to care about offset, the RF driver does this.
    //radio.setFrequency((Cfg.frequency * 1000000)/radio.FSTEP + Cfg.FedvSteps);
    radio.setDataShaping (Cfg.TXGaussShaping);
    radio.setPowerLevel  (Cfg.TxPower);
    serial->print("freq = 0x"); serial->print(radio.getFrequency(),HEX); serial->println();
    radio.sleep();       // sleep right away to save power
}

// look up the correction table and calculate correction steps
int16_t myMAC::radio_calc_temp_correction(int temp)
{
    // clamp Temperature
    if (temp > 105) temp = 105;
    if (temp < -30) temp = -30;

    // search for entry in look-up table
    uint16_t index  =  (temp + 30) * 2 + sizeof(Cfg);
    int16_t coeff;
    EEPROM.get(index, coeff);  // this is ppm *1000
    float steps = coeff * Cfg.frequency / 1000 / radio.FSTEP;

    int16_t corr = int(steps < 0 ? steps -0.5 : steps+ 0.5);
    /*
    serial->print("sizeof cfg: ");serial->print(sizeof(Cfg));
    serial->print(", index: ");serial->print(index);
    serial->print(", coeff: ");serial->print(coeff);
    serial->print(", steps: ");serial->print(steps);
    serial->print(", correction: ");serial->print(corr);
    serial->println();
    */
    return corr; // frequency-steps
}

bool myMAC::radio_receive(bool blocking)
{
    bool returncode = false;
    bool done = false;
    bool gateway_mode = Cfg.RequestAck&0x4;

    // invalidate rxpacket;
    rxpacket.success= false;
    rxpacket.errorcode= 0; // error: no packet received
    rxpacket.numerrors = 0xffff;


    if(blocking)
        do
        {
            done  = radio.receiveDone();
        }
        while (!done);
    else
        done  =  radio.receiveDone();

    if (done)
    {
        returncode =true;
        rxpacket.datalen = radio.DATALEN;
        if ((radio.DATALEN % 8 == 0) || (radio.DATALEN % 4 == 0 && !Cfg.FecEnable))
        {
            //byte datalen = radio.DATALEN;
            if (Cfg.InterleaverEnable)
            {
                interleave ((unsigned char*)radio.DATA, radio.DATALEN, REVERSE);
            }

            if (this->Cfg.FecEnable)
            {
                rxpacket.datalen /=2;
                if (this->coder.decode_block((unsigned char*)radio.DATA, (unsigned char*) &rxpacket.payload, rxpacket.datalen, rxpacket.numerrors))
                {
                }
                else
                {
                    rxpacket.payload[NODEID] = 0; // Error: could not decode FEC data
                    rxpacket.errorcode =-1;
                }
            }
            else
            {
                // copy data from radio.DATA into rxpacket.payload;
                memcpy(rxpacket.payload, (const void*)radio.DATA, radio.DATALEN);
            }

            if (Cfg.EncryptionEnable)
            {
                xxtea Xxtea((uint8_t*) encryption_key, (uint8_t*) &rxpacket.payload);
                Xxtea.crypter(decrypt, rxpacket.datalen); //Decrypt it
            }

            if (rxpacket.payload[TARGETID] == Cfg.Nodeid) // is the message for me?
            {
                rxpacket.success= true;
            }
            else if (gateway_mode)  // Gateway mode: listen to all messages on this channel, do not answer to ack requests (target node must do this)
            {
                rxpacket.success= true;
            }
            else // Error : not my message or address corrupted
            {
                rxpacket.errorcode =-3;
            }
        }
        else
        {
            rxpacket.errorcode = -2; // Error: data length does not match
        }

        rxpacket.RSSI = (float)radio.RSSI/2.0; // radio.RSSI is an int and it is a negative number.
        rxpacket.FEI= radio.FEI;
        rxpacket.TEMP = (int8_t)radio.readTemperature(0);

        // Sending an ACK
        if(rxpacket.success && (rxpacket.payload[FLAGS] & 0x80) && (rxpacket.payload[NODEID] != 0) && (rxpacket.payload[TARGETID] == Cfg.Nodeid) && !(Cfg.RequestAck&0x2)) // Ack requested
        {
            // WORKS
            PayloadAck ack_packet;
            ack_packet.nodeid = Cfg.Nodeid;    //22
            ack_packet.targetid = rxpacket.payload[NODEID];
            ack_packet.flags = (rxpacket.payload[FLAGS] &0x1f) | 0x40;
            ack_packet.fei = rxpacket.FEI;
            ack_packet.count = this->extract_count(rxpacket.payload);
            ack_packet.RSSI = -radio.RSSI;
            ack_packet.temp = rxpacket.TEMP;
            Payload hilfspaket;
            memcpy(&hilfspaket, &ack_packet, sizeof(Payload));
            this->radio_send( hilfspaket, 0); // do under no circumstances request an Ack from sending ACK! Disguise ackpacket as a normal payload!
        }
    }
    return returncode;
}

bool myMAC::radio_send(uint8_t *data, uint8_t datalen, uint8_t requestAck, int16_t temperature)
{
    radio.setFrequency((Cfg.frequency * 1000000)/radio.FSTEP + Cfg.FedvSteps - radio_calc_temp_correction(temperature) );
    return this->radio_send(data,datalen,requestAck);
}



bool myMAC::radio_send(Payload &tinytx, uint8_t requestAck)
{

    return this->radio_send((uint8_t*) &tinytx, sizeof(Payload), requestAck);
}

bool myMAC::radio_send(uint8_t *data, uint8_t datalen, uint8_t requestAck)
{
    unsigned char tinytx_encrypt[datalen]; // copy of the message, possibly encrypted.
    bool ackreceived = false;

    if (requestAck&0x1)
        data[FLAGS] |= 0x80;
    else
        data[FLAGS] &= 0x7f;

    memcpy(tinytx_encrypt, data, datalen);
    // encrypt?
    if (Cfg.EncryptionEnable)
    {
        xxtea Xxtea((uint8_t*) encryption_key, (uint8_t*) tinytx_encrypt);
        Xxtea.crypter(encrypt, datalen); //Encrypt it
    }

    //FEC
    if(Cfg.FecEnable)
    {
        unsigned char tinytx_fec[datalen*2];   // the encoded message
        this->coder.encode_block((unsigned char*) tinytx_encrypt, tinytx_fec, datalen); // add Forward Error Correction

        //Interleave?
        if (Cfg.InterleaverEnable)
            interleave(tinytx_fec, sizeof (tinytx_fec)); // spread bits over the entire message to get over burst errors

        radio.send(tinytx_fec, sizeof (tinytx_fec)); // Cfg.Gatewayid not used by radio.Send!
    }
    // No FEC
    else
    {
        //Interleave?
        if (Cfg.InterleaverEnable)
        {
            interleave(tinytx_encrypt, datalen);
        }
        radio.send(tinytx_encrypt, datalen);
    }

    if (requestAck&0x1)
    {
        // wait for an answer
        unsigned long sentTime;
        sentTime = millis();
        while ((millis()-sentTime)<200) // answer comes usually after 16ms
        {
            this->radio_receive(false); // non-blocking;
            if (rxpacket.success)
            {
                if(Cfg.SerialEnable)  // debug only
                {
                    //serial->print(millis()-sentTime);serial->println("ms, ");
                    //serial->print("FEI= ");  serial->print(rxpacket.FEI);
                    //serial->print(" RSSI= "); serial->print(rxpacket.RSSI);
                    //serial->println();
                    //serial->flush();
                }
                break;
            }
        }
        interrupts();  //radio_receive() disables interrupts after successful reception of data. Need to re-enable here explicitely
        radio.sleep(); // CC1101: this flushes the RX Buffer.
        // got an answer
        if (rxpacket.success)
        {
            // check if its the right answer
            PayloadAck *ack_packet = (PayloadAck*) &(rxpacket.payload); // interpret the received data as PayloadAck packet

            // handling count is complicated.
            // need to extract count from sent message.
            if (ack_packet->flags &0x40  &&  ack_packet->count == extract_count(data))
            {
                ackreceived = true;
            }
            serial->print("FEI: ");serial->print(ack_packet->fei);serial->println();
            serial->print("RSSI: ");serial->print(ack_packet->RSSI);serial->println();
            serial->flush();
        }
        else
        {
            serial->println("rxpacket not successful or timeout");
        }

    }
    radio.sleep();
    return ackreceived;
}

uint8_t myMAC::radio_read_temperature(uint8_t correction)
{
    return radio.readTemperature(correction);
}

uint8_t myMAC::extract_count(uint8_t *message)
{
    uint8_t count =0;
    if (message[FLAGS] & 0x20) // alternate packet types have a different layout structure
    {
        count  = message[ALT_COUNT];
    }
    else
    {
        Payload *oldformat;  // pointer to Payload
        oldformat = (Payload*) message; // cast pointer
        count = oldformat->count; // extract 8-Bit count value
    }

    return count;
}
