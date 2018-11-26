// based on jeelib

#ifndef SPI_COMMON_h
#define SPI_COMMON_h

#include <stdint.h>

#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__) // attiny84 pin assignment
  #define USI_DDR               DDRA
  #define USI_PORT              PORTA
  #define DI_BIT                6
  #define DO_BIT                5
  #define USCK_BIT              4
#endif

class SPIclass {
  public:
    static void begin(void);
    static uint8_t transfer(uint8_t out);
};

extern SPIclass SPI;

#endif
