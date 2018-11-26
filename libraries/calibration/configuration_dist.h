//
// Version 4
//
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
#ifndef configuration
#define configuration
#include <Arduino.h>

#define EEPROMVERSIONNUMBER 4

typedef struct
{
	byte Nodeid = 			1;
	byte Networkid =		210;
	byte Gatewayid = 		22;
	uint16_t VccAtCalmV  = 	3000;
	uint16_t AdcCalValue =  375;
	uint16_t Senddelay = 	7;
	byte Frequencyband = 43 ;
	float frequency = 434.0;
	byte TxPower = 0;
	byte RequestAck =0;
	byte LedCount = 0;
	byte LedPin = 8;
	byte RxPin = 0;
	byte TxPin = 1;
	byte SDAPin = A4;
	byte SCLPin = A5;
	byte I2CPowerPin = 9;
    char PCI0Pin=-1;
    byte PCI0Trigger=2; // Low/Change/Falling/Rising = 0/1/2/3
    char PCI1Pin=-1;
    byte PCI1Trigger=2; 
    char PCI2Pin=-1;
    byte PCI2Trigger=2; 
    char PCI3Pin=-1;
    byte PCI3Trigger=2;
    byte UseCrystalRtc=0;
    byte EncryptionEnable=1;
    byte FecEnable =1;
    byte InterleaverEnable =1;
    byte EepromVersionNumber=4;
    uint16_t SoftwareversionNumber=0;
    byte TXGaussShaping =0;
    byte SerialEnable =1;
    byte IsRFM69HW = 0;
    byte PaBoost = 0;
    int16_t FedvSteps= 0;   
	byte checksum=0xff;
    uint16_t size=0;    // from here, data are not part of checksum and are possibly not encrypted.
	uint16_t dummy1=0;
}
Configuration;

#endif