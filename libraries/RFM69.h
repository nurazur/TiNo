// **********************************************************************************
// Driver definition for HopeRF RFM69W/RFM69HW/RFM69CW/RFM69HCW, Semtech SX1231/1231H
// **********************************************************************************
// Copyright Felix Rusu (2014), felix@lowpowerlab.com
// http://lowpowerlab.com/
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************


#ifndef RFM69_AH_h
#define RFM69_AH_h
#include <Arduino.h>            //assumes Arduino IDE v1.0 or greater

#include "GenericRadio.h"

#define RADIO RFM69

#define RF69_MAX_DATA_LEN         64 // to take advantage of the built in AES/CRC we want to limit the frame size to the internal FIFO size (66 bytes - 3 bytes overhead)
#define RF69_SPI_CS               SS // SS is the SPI slave select pin, for instance D10 on atmega328

// INT0 on AVRs should be connected to RFM69's DIO0 (ex on Atmega328 it's D2, on Atmega644/1284 it's D2)
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega88) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
  #define RF69_IRQ_PIN          2
  #define RF69_IRQ_NUM          0
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
  #define RF69_IRQ_PIN          2
  #define RF69_IRQ_NUM          2
#elif defined(__AVR_ATmega32U4__)
  #define RF69_IRQ_PIN          3
  #define RF69_IRQ_NUM          0
#elif defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
  #define RF69_IRQ_PIN          2 // PB2/INT0
  #define RF69_IRQ_NUM          0
  #undef RF69_SPI_CS
  #define RF69_SPI_CS           1 // PB1
#endif


#define CSMA_LIMIT          -90 // upper RX signal sensitivity threshold in dBm for carrier sense access
#define RF69_MODE_SLEEP       0 // XTAL OFF
#define	RF69_MODE_STANDBY     1 // XTAL ON
#define RF69_MODE_SYNTH	      2 // PLL ON
#define RF69_MODE_RX          3 // RX MODE
#define RF69_MODE_TX		      4 // TX MODE

//available frequency bands
#define RF69_315MHZ     31  // non trivial values to avoid misconfiguration
#define RF69_433MHZ     43
#define RF69_868MHZ     86
#define RF69_915MHZ     91

#define BAND_315MHZ     31  // non trivial values to avoid misconfiguration
#define BAND_433MHZ     43
#define BAND_868MHZ     86
#define BAND_915MHZ     91

#define null                  0
#define COURSE_TEMP_COEF    -90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#define RF69_BROADCAST_ADDR 255
#define RF69_CSMA_LIMIT_MS 1000
#define RF69_TX_LIMIT_MS 1000
#define RF69_FSTEP 61.03515625 // == FXOSC/2^19 = 32mhz/2^19 (p13 in DS)

class RFM69 : public GenericRadio
{
  public:
    //static volatile byte DATA[RF69_MAX_DATA_LEN];          // recv/xmit buf, including hdr & crc bytes
    //static volatile byte DATALEN;
    static volatile uint8_t STATUSREG;
    //int RSSI; //most accurate RSSI during reception (closest to the reception)
    //static volatile int16_t FEI;
    static volatile byte _mode; //should be protected?
    static volatile byte TEMPREG;
    //int fdev;
    //long vcc_dac;
    bool do_frequency_correction;
    
    RFM69(byte slaveSelectPin=RF69_SPI_CS, byte interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, byte interruptNum=RF69_IRQ_NUM) : GenericRadio(RF69_FSTEP)
	{
      _slaveSelectPin = slaveSelectPin;
      _interruptPin = interruptPin;
      _interruptNum = interruptNum;
      _mode = RF69_MODE_STANDBY;
      _powerLevel = 31;
      _isRFM69HW = isRFM69HW;
      _PaBoost =false;
	  fdev =0;
      //FSTEP = RF69_FSTEP;
	  do_frequency_correction = false;
      DATA = new byte[RF69_MAX_DATA_LEN];
    }

    //bool initialize(byte freqBand, byte ID, byte networkID=1);
    bool initialize(byte freqBand, byte networkID=1, byte txpower=31);
    bool Initialize(byte freqBand, byte networkID=1, byte txpower=31); // for compatibility with RFM12 lib
    void OpModeOOK();
    void OOKTransmitBegin();
    void OOKTransmitEnd();
    void setAddress(byte addr);
    void setNetwork(byte networkID);
    bool canSend();
    //void send(byte toAddress, const void* buffer, byte bufferSize);
    void send(const void* buffer, byte bufferSize);
    //void Send(byte toAddress, const void* buffer, byte bufferSize); // for compatibility with older RFM12B lib
    void Send(const void* buffer, byte bufferSize); // for compatibility with older RFM12B lib
    //bool sendWithRetry(byte toAddress, const void* buffer, byte bufferSize, byte retries=2, byte retryWaitTime=40); //40ms roundtrip req for  61byte packets
    bool receiveDone();
    //bool ACKReceived(byte fromNodeID);
    //bool ACKRequested();
    //void sendACK(const void* buffer = "", uint8_t bufferSize=0);
    uint32_t getFrequency();
    void setFrequency(uint32_t steps);
	void setFrequencyMHz(float freqMHz);
    int frequency_correct(void);
    //void encrypt(const char* key);
    void setCS(byte newSPISlaveSelect);
    int readRSSI(bool forceTrigger=false);
    void promiscuous(bool onOff=true);
    void setHighPower(bool isRFM69HW=true, bool PaBoost = false, bool usePa1andPa2=false); //have to call it after initialize for RFM69HW
    void setPowerLevel(byte level); //reduce/increase transmit power level
    void sleep();
    void Sleep(){sleep();}
    void setDataShaping(uint8_t datashaping);
    byte readTemperature(byte calFactor=0); //get CMOS temperature (8bit)
    void rcCalibration(); //calibrate the internal RC oscillator for use in wide temperature variations - see datasheet section [4.3.5. RC Timer Accuracy]

    // allow hacking registers by making these public
    byte readReg(byte addr);
    void writeReg(byte addr, byte val);
    void readAllRegs();
    int16_t readAFC(void);
    int16_t readFEI(void);
	//const double FSTEP;

  protected:
    static void isr0();
    void virtual interruptHandler();
    static RFM69* selfPointer;
    
    void sendFrame(const void* buffer, byte size);

    
    byte _slaveSelectPin;
    byte _interruptPin;
    byte _interruptNum;
    byte _powerLevel;
    bool _isRFM69HW;
    bool _PaBoost;
	
	#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
	#else
    byte _SPCR;
    byte _SPSR;
	#endif

    

    void receiveBegin();
    void setMode(byte mode);
    void setHighPowerRegs(bool onOff);
    void select();
    void unselect();

};

#endif
