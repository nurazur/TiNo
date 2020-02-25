/*
    CC1101.cpp - CC1101 module library
    Copyright (c) 2010 Michael.
    Author: Michael, <www.elechouse.com>
    Version: November 12, 2010

    This library is designed to use CC1101/CC1100 module on Arduino platform.
    CC1101/CC1100 module is an useful wireless module.Using the functions of the 
    library, you can easily send and receive data by the CC1101/CC1100 module. 
    Just have fun!
    For the details, please refer to the datasheet of CC1100/CC1101.
*/
extern long readVcc();

//#define LIBCALL_ENABLEINTERRUPT
//#include <EnableInterrupt.h>
#include <CC1101.h>

volatile int GenericRadio::RSSI;
volatile int16_t GenericRadio::FEI;
volatile byte GenericRadio::DATALEN;




CC1101* CC1101::selfPointer;

/****************************************************************/
#define     WRITE_BURST         0x40                        //write burst
#define     READ_SINGLE         0x80                        //read single
#define     READ_BURST          0xC0                        //read burst
#define     BYTES_IN_RXFIFO     0x7F                        //byte number in RXfifo

/****************************************************************/
//byte PaTabel[8] = {0x50 ,0x50 ,0x50 ,0x50 ,0x50 ,0x50 ,0x50 ,0x50};  //0 dBm
byte PaTabel[8] = {0xcb ,0xcb ,0xcb ,0xcb ,0xcb ,0xcb ,0xcb ,0xcb};  //7 dBm
//byte PaTabel[8] = {0xc2 ,0xcb ,0x81 ,0x50 ,0x27 ,0x1e ,0x0f ,0x03};  //7 bis -30dBm
//byte PaTabel[8] = {0x17 ,0x17 ,0x17 ,0x17 ,0x17 ,0x17 ,0x17 ,0x17};  //-20 dBm


/*
CC1101::CC1101(byte, byte): GenericRadio(RF69_FSTEP)
{
    FSTEP = CC1101_FSTEP;
    fdev = 0;
    something_received = false;
    _mode = IDLE;
}
*/

void CC1101::isr0() { selfPointer->interruptHandler(); }


/****************************************************************
*FUNCTION NAME:RegConfigSettings
*FUNCTION     :CC1101 register config //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101::RegConfigSettings(void) 
{
    //SpiWriteReg(CC1101_IOCFG2,   0x0B);     //serial clock.synchronous to the data in synchronous serial mode ????
    SpiWriteReg(CC1101_IOCFG2,   0x2E);     // High impedance (3-state)
    SpiWriteReg(CC1101_IOCFG0,   0x06);     //asserts when sync word has been sent/received, and de-asserts at the end of the packet 
    SpiWriteReg(CC1101_SYNC1,    0x2D);     // Network ID high Byte
    SpiWriteReg(CC1101_SYNC0,    0xD2);     // network ID Low Byte
    
    SpiWriteReg(CC1101_PKTLEN,   0x3D);     //61 bytes max length
    SpiWriteReg(CC1101_PKTCTRL1, 0x04);     //two status bytes will be appended to the payload of the packet,including RSSI LQI and CRC OK
                                            //No address check
    SpiWriteReg(CC1101_PKTCTRL0, 0x01);     //whitening off;CRC Enable variable length packets, packet length configured by the first byte after sync word
    SpiWriteReg(CC1101_ADDR,     0x00);     //address used for packet filtration.
    SpiWriteReg(CC1101_CHANNR,   0x00);
    //SpiWriteReg(CC1101_FSCTRL1,  0x08);  // original setting
    SpiWriteReg(CC1101_FSCTRL1,  0x06);
    SpiWriteReg(CC1101_FSCTRL0,  0x00);
    //SpiWriteReg(CC1101_FSCTRL0,  0x1C);  // 28 ;Frequency Offset added
    //SpiWriteReg(CC1101_FSCTRL0,  0xfd);    //-4
    
    SpiWriteReg(CC1101_FREQ2,    0x21);
    SpiWriteReg(CC1101_FREQ1,    0x44);
    SpiWriteReg(CC1101_FREQ0,    0xEC);// 865 MHz
    
    //SpiWriteReg(CC1101_MDMCFG4,  0x59); // 19.2 kBd - 325 kHz BW
    //SpiWriteReg(CC1101_MDMCFG4,  0x99); // 19.2 kBd - 160 kHz BW
    //SpiWriteReg(CC1101_MDMCFG4,  0xA9); // 19.2 kBd - 135 kHz BW
    SpiWriteReg(CC1101_MDMCFG4,  0xB9); // 19.2 kBd - 116 kHz BW
    //SpiWriteReg(CC1101_MDMCFG4,  0xC9); // 19.2 kBd - 101 kHz BW
    //SpiWriteReg(CC1101_MDMCFG4,  0xD9); // 19.2 kBd - 81 kHz BW
    //SpiWriteReg(CC1101_MDMCFG4,  0xF9); // 19.2 kBd - 58 kHz BW
    
    SpiWriteReg(CC1101_MDMCFG3,  0x83); // 19.2 kBd
    
    //SpiWriteReg(CC1101_MDMCFG2,  0x02); //default 16/16 bits sync
    //SpiWriteReg(CC1101_MDMCFG2,  0x01); // 15/16 bits sync
    SpiWriteReg(CC1101_MDMCFG2,  0x06);
    
    SpiWriteReg(CC1101_MDMCFG1,  0x12);  // normal 0x22 4 preamble bytes, here 3 Bytes preamble like TiNo    
    SpiWriteReg(CC1101_MDMCFG0,  0xF8);
    
    SpiWriteReg(CC1101_DEVIATN,  0x42); // 30 kHz
    
    //SpiWriteReg(CC1101_MCSM1 ,   0x30);  // NEW : stay in RX. After TX go IDLE.
    SpiWriteReg(CC1101_MCSM0 ,   0x18); // original
    
    
    //SpiWriteReg(CC1101_FOCCFG,   0x1D); //ok derived from SmartRF Studio 7
    //SpiWriteReg(CC1101_FOCCFG,   0x15); // 1/8 BW offset compensation
    SpiWriteReg(CC1101_FOCCFG,   0x16);  // 1/4 BW offset compensation
    //SpiWriteReg(CC1101_FOCCFG,   0x1C); //no frequency offset compensation
    //SpiWriteReg(CC1101_BSCFG,    0x1C);
    
    SpiWriteReg(CC1101_BSCFG,    0x6D);
    //SpiWriteReg(CC1101_AGCCTRL2, 0xC7); original setting (??)
    SpiWriteReg(CC1101_AGCCTRL2, 0x03);     // new for testing
    //SpiWriteReg(CC1101_AGCCTRL1, 0x00);
    //SpiWriteReg(CC1101_AGCCTRL0, 0xB2);   
    SpiWriteReg(CC1101_FREND1,   0xB6);
    SpiWriteReg(CC1101_FREND0,   0x10); //0 dBm, no pa ramping
    //SpiWriteReg(CC1101_FSCAL3,   0xEA);
    SpiWriteReg(CC1101_FSCAL3,   0xE9);
    SpiWriteReg(CC1101_FSCAL2,   0x2A);
    SpiWriteReg(CC1101_FSCAL1,   0x20);
    SpiWriteReg(CC1101_FSCAL0,   0x1F);   
    SpiWriteReg(CC1101_FSTEST,   0x59);
    SpiWriteReg(CC1101_TEST2,    0x81);
    SpiWriteReg(CC1101_TEST1,    0x35);
    SpiWriteReg(CC1101_TEST0,    0x09);   
}

/****************************************************************
*FUNCTION NAME:SpiInit
*FUNCTION     :spi communication initialization
*INPUT        :none
*OUTPUT       :none
****************************************************************/

void CC1101::SpiInit(void)
{
  // initialize the SPI pins
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT);
  pinMode(SS_PIN, OUTPUT);

  // enable SPI Master, MSB, SPI mode 0, FOSC/4
  //SpiMode(0);
  // enable SPI master with configuration byte specified
  SPCR = 0;
  SPCR |= (1<<SPE) | (1<<MSTR);
  
}


/****************************************************************
*FUNCTION NAME:SpiMode
*FUNCTION     :set spi mode
*INPUT        :        config               mode
               (0<<CPOL) | (0 << CPHA)       0
               (0<<CPOL) | (1 << CPHA)       1
               (1<<CPOL) | (0 << CPHA)       2
               (1<<CPOL) | (1 << CPHA)       3
*OUTPUT       :none
*/

void CC1101::SpiMode(byte config)
{
  byte tmp;

  // enable SPI master with configuration byte specified
  SPCR = 0;
  SPCR = (config & 0x7F) | (1<<SPE) | (1<<MSTR);
  tmp = SPSR;
  tmp = SPDR;
}
/****************************************************************/


/****************************************************************
*FUNCTION NAME:SpiTransfer
*FUNCTION     :spi transfer
*INPUT        :value: data to send
*OUTPUT       :data to receive
****************************************************************/

byte CC1101::SpiTransfer(byte value)
{
  SPDR = value;
  while (!(SPSR & (1<<SPIF))) ;
  return SPDR;
}


/****************************************************************
*FUNCTION NAME: GDO_Set()
*FUNCTION     : set GDO0,GDO2 pin
*INPUT        : none
*OUTPUT       : none
****************************************************************/
void CC1101::GDO_Set (void)
{
    pinMode(GDO0, INPUT);
    pinMode(GDO2, INPUT);
}

/****************************************************************
*FUNCTION NAME:Reset
*FUNCTION     :CC1101 reset //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101::Reset (void)
{
    digitalWrite(SS_PIN, LOW);
    delay(1);
    digitalWrite(SS_PIN, HIGH);
    delay(1);
    digitalWrite(SS_PIN, LOW);
    while(digitalRead(MISO_PIN));
    SpiTransfer(CC1101_SRES);
    while(digitalRead(MISO_PIN));
    digitalWrite(SS_PIN, HIGH);
}

/****************************************************************
*FUNCTION NAME:Init
*FUNCTION     :CC1101 initialization
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101::Init(void)
{
    SpiInit();                                      //spi initialization
    GDO_Set();                                      //GDO set
    digitalWrite(SS_PIN, HIGH);
    digitalWrite(SCK_PIN, HIGH);
    digitalWrite(MOSI_PIN, LOW);
    Reset();                                        //CC1101 reset
    RegConfigSettings();                            //CC1101 register config
    SpiWriteBurstReg(CC1101_PATABLE,PaTabel,8);     //CC1101 PATABLE config
}

bool CC1101::Initialize(byte freqBand, byte networkID, byte txpower) // frequencyband is not used on this chip
{
    (void) freqBand;
    SpiInit();                                      //spi initialization
    GDO_Set();                                      //GDO set
    digitalWrite(SS_PIN, HIGH);
    digitalWrite(SCK_PIN, HIGH);
    digitalWrite(MOSI_PIN, LOW);
    Reset();                                        //CC1101 reset
    RegConfigSettings();
    SpiWriteReg(CC1101_SYNC0, networkID);
    SpiWriteBurstReg(CC1101_PATABLE,&txpower,1);     //CC1101 power config
    this->vcc_dac = readVcc();
    selfPointer = this;
    attachInterrupt(digitalPinToInterrupt(GDO0), CC1101::isr0, FALLING);
    //enableInterrupt(GDO0, CC1101::isr0, FALLING);
    return true;
}


void CC1101::setPowerLevel(byte level)
{
    SpiWriteBurstReg(CC1101_PATABLE,&level,1);     //CC1101 power config
}

/****************************************************************
*FUNCTION NAME:SpiWriteReg
*FUNCTION     :CC1101 write data to register
*INPUT        :addr: register address; value: register value
*OUTPUT       :none
****************************************************************/
void CC1101::SpiWriteReg(byte addr, byte value)
{
    digitalWrite(SS_PIN, LOW);
    while(digitalRead(MISO_PIN));
    SpiTransfer(addr);
    SpiTransfer(value);
    digitalWrite(SS_PIN, HIGH);
}

/****************************************************************
*FUNCTION NAME:SpiWriteBurstReg
*FUNCTION     :CC1101 write burst data to register
*INPUT        :addr: register address; buffer:register value array; num:number to write
*OUTPUT       :none
****************************************************************/
void CC1101::SpiWriteBurstReg(byte addr, byte *buffer, byte num)
{
    byte i, temp;

    temp = addr | WRITE_BURST;
    digitalWrite(SS_PIN, LOW);
    while(digitalRead(MISO_PIN));
    SpiTransfer(temp);
    for (i = 0; i < num; i++)
    {
        SpiTransfer(buffer[i]);
    }
    digitalWrite(SS_PIN, HIGH);
}

/****************************************************************
*FUNCTION NAME:SpiStrobe
*FUNCTION     :CC1101 Strobe
*INPUT        :strobe: command; //refer define in CC1101.h//
*OUTPUT       :none
****************************************************************/
void CC1101::SpiStrobe(byte strobe)
{
    digitalWrite(SS_PIN, LOW);
    while(digitalRead(MISO_PIN));
    SpiTransfer(strobe);
    digitalWrite(SS_PIN, HIGH);
}

/****************************************************************
*FUNCTION NAME:SpiReadReg
*FUNCTION     :CC1101 read data from register
*INPUT        :addr: register address
*OUTPUT       :register value
****************************************************************/
byte CC1101::SpiReadReg(byte addr) 
{
    byte temp, value;

    temp = addr|READ_SINGLE;
    digitalWrite(SS_PIN, LOW);
    while(digitalRead(MISO_PIN));
    SpiTransfer(temp);
    value=SpiTransfer(0);
    digitalWrite(SS_PIN, HIGH);

    return value;
}

/****************************************************************
*FUNCTION NAME:SpiReadBurstReg
*FUNCTION     :CC1101 read burst data from register
*INPUT        :addr: register address; buffer:array to store register value; num: number to read
*OUTPUT       :none
****************************************************************/
void CC1101::SpiReadBurstReg(byte addr, byte *buffer, byte num)
{
    byte i,temp;

    temp = addr | READ_BURST;
    digitalWrite(SS_PIN, LOW);
    while(digitalRead(MISO_PIN));
    SpiTransfer(temp);
    for(i=0;i<num;i++)
    {
        buffer[i]=SpiTransfer(0);
    }
    digitalWrite(SS_PIN, HIGH);
}

/****************************************************************
*FUNCTION NAME:SpiReadStatus
*FUNCTION     :CC1101 read status register
*INPUT        :addr: register address
*OUTPUT       :status value
****************************************************************/
byte CC1101::SpiReadStatus(byte addr) 
{
    byte value,temp;

    temp = addr | READ_BURST;
    digitalWrite(SS_PIN, LOW);
    while(digitalRead(MISO_PIN));
    SpiTransfer(temp);
    value=SpiTransfer(0);
    digitalWrite(SS_PIN, HIGH);

    return value;
}


void CC1101::setFrequency(uint32_t freq)
{
    SpiWriteReg(CC1101_FREQ2,    freq >> 16);
    SpiWriteReg(CC1101_FREQ1,    freq >> 8);
    SpiWriteReg(CC1101_FREQ0,   freq );
}

void CC1101::setFrequencyMHz(float freqMHz)
{
    uint32_t freqSteps = (uint32_t) floor((freqMHz *1000000.0 / FSTEP)+0.5) + this->fdev;
    setFrequency(freqSteps);
}

uint32_t CC1101::getFrequency()
{
    return ((uint32_t)SpiReadReg(CC1101_FREQ2)<<16) + ((uint16_t)SpiReadReg(CC1101_FREQ1)<<8) + SpiReadReg(CC1101_FREQ0);
}



void CC1101::setDataShaping(uint8_t datashaping)
{
    byte mdmcfg2 = SpiReadReg(CC1101_MDMCFG2) & 0xCF;
    if (datashaping !=0) 
        SpiWriteReg(CC1101_MDMCFG2, mdmcfg2 | 0x10);// GFSK
    else 
        SpiWriteReg(CC1101_MDMCFG2, mdmcfg2); // 2-FSK
}



/****************************************************************
*FUNCTION NAME:SendData
*FUNCTION     :use CC1101 send data
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
*OUTPUT       :none
****************************************************************/
void CC1101::SendData(byte *txBuffer,byte size)
{
    SpiWriteReg(CC1101_TXFIFO,size);
    SpiWriteBurstReg(CC1101_TXFIFO,txBuffer,size);          //write data to send
    SpiStrobe(CC1101_STX);                                  //start send
   
    while (!digitalRead(GDO0));                             // Wait for GDO0 to be set -> sync transmitted  NEED TO IMPLEMENT A TIMEOUT!
    this->vcc_dac = readVcc();                              // Measure VCC during Burst
    while (digitalRead(GDO0));                              // Wait for GDO0 to be cleared -> end of packet
    SpiStrobe(CC1101_SFTX);                                 //flush TXfifo
}

void CC1101::send(const void* buffer, byte bufferSize)
{
    SendData((byte*)buffer, bufferSize);
}

/****************************************************************
*FUNCTION NAME:SetReceive
*FUNCTION     :set CC1101 to receive state
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101::SetReceive(void)
{
    SpiStrobe(CC1101_SRX);
    //_mode = RX;
}

/****************************************************************
*FUNCTION NAME:CheckReceiveFlag
*FUNCTION     :check receive data or not
*INPUT        :none
*OUTPUT       :flag: 0 no data; 1 receive data 
****************************************************************/
byte CC1101::CheckReceiveFlag(void)
{
    if(digitalRead(GDO0))           //receive data
    {
        while (digitalRead(GDO0));
        return 1;
    }
    else                            // no data
    {
        return 0;
    }
}

bool CC1101::receiveDone()
{
    if (something_received)
    {
        something_received =false;
        _mode = IDLE;
        
        return true;
    }
    
    else if (_mode != RX) 
    {
        SetReceive();
        _mode = RX;
        return false;
    }

    return false; // in RX mode but nothing received yet
}



void CC1101::interruptHandler() 
{  
    if (_mode == RX)
        if(ReceiveData_with_StatusBytes()) something_received = true;
}

/****************************************************************
*FUNCTION NAME:ReceiveData
*FUNCTION     :read data received from RXfifo
*INPUT        :rxBuffer: buffer to store data
*OUTPUT       :size of data received
****************************************************************/
byte CC1101::ReceiveData(byte *rxBuffer)
{
    byte size;
    byte status[2];

    if(SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO)
    {
        size=SpiReadReg(CC1101_RXFIFO);
        SpiReadBurstReg(CC1101_RXFIFO,rxBuffer,size);
        SpiReadBurstReg(CC1101_RXFIFO,status,2);
        SpiStrobe(CC1101_SFRX);
        return size;
    }
    else
    {
        SpiStrobe(CC1101_SFRX);
        return 0;
    }
    
}


uint8_t CC1101::ReadStatus(void)
{
    return SpiReadStatus(CC1101_MARCSTATE);
}


/****************************************************************
*FUNCTION NAME:ReceiveData
*FUNCTION     :read data received from RXfifo
*INPUT        :rxBuffer: buffer to store data
*OUTPUT       :size of data received
****************************************************************/
byte CC1101::ReceiveData_with_StatusBytes(byte *rxBuffer)
{
    //byte size;
    int rssi_dbm;
    
    
    uint8_t rxbytes = SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO;
    
    this->DATALEN=SpiReadReg(CC1101_RXFIFO);
    if(rxbytes) // read number of bytes in FIFO
    {
         // Read length from first Byte in FIFO (message payload)
        if (this->DATALEN>0)
        {
            SpiReadBurstReg(CC1101_RXFIFO,rxBuffer,this->DATALEN);
            
            byte pktctrl =  SpiReadReg(CC1101_PKTCTRL1) & 0x04;
            if (pktctrl)
            {
                rssi_dbm =  SpiReadReg(CC1101_RXFIFO);
                this->LQI = SpiReadReg(CC1101_RXFIFO);
                
                if (rssi_dbm < 128)
                {
                    rssi_dbm = rssi_dbm/2 - 74;
                }
                else
                {
                    rssi_dbm = (rssi_dbm-256)/2 - 74;
                }
                this->RSSI = rssi_dbm;
            }
        }
        
        SpiStrobe(CC1101_SFRX); // Flush RX FIFO
        return this->DATALEN;
    }
    else
    {
        this->DATALEN =0;
        SpiStrobe(CC1101_SIDLE);
        SpiStrobe(CC1101_SFRX);
        return this->DATALEN;
    }
    
}


/****************************************************************
*FUNCTION NAME:ReceiveData_with_StatusBytes
*FUNCTION     :read data received from RXfifo
*INPUT        :rxBuffer: buffer to store data
*OUTPUT       :size of data received
****************************************************************/
byte CC1101::ReceiveData_with_StatusBytes(void)
{
    //byte size;
    float rssi_dbm;
    uint8_t rxbytes;
    
    //CheckReceiveFlag(); //wait until GDO0 deasserts at end of RX Packet
    rxbytes = SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO;
    this->DATALEN=SpiReadReg(CC1101_RXFIFO);// read first Byte in RXFIFO
    
    if(rxbytes)
    {
        if (this->DATALEN >0)
        {
            SpiReadBurstReg(CC1101_RXFIFO,this->DATA,this->DATALEN);
            
            byte pktctrl =  SpiReadReg(CC1101_PKTCTRL1) & 0x04;
            if (pktctrl)
            {
                rssi_dbm =  SpiReadReg(CC1101_RXFIFO);
                this->LQI = SpiReadReg(CC1101_RXFIFO);
                
                if (rssi_dbm < 128)
                {
                    rssi_dbm = rssi_dbm/2.0 - 74;
                }
                else
                {
                    rssi_dbm = (rssi_dbm-256)/2.0 - 74;
                }
                this->RSSI = rssi_dbm * 2;
            }
            readFEI();
        }      
    }
    else
    {
        this->DATALEN =0;
    }

    SpiStrobe(CC1101_SFRX); // Flush RX Buffer
    return this->DATALEN;
}


void CC1101::Fscal(void)
{
    SpiStrobe(CC1101_SFSTXON);
}


void CC1101::sleep(void)
{
    _mode = IDLE;
    SpiStrobe(CC1101_SIDLE);
    SpiStrobe(CC1101_SPWD);
    
}

int16_t CC1101::readFEI(void)
{
    this->FEI = (char)SpiReadStatus(CC1101_FREQEST);
    return this->FEI;
}

byte CC1101::readTemperature(byte)
{
    return 0; // CC1101 has analog CMOS temperature meter
}



