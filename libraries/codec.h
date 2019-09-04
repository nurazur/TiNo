#ifndef my_codec_h
#define my_codec_h
#include <Arduino.h>


class fec_codec{
    public:
        //constructor
        fec_codec(unsigned char* codes);
        fec_codec();

        // encode a byte into a 16 bit integer
        uint16_t encode_8(unsigned char b);

        // encode a stream from buffer p_in into a buffer p_out with len codes
        void encode_block(unsigned char* p_in, unsigned char* p_out, unsigned char len);

        // take a 16 bit int and encode to 24 bit hamming 8/4 - disregard highest 4 bits (value shall be < 4096)
        void encode_24(uint16_t value, unsigned char *dest);

        // take a 16 bit int and encode to 16 bit hamming 8/4 - disregard highest 8 bits (value shall be < 256)
        void encode_16(uint16_t value, unsigned char *dest);

        // take a received byte and return a decoded nibble, if success
        bool decode_byte(unsigned char code, unsigned char &dest);
        bool decode_byte(unsigned char code, unsigned char &dest, char &num_errors);

        bool decode_bytes(unsigned char* encoded_msg, uint16_t *decoded_msg, byte num_bytes=2); // up to 4 bytes

        // take 3 bytes from an encoded string and return a 12-bit decoded number.
        bool decode_24(unsigned char* encoded_msg, uint16_t *decoded_msg);

        // fill target p_out with len decoded bytes, previously encoded with hamming_encode_block
        bool decode_block(unsigned char* p_in, unsigned char* p_out, unsigned char len);
        bool decode_block(unsigned char* p_in, unsigned char* p_out, unsigned char len, uint16_t &num_errors);


    private:
        // count the bits set to "1" in a byte n
        unsigned char bitcount(unsigned char n);

        //hamming codes for FEC
        unsigned char *hamming_codes;
};


#define DELTA 0x9E3779B9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (cryptKey[(uint8_t)((p&3)^e)] ^ z)))

#define encrypt true
#define decrypt false
class xxtea{
    public:
        //constructor
        xxtea(const uint8_t* key, uint8_t* data_buf);
        uint8_t crypter_pad(bool sending, uint8_t len);
        uint8_t crypter(bool sending, uint8_t len);
        uint8_t encrypt_(uint8_t len);

    private:
        uint32_t cryptKey[4];
        uint32_t seqNum;
        static long rf12_seq;
        uint8_t *_data;
};

#define FORWARD true
#define REVERSE false
void interleave (unsigned char* s, int rows, bool forward =FORWARD);


#endif