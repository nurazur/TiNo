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


#include "codec.h"
//hamming codes for FEC
//                                      0       1     2     3     4     5     6     7     8     9    10    11     12   13    14    15
static unsigned char HammingCodes[] = {0x15, 0x02, 0x49, 0x5e, 0x64, 0x73, 0x38, 0x2f, 0xd0, 0xc7, 0x8c, 0x9b, 0xa1, 0xb6, 0xfd, 0xea};

/*****************************************************************************/
/****                         CLASS Fec_codec                             ****/
/*****************************************************************************/

fec_codec::fec_codec(unsigned char* codes):hamming_codes(codes)
{
}

fec_codec::fec_codec()
{
    hamming_codes = HammingCodes;
}

unsigned char fec_codec::bitcount(unsigned char n)
{
    unsigned char count = 0;
    while (n>0)
    {
        count += 1;
        n &= (n-1);
    }
    return count;
}

uint16_t fec_codec::encode_8(unsigned char byte)
{
    uint16_t lnibble = byte & 0xf;
    uint16_t hnibble = (byte >> 4) &0xf;

    uint16_t result = (uint16_t) this->hamming_codes[hnibble];
    result = (result << 8); //& 0xff00;
    result |= (uint16_t) this->hamming_codes[lnibble];
    return result;
    //return (uint16_t) (codes[hnibble] << 8) | codes[lnibble];
}

//fill the target p_out with len codes  from buffer p_in
void fec_codec::encode_block(unsigned char* p_in, unsigned char* p_out, unsigned char len)
{
    for (unsigned char i =0; i< len; i++)
    {
        uint8_t j = i<<1;
        p_out[j] =         hamming_codes[p_in[i] & 0xf];
        p_out[j | 0x1] =   hamming_codes[(p_in[i]>>4) &0xf];
    }
}

bool fec_codec::decode_byte(unsigned char code, unsigned char &dest)
{
    for (unsigned char i=0; i<16; i++) // go over all 16 codes, representing 0...9,a...f
    {
        unsigned char count = this->bitcount(code ^ this->hamming_codes[i]);
        if (count <= 1)
        {
            dest = i;
            return true;
        }
    }
    return false; // no code match
}


bool fec_codec::decode_byte(unsigned char code, unsigned char &dest, char &num_errors)
{
    num_errors = 8;
    for (unsigned char i=0; i<16; i++) // go over all 16 codes, representing 0...9,a...f
    {
        unsigned char count = this->bitcount(code ^ this->hamming_codes[i]);
        if (count <= 1)
        {
            num_errors = count;
            dest = i;
            return true;
        }
        else
        {
            if (count < num_errors) num_errors =  count;
        }

    }
    return false; // no code match
}

// fill target p_out with len decoded bytes, previously encoded with hamming_encode_block
// p_in is the undecoded message
// p_out is the decoded message, and is 1/2 length of p_in
// len ist length of p_in buffer. So length of p_out buffer must be 2 * len.
bool fec_codec::decode_block(unsigned char* p_in, unsigned char* p_out, unsigned char len)
{
    for (unsigned char i=0; i<len; i++)
    {
        unsigned char lnibble, hnibble;

        uint8_t j = i<<1;
        if (!this->decode_byte(p_in[j],     lnibble)) return false;
        if (!this->decode_byte(p_in[j|0x1], hnibble)) return false;
        p_out[i] = lnibble | (hnibble<<4);
    }
    return true;
}

bool fec_codec::decode_block(unsigned char* p_in, unsigned char* p_out, unsigned char len, uint16_t &num_errors)
{
    num_errors =0;
    for (unsigned char i=0; i<len; i++)
    {
        unsigned char lnibble, hnibble;
        char errors;

        uint8_t j = i<<1;
        if (!this->decode_byte(p_in[j],     lnibble, errors))
        {
            return false;
        }
        else
        {
            num_errors += errors;
        }

        if (!this->decode_byte(p_in[j|0x1], hnibble, errors))
        {
            return false;
        }
        else
        {
            num_errors += errors;
        }

        p_out[i] = lnibble | (hnibble<<4);
    }
    return true;
}

// little endian encoding of a 12 bit number.
void fec_codec::encode_24(uint16_t value, unsigned char *dest)
{
    dest[2] = this->hamming_codes [value &0xf];
    dest[1] = this->hamming_codes [(value>>4) &0xf];
    dest[0] = this->hamming_codes [(value>>8) &0xf];
}

void fec_codec::encode_16(uint16_t value, unsigned char *dest)
{
    dest[1] = this->hamming_codes [value &0xf];
    dest[0] = this->hamming_codes [(value>>4) &0xf];
}


bool fec_codec::decode_bytes(unsigned char* encoded_msg, uint16_t *decoded_msg, byte num_bytes)
{
    uint16_t decval =0;
    bool success = true;
    unsigned char c;
    for (byte i=0; i<num_bytes; i++)
    {
        if (this->decode_byte(encoded_msg[i], c))
        {
            decval <<= 4;
            decval |= c;
        }
        else
        {
            success = false;
        }
    }
    //Serial.print(decval, HEX);
    //Serial.println("");
    *decoded_msg = decval;
    return success;
}

bool fec_codec::decode_24(unsigned char* encoded_msg, uint16_t *decoded_msg)
{
    return this->decode_bytes(encoded_msg, decoded_msg, 3);
}


/*****************************************************************************/
/****   CLASS XXTEA                                                        ***/
/*****************************************************************************/

//constructor
xxtea::xxtea(const uint8_t* key, uint8_t* data_buf)
{
    for (uint8_t i = 0; i < 16; ++i)
      ((uint8_t*) cryptKey)[i] = key[i];

    this->_data = data_buf;
}


/* Encryption and decryption function. */
/* gets the data length and returns the effectively encrypted/decrypted data length. */

uint8_t xxtea::crypter_pad(bool sending, uint8_t rf12_len)
{
    uint32_t y, z, sum, *v = (uint32_t*) _data;
    uint8_t p, e, rounds = 6;

    if (sending)
    {
        // pad with 1..4-byte sequence number
        *(uint32_t*)(_data + rf12_len) = ++seqNum;
        uint8_t pad = 3 - (rf12_len & 3);
        rf12_len += pad;
        _data[rf12_len] &= 0x3F;
        _data[rf12_len] |= pad << 6;
        ++rf12_len;
        // actual encoding
        char n = rf12_len / 4;
        if (n > 1)
        {
          sum = 0;
          z = v[n-1];
          do {
            sum += DELTA;
            e = (sum >> 2) & 3;
            for (p=0; p<n-1; p++)
                y = v[p+1], z = v[p] += MX;
            y = v[0];
            z = v[n-1] += MX;
          } while (--rounds);
        }
    }
    else
    {
        // actual decoding
        char n = rf12_len / 4;
        if (n > 1) {
          sum = rounds*DELTA;
          y = v[0];
          do {
            e = (sum >> 2) & 3;
            for (p=n-1; p>0; p--)
              z = v[p-1], y = v[p] -= MX;
            z = v[n-1];
            y = v[0] -= MX;
          } while ((sum -= DELTA) != 0);
        }
        // strip sequence number from the end again
        if (n > 0)
        {
            uint8_t pad = _data[--rf12_len] >> 6;
            rf12_seq = _data[rf12_len] & 0x3F;
            while (pad-- > 0)
            rf12_seq = (rf12_seq << 8) | _data[--rf12_len];
        }
    }
    return rf12_len;
}

// crypter without padding because length is modulo 4
uint8_t xxtea::crypter(bool sending, uint8_t rf12_len)
{
    uint32_t y, z, sum, *v = (uint32_t*) _data;
    uint8_t p, e, rounds = 6;

    if (sending)
    {
        // actual encoding
        char n = rf12_len / 4;
        if (n > 1)
        {
          sum = 0;
          z = v[n-1];
          do {
            sum += DELTA;
            e = (sum >> 2) & 3;
            for (p=0; p<n-1; p++)
                y = v[p+1], z = v[p] += MX;
            y = v[0];
            z = v[n-1] += MX;
          } while (--rounds);
        }
    }
    else
    {
        // actual decoding
        char n = rf12_len / 4;
        if (n > 1) {
          sum = rounds*DELTA;
          y = v[0];
          do {
            e = (sum >> 2) & 3;
            for (p=n-1; p>0; p--)
              z = v[p-1], y = v[p] -= MX;
            z = v[n-1];
            y = v[0] -= MX;
          } while ((sum -= DELTA) != 0);
        }
    }
    return rf12_len;
}

// crypter without padding because length is modulo 4
uint8_t xxtea::encrypt_(uint8_t rf12_len)
{
    uint32_t y, z, sum, *v = (uint32_t*) _data;
    uint8_t p, e, rounds = 6;

    //if (sending)
    {
        // actual encoding
        char n = rf12_len / 4;
        if (n > 1)
        {
          sum = 0;
          z = v[n-1];
          do {
            sum += DELTA;
            e = (sum >> 2) & 3;
            for (p=0; p<n-1; p++)
                y = v[p+1], z = v[p] += MX;
            y = v[0];
            z = v[n-1] += MX;
          } while (--rounds);
        }
    }
    /*
    else
    {
        // actual decoding
        char n = rf12_len / 4;
        if (n > 1) {
          sum = rounds*DELTA;
          y = v[0];
          do {
            e = (sum >> 2) & 3;
            for (p=n-1; p>0; p--)
              z = v[p-1], y = v[p] -= MX;
            z = v[n-1];
            y = v[0] -= MX;
          } while ((sum -= DELTA) != 0);
        }
    }
    */
    return rf12_len;
}


/********************** INTERLEAVER  *********************/

// block interleaver 1 byte wide with arbitrary number of rows.
// Is not symmetrical, i.e there are 2 differnt functions for encoding and decoding.
void interleave (unsigned char* s, int rows, bool forward)
{
    int i;
    unsigned char d[rows], bit;
    for (i=0; i<rows; i++) d[i] =0;

    if (forward)
    {
        for (i=0; i<8*rows; i++)
        {
            bit = (s[i/8] >> (i%8)) &0x1;
            bit <<= i/rows;

            d[i%rows] |= bit;
        }
    }
    else
    {
        for (i=0; i<8*rows; i++)
        {
            bit = (s[i%rows] >> (i/rows)) &0x1;
            bit <<= i%8;

            d[i/8] |= bit;
        }

    }
    for (i=0; i<rows; i++) s[i] = d[i]; // not necessary; can make d static and return a pointer to d
}


