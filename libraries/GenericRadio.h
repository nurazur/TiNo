#ifndef GENERICRADIO
#define GENERICRADIO

class GenericRadio{
    public:
        GenericRadio(double f): FSTEP(f) {}
        virtual bool Initialize(byte , byte , byte ) = 0;
        virtual void setHighPower(bool , bool , bool ) = 0;
        virtual void setFrequencyMHz(float) = 0;
        virtual void setFrequency(uint32_t ) = 0;
        virtual uint32_t getFrequency() = 0;
        virtual void setDataShaping(uint8_t) = 0;
        virtual void setPowerLevel(byte) =0;
        virtual void sleep() = 0;
        virtual bool receiveDone() = 0;
        virtual byte readTemperature(byte) = 0;
        virtual void send(const void*, byte) =  0;
        virtual void OOKTransmitBegin() =0;

        byte *DATA;
        int fdev;
        long vcc_dac;
        const double FSTEP;
        static volatile byte DATALEN;
        static volatile int RSSI;
        static volatile int16_t FEI;
};

#endif