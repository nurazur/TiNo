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

// "mini" version heavily modified by nurazur - nurazur@gmail.com
// this version is a bare RF driver withoput gimmicks, so ack/nack has been removed because it is not part of the RF functionality
// all coding a protocol implementation has to be done by a higher layer (Data link layer)


#include "RFM69.h"
#include "RFM69registers.h"
//#include "SPI_common.h"
#include "SPI.h"
//#define LIBCALL_ENABLEINTERRUPT
//#include <EnableInterrupt.h>
using namespace std;

extern long readVcc();

volatile byte RFM69::_mode;       // current transceiver state
volatile byte GenericRadio::DATALEN;
volatile uint8_t RFM69::STATUSREG;
volatile int GenericRadio::RSSI; //most accurate RSSI during reception (closest to the reception)
volatile int16_t GenericRadio::FEI;
volatile byte RFM69::TEMPREG;



RFM69* RFM69::selfPointer;


int16_t RFM69::readAFC(void)
{
    int16_t afc=0;
    afc = readReg(REG_AFCMSB) << 8;
    afc |= readReg(REG_AFCLSB);
    return afc;
}


int16_t RFM69::readFEI(void)
{
    int16_t fei;
    writeReg(REG_AFCFEI, readReg(REG_AFCFEI) | RF_AFCFEI_FEI_START); // start a FEI Measurement
    while ((readReg(REG_AFCFEI) & RF_AFCFEI_FEI_DONE) == 0x00); // Wait for FEI Done
    fei = readReg(REG_FEIMSB) << 8;
    fei |= readReg(REG_FEILSB);
    //writeReg(REG_AFCFEI, RF_AFCFEI_AFCAUTO_OFF | RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_AFC_CLEAR);
    return fei;
}


//Remark: setting frequency deviation fdev must be set BEFORE initializing the RFM
//Note:    nodeID is not used anymore, this must be done in a higher layer
bool RFM69::initialize(byte freqBand, byte networkID, byte txpower)
{
  (void) freqBand; // not used anymore
  const byte CONFIG[][2] =
  {
    /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
    /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
    /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_19200 }, //default:4.8 KBPS
    //* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_19200 },
    /* 0x04 */ { REG_BITRATELSB, 0x86 }, //to fit better the setting in RFM12B
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_30000}, // (FDEV + BitRate/2 <= 500Khz)
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_30000},

    // set frequency - just for initialization
    /* 0x07 */ { REG_FRFMSB, RF_FRFMSB_865},
    /* 0x08 */ { REG_FRFMID, RF_FRFMID_865},
    /* 0x09 */ { REG_FRFLSB, RF_FRFLSB_865},

    //* 0x07 */ { REG_FRFMSB, (byte)(freqBand==BAND_315MHZ ? RF_FRFMSB_315 : (freqBand==BAND_433MHZ ? 0x6c : (freqBand==BAND_868MHZ ? RF_FRFMSB_865 : RF_FRFMSB_915))) },
    //* 0x08 */ { REG_FRFMID, (byte)(freqBand==BAND_315MHZ ? RF_FRFMID_315 : (freqBand==BAND_433MHZ ? 0x80 : (freqBand==BAND_868MHZ ? RF_FRFMID_865 : RF_FRFMID_915))) },
    //* 0x09 */ { REG_FRFLSB, (byte)(freqBand==BAND_315MHZ ? RF_FRFLSB_315 : (freqBand==BAND_433MHZ ? 0x00 : (freqBand==BAND_868MHZ ? RF_FRFLSB_865 : RF_FRFLSB_915))) },


    // looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
    // +17dBm and +20dBm are possible on RFM69HW
    // +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
    // +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
    // +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
    ///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
    ///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA)

    // RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4khz)
    //* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 }, //(BitRate < 2 * RxBw) //41.7 kHz
    /* 0x19 for BR-19200:  */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_3 }, //62.5 kHz
    //* 0x19 Test */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_4 },
    /* 0x1A */
    /* 0x1E */ { REG_AFCFEI, RF_AFCFEI_AFCAUTO_OFF | RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_AFC_CLEAR},
    /* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
    /* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, // DIO5 ClkOut disable for power saving
    /* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // writing to this bit ensures that the FIFO & status flags are reset
    /* 0x29 */ { REG_RSSITHRESH, 216 }, //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
    ///* 0x2d */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE } // default 3 preamble bytes 0xAAAAAA
    /* 0x2e */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0 },
    /* 0x2f */ { REG_SYNCVALUE1, 0x2D },      //make this compatible with sync1 byte of RFM12B lib
    /* 0x30 */ { REG_SYNCVALUE2, networkID }, //NETWORK ID
    ///* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
        //* 0x39 */ { REG_NODEADRS, nodeID }, //turned off because we're not using address filtering
    /* 0x38 */ { REG_PAYLOADLENGTH, 66 }, //in variable length mode: the max frame size, not used in TX
    /* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
    /* 0x3d */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //*for BR-19200: //* 0x3d */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //* 0x6F */ { REG_TESTDAGC, RF_DAGC_CONTINUOUS }, // run DAGC continuously in RX mode
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
    {255, 0}
  };

  pinMode(_slaveSelectPin, OUTPUT);
  digitalWrite(_slaveSelectPin, HIGH);
  SPI.begin();

  do writeReg(REG_SYNCVALUE1, 0xaa); while (readReg(REG_SYNCVALUE1) != 0xaa); // verstehe ich nicht!
  do writeReg(REG_SYNCVALUE1, 0x55); while (readReg(REG_SYNCVALUE1) != 0x55);

  for (byte i = 0; CONFIG[i][0] != 255; i++)
    writeReg(CONFIG[i][0], CONFIG[i][1]);

  // Encryption is persistent between resets and can trip you up during debugging.
  // Disable it during initialization so we always start from a known state.
  // encrypt(0);

  setHighPower(_isRFM69HW); //called regardless if it's a RFM69W or RFM69HW
  setPowerLevel(txpower);

  setMode(RF69_MODE_STANDBY);
  while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
  attachInterrupt(_interruptNum, RFM69::isr0, RISING);
  //enableInterrupt(_interruptPin, RFM69::isr0, RISING);

  setFrequency(getFrequency()  + fdev);

  selfPointer = this;

  this->vcc_dac = readVcc();

  interrupts();
  STATUSREG = SREG; // save state of interrupts
  return true;
}

bool RFM69::Initialize(byte freqBand, byte networkID, byte txpower)
{
    return this->initialize(freqBand, networkID, txpower);
}


void RFM69::OpModeOOK()
{
    writeReg( REG_OPMODE, RF_OPMODE_SEQUENCER_OFF | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY);
    writeReg( REG_DATAMODUL, RF_DATAMODUL_DATAMODE_CONTINUOUSNOBSYNC | RF_DATAMODUL_MODULATIONTYPE_OOK | RF_DATAMODUL_MODULATIONSHAPING_00);
}

void RFM69::OOKTransmitBegin()
{
    this->OpModeOOK();
    setMode(RF69_MODE_TX);
}

void RFM69::OOKTransmitEnd()
{
    //setMode(RF69_MODE_STANDBY);
    sleep();
}


//return the frequency (in register word)
uint32_t RFM69::getFrequency()
{
  return ((uint32_t)readReg(REG_FRFMSB)<<16) + ((uint16_t)readReg(REG_FRFMID)<<8) + readReg(REG_FRFLSB);
}

/*set the frequency (in Hz)
void RFM69::setFrequency(uint32_t freqHz)
{
  //TODO: p38 hopping sequence may need to be followed in some cases
  freqHz /= RF69_FSTEP; //divide down by FSTEP to get FRF
  writeReg(REG_FRFMSB, freqHz >> 16);
  writeReg(REG_FRFMID, freqHz >> 8);
  writeReg(REG_FRFLSB, freqHz);
}
*/
void RFM69::setFrequencyMHz(float freqMHz)
{
    uint32_t freqSteps = (uint32_t) floor((freqMHz *1000000.0 / FSTEP)+0.5) + this->fdev;
    setFrequency(freqSteps);
}

//set the frequency (in Hz/fstep)
void RFM69::setFrequency(uint32_t freq)
{
  writeReg(REG_FRFMSB, freq >> 16);
  writeReg(REG_FRFMID, freq >> 8);
  writeReg(REG_FRFLSB, freq);
}




/* 0 = no shaping
*  1 BT=1
*  2 BT =0.5
*  3 BT= 0.3
*/
void RFM69::setDataShaping(uint8_t datashaping)
{
    byte reg = readReg(REG_DATAMODUL) &0xfc;
    writeReg(REG_DATAMODUL, reg | (datashaping&0x3));
}

void RFM69::setMode(byte newMode)
{
    if (newMode == _mode) return; //TODO: can remove this?

    switch (newMode) {
        case RF69_MODE_TX:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
            break;
        case RF69_MODE_RX:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
            break;
        case RF69_MODE_SYNTH:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
            break;
        case RF69_MODE_STANDBY:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
            break;
        case RF69_MODE_SLEEP:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
            break;
        default: return;
    }

    // we are using packet mode, so this check is not really needed
  // but waiting for mode ready is necessary when going from sleep because the FIFO may not be immediately available from previous mode
    while (_mode == RF69_MODE_SLEEP && (readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady

    _mode = newMode;
}

void RFM69::sleep() {
  setMode(RF69_MODE_SLEEP);
}

void RFM69::setAddress(byte addr)
{
    writeReg(REG_NODEADRS, addr);
}

void RFM69::setNetwork(byte networkID)
{
    writeReg(REG_SYNCVALUE2, networkID);
}

// set output power: 0=min (-18 dBm), 31=max (13 dBm)
void RFM69::setPowerLevel(byte powerLevel)
{
  _powerLevel = powerLevel;
  writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | (_powerLevel > 31 ? 31 : _powerLevel));
}




bool RFM69::canSend()
{
  if (_mode == RF69_MODE_RX && DATALEN == 0 && readRSSI() < CSMA_LIMIT) //if signal stronger than -100dBm is detected assume channel activity
  {
    setMode(RF69_MODE_STANDBY);
    return true;
  }
  return false;
}

void RFM69::send(const void* buffer, byte bufferSize)
{
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  unsigned long now = millis();
  while (!canSend() && millis()-now < RF69_CSMA_LIMIT_MS) receiveDone();

  sendFrame(buffer, bufferSize);
}

void RFM69::Send(const void* buffer, byte bufferSize)
{
    send(buffer, bufferSize);
}


void RFM69::sendFrame(const void* buffer, byte bufferSize)
{
    setMode(RF69_MODE_STANDBY); //turn off receiver to prevent reception while filling fifo
    while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
    writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
    if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;

    if (_PaBoost) setHighPowerRegs(true);

    writeReg(REG_AUTOMODES, RF_AUTOMODES_ENTER_FIFONOTEMPTY | RF_AUTOMODES_EXIT_PACKETSENT | RF_AUTOMODES_INTERMEDIATE_TRANSMITTER);
    //write to FIFO
    select();
    SPI.transfer(REG_FIFO | 0x80);
    SPI.transfer(bufferSize);
    for (byte i = 0; i < bufferSize; i++)
        SPI.transfer(((byte*)buffer)[i]);
    unselect();

    // no need to wait for transmit mode to be ready since its handled by the radio //
    // setMode(RF69_MODE_TX);

    unsigned long txStart = millis();
    //this->vcc_dac = readVcc(); // original, works stable
    //while (digitalRead(_interruptPin) == 0 && millis()-txStart < RF69_TX_LIMIT_MS); //wait for DIO0 to turn HIGH signalling transmission finish // ORIGINAL CODE
    //while (!digitalRead(_interruptPin));

    //while (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT == 0x00); // Wait for ModeReady does not work! PacketSent Flag does not work at all!
    while ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFONOTEMPTY)  && millis()-txStart < RF69_TX_LIMIT_MS  ); //works
    #if (F_CPU >1000000L)
    delayMicroseconds(400); // need a delay because the FIFONOTEMPTY goes low before the packet is sent.
    #endif
    writeReg(REG_AUTOMODES, RF_AUTOMODES_ENTER_OFF | RF_AUTOMODES_EXIT_OFF | RF_AUTOMODES_INTERMEDIATE_SLEEP);
    if (_PaBoost) setHighPowerRegs(false);
    this->vcc_dac = readVcc();
}


/*
void RFM69::sendFrame(const void* buffer, byte bufferSize)
{
    setMode(RF69_MODE_STANDBY); //turn off receiver to prevent reception while filling fifo
    while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
    writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
    if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;

    if (_PaBoost) setHighPowerRegs(true);

    //write to FIFO
    select();
    SPI.transfer(REG_FIFO | 0x80);
    SPI.transfer(bufferSize);
    for (byte i = 0; i < bufferSize; i++)
        SPI.transfer(((byte*)buffer)[i]);
    unselect();

    // no need to wait for transmit mode to be ready since its handled by the radio
    setMode(RF69_MODE_TX);

    unsigned long txStart = millis();
    while (digitalRead(_interruptPin) == 0 && millis()-txStart < RF69_TX_LIMIT_MS); //wait for DIO0 to turn HIGH signalling transmission finish // ORIGINAL CODE
    setMode(RF69_MODE_STANDBY);
    if (_PaBoost) setHighPowerRegs(false);
    this->vcc_dac = readVcc();
}
*/

void RFM69::interruptHandler()
{
  if (_mode == RF69_MODE_RX && (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY))
  {
    RSSI = readRSSI(false);
    FEI = readFEI();
    setMode(RF69_MODE_STANDBY);

    select();
    SPI.transfer(REG_FIFO & 0x7f);

    DATALEN = SPI.transfer(0); // CRC bytes are not counted by DATALEN

    DATALEN = DATALEN > RF69_MAX_DATA_LEN ? RF69_MAX_DATA_LEN : DATALEN; //precaution


    if (DATALEN < 6) // need at least senderid, targetid and 1 payload byte min.
    {
        DATALEN = 0;
        unselect();
        receiveBegin();
        return;
    }

    // now load the data
    for (byte i= 0; i < DATALEN; i++)
    {
      DATA[i] = SPI.transfer(0);
    }
    if (DATALEN<RF69_MAX_DATA_LEN) DATA[DATALEN]=0; //add null at end of string

    unselect();
    //writeReg(REG_AFCFEI, RF_AFCFEI_AFCAUTO_OFF | RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_AFC_CLEAR);
    setMode(RF69_MODE_RX);
  }

  //RSSI = readRSSI(false);
  interrupts(); // brauchts des?
}

void RFM69::isr0() { selfPointer->interruptHandler(); }

void RFM69::receiveBegin() {
  DATALEN = 0;
  //SENDERID = 0;
  //TARGETID = 0;

  //ACK_REQUESTED = 0;
  //ACK_RECEIVED = 0;
  RSSI = 0;
  FEI=0;

  if (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
    writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01); //set DIO0 to "PAYLOADREADY" in receive mode
  setMode(RF69_MODE_RX);
  interrupts();
}

bool RFM69::receiveDone()
{
  noInterrupts(); //re-enabled in unselect() or via receiveBegin()
  if (_mode == RF69_MODE_RX && DATALEN>0)
  {
    setMode(RF69_MODE_STANDBY);
    return true;
  }
  else if (_mode == RF69_MODE_RX)  //already in RX no payload yet
  {
    interrupts(); //explicitly re-enable interrupts
    return false;
  }
  receiveBegin();
  return false;
}

// To enable encryption: radio.encrypt("ABCDEFGHIJKLMNOP");
// To disable encryption: radio.encrypt(null) or radio.encrypt(0)
// KEY HAS TO BE 16 bytes !!!

/* not used in TiNo
void RFM69::encrypt(const char* key) {
  setMode(RF69_MODE_STANDBY);
  if (key!=0)
  {
    select();
    SPI.transfer(REG_AESKEY1 | 0x80);
    for (byte i = 0; i<16; i++)
      SPI.transfer(key[i]);
    unselect();
  }
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFE) | (key ? 1 : 0));
}
*/


int RFM69::readRSSI(bool forceTrigger) {
  int rssi = 0;
  if (forceTrigger)
  {
    //RSSI trigger not needed if DAGC is in continuous mode
    writeReg(REG_RSSICONFIG, RF_RSSI_START);
    while ((readReg(REG_RSSICONFIG) & RF_RSSI_DONE) == 0x00); // Wait for RSSI_Ready
  }
  rssi = -readReg(REG_RSSIVALUE);
  //rssi >>= 1;
  return rssi;
}

byte RFM69::readReg(byte addr)
{
  select();
  SPI.transfer(addr & 0x7F);
  byte regval = SPI.transfer(0);
  unselect();
  return regval;
}

void RFM69::writeReg(byte addr, byte value)
{
  select();
  SPI.transfer(addr | 0x80);
  SPI.transfer(value);
  unselect();
}

/// Select the transceiver
void RFM69::select()
{
    STATUSREG= SREG;
    noInterrupts();
    #if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__) // tiny doesn't set SPI settings like this
    #else // set SPI settings if not on tiny
        // save current SPI settings
        _SPCR = SPCR;
        _SPSR = SPSR;
        SPCR = (SPCR & ~0x0C) | 0x00;  // SPI.setDataMode(SPI_MODE0); (last hex number is the mode)
        SPCR &= ~(_BV(DORD)); // this is for MSBFIRST; LSBFIRST would be SPCR |= _BV(DORD);
        // clock divider: look this up in the datasheet
        // DIV4: use 0x00   here: V
        // DIV2: use 0x04   here: v
        SPCR = (SPCR & ~0x03) | (0x00 & 0x03);
        SPSR = (SPSR & ~0x01) | (0x00 & 0x01);
    #endif
  digitalWrite(_slaveSelectPin, LOW);
}

/// UNselect the transceiver chip
void RFM69::unselect()
{
    digitalWrite(_slaveSelectPin, HIGH);
    #if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
    #else
        //restore SPI settings to what they were before talking to RFM69
        SPCR = _SPCR;
        SPSR = _SPSR;
    #endif
    SREG=STATUSREG;
    //interrupts();
}

// ON  = disable filtering to capture all frames on network
// OFF = enable node+broadcast filtering to capture only frames sent to this/broadcast address
/*
void RFM69::promiscuous(bool onOff) {
  _promiscuousMode=onOff;
  //writeReg(REG_PACKETCONFIG1, (readReg(REG_PACKETCONFIG1) & 0xF9) | (onOff ? RF_PACKET1_ADRSFILTERING_OFF : RF_PACKET1_ADRSFILTERING_NODEBROADCAST));
}
*/


/*
setHighPower is used for the RFM69HW RFM69HCW only.
To use normal Mode, call setHighPower(true). This uses PA1 only and should be used up to 13 dBm (PL 31)
To use PA1 and PA2 in normal mode, call setHighPower(true, false, true). Use this in PL 30 and 31 (14 and 15 dBm)
To use highest possible power, call setHighPower(true, true); Use this for PL 31, 30 and 29 (16, 17 and 18 dBm)
*/
void RFM69::setHighPower(bool isRFM69HW, bool PaBoost, bool usePa1andPa2)
{
    _isRFM69HW = isRFM69HW;
    _PaBoost = PaBoost;

    if (_isRFM69HW)
    {
        if (_PaBoost) // RFM69HW --> Boost only makes sense with both PA's ON. Turn on PA 1 and PA 2 and set Pa Boost registers
        {
            writeReg(REG_OCP, RF_OCP_OFF);
            writeReg(REG_PALEVEL, RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | _powerLevel); //enable P1 & P2 amplifier stages
            setHighPowerRegs(true);
        }
        else if (usePa1andPa2) // turn on Pa 1 and Pa 2 but do not set boost registers
        {
            writeReg(REG_OCP, RF_OCP_ON);
            writeReg(REG_PALEVEL, RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | _powerLevel); //enable P1 & P2 amplifier stages
            setHighPowerRegs(false);
        }
        else // normal mode for up to 13+ dBm
        {
            writeReg(REG_OCP, RF_OCP_ON);
            writeReg(REG_PALEVEL, RF_PALEVEL_PA1_ON | _powerLevel); //enable P1 only
            setHighPowerRegs(false);
        }
    }
    else // RFM69CW
    {
        writeReg(REG_OCP, RF_OCP_ON);
        writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | _powerLevel); //enable P0 only
        setHighPowerRegs(false);
    }
}


void RFM69::setHighPowerRegs(bool PaBoost) {
    if (PaBoost)
    {
        writeReg(REG_TESTPA1, 0x5D );  //Boost
        writeReg(REG_TESTPA2, 0x7C );
    }
    else
    {
        writeReg(REG_TESTPA1, 0x55);
        writeReg(REG_TESTPA2, 0x70);
    }
}

void RFM69::setCS(byte newSPISlaveSelect) {
  _slaveSelectPin = newSPISlaveSelect;
  digitalWrite(_slaveSelectPin, HIGH);
  pinMode(_slaveSelectPin, OUTPUT);
}

/*for debugging
void RFM69::readAllRegs()
{
  byte regVal;

  for (byte regAddr = 1; regAddr <= 0x4F; regAddr++)
    {
    select();
    SPI.transfer(regAddr & 0x7f);   // send address + r/w bit
    regVal = SPI.transfer(0);
    unselect();

    Serial.print(regAddr, HEX);
    Serial.print(" - ");
    Serial.print(regVal,HEX);
    Serial.print(" - ");
    Serial.println(regVal,BIN);
    }
  unselect();
}
*/

byte RFM69::readTemperature(byte calFactor)  //returns centigrade
{
  setMode(RF69_MODE_STANDBY);
  writeReg(REG_TEMP1, RF_TEMP1_MEAS_START);
  while ((readReg(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING)) ;
  //return ~readReg(REG_TEMP2) + COURSE_TEMP_COEF + calFactor; //'complement'corrects the slope, rising temp = rising val
                                                               // COURSE_TEMP_COEF puts reading in the ballpark, user can add additional correction
  return 166 - readReg(REG_TEMP2) - calFactor;
  }



/*
byte RFM69::readTemperature(byte calFactor)  //returns centigrade
{
  char done = 0;
  unsigned long temp_start;
  this->Temperature =0;
  byte counter=0;

  for (int i=0; i<2; i++)
  {
      writeReg(REG_TEMP1, RF_TEMP1_MEAS_START);
      temp_start = micros();
      while (done==0)
      {
          if (readReg(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING)
          {
              if ((micros() - temp_start) > 500)
                    done = -1; // timeout
                    //break;
          }
          else
          {
              done =1;
          }
      }

      TEMPREG = 166 - readReg(REG_TEMP2);
      if ((int)TEMPREG < 100)
      {
          this->Temperature += TEMPREG;
          counter++;
      }
  }
  if (counter >0)
      this->Temperature /= counter;
  else
      this->Temperature = -40;
  return (byte) done;
}
*/


/*
void RFM69::rcCalibration()
{
  writeReg(REG_OSC1, RF_OSC1_RCCAL_START);
  while ((readReg(REG_OSC1) & RF_OSC1_RCCAL_DONE) == 0x00);
}
*/

