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

#define MAXINPUTSTRINGLENGTH 32


typedef struct{
int16_t avg=0;
int16_t max;
int16_t min;
uint8_t num_averages=10;
uint8_t count;
float stddev=0;
bool success;
int temperature;
} FeiMeasurement; 

class Calibration
{
    public:
      Calibration (Configuration& C, Stream* S, myMAC* Mac=NULL, int softwarebuild=0, uint8_t *encryptionkey = NULL);
      
      Calibration (Configuration& C, Stream* S, int softwarebuild=0);
      
            
      // calculates the checksum over the configuration data stored in EEprom
      uint8_t checksum(void);
      
      // calculates the checksum over configuration data stored in EEprom and 
      // compares to the checksum value stored in EEprom.
      bool verify_checksum(void);
            
      // actual calibration routine
      void calibrate(); 
      
      // check EEPROM and enter Calibratiion mode if there is a problem 
      // check if there is a request from a calibration program to enter calibration state. 
      void configure();
      
      // receives a reference meassage and measures the FEI 10x, takes the aveage and retursn the result in the FeiMeasurement structure
      bool radio_fei_cal(FeiMeasurement &fei);

    protected:
      void error_message(char* msg);
      
      // parser for strings received over serial port
      bool parse (char* str2parse);
      bool authenticate (char* str2parse);
    
    private:
        uint8_t* encryption_key;
        Stream *serial;
        Configuration &Cfg;
        myMAC *Mac;
        int SoftwareVersion;
        bool authenticated;
        
};