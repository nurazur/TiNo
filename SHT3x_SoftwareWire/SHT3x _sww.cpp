#include "SHT3x_sww.h"

// high repeatability, clock stretch
// SoftwareWire only works with clock stretch
#define _MeasMSB 0x2C
#define _MeasLSB 0x06
//#define _MeasLSB 0x10

// high repeatability, no clock stretch
//#define _MeasMSB 0x24
//#define _MeasLSB 0x00

#define UpdateIntervalMillisec 500
#define TimeoutMillisec 100

SHT3x::SHT3x(SoftwareWire* I2C, int Address, SHT3xSensor SensorType)
{
	_Error = noError;
	SetAddress((uint8_t)Address);
	_SensorType = SensorType;
    i2c = I2C;
	
}

void	SHT3x::Begin()
{
	i2c->begin();
}

void SHT3x::UpdateData()
{
	_Error = noError;
    static uint32_t _LastUpdateMillisec = 0;
	if ((_LastUpdateMillisec == 0) || ((millis()-_LastUpdateMillisec)>=UpdateIntervalMillisec))
	{
		SendCommand(_MeasMSB,_MeasLSB);
		if (_Error == noError)
		{
			i2c->requestFrom(_Address, (uint8_t)6);
			uint32_t WaitingBeginTime = millis();
			while ((i2c->available()<6) && ((millis() - WaitingBeginTime) < TimeoutMillisec))
			{
				//Do nothing, just wait
			}
			if ((millis() - WaitingBeginTime) < TimeoutMillisec)
			{
				_LastUpdateMillisec = WaitingBeginTime; 
				uint8_t data[6];
				for (uint8_t i = 0; i<6; i++)
				{
					data[i] = i2c->read();
				}
				
				if ((CRC8(data[0],data[1],data[2])) && (CRC8(data[3],data[4],data[5])))
				{
					uint16_t TemperatureRaw 	= (data[0]<<8)+(data[1]<<0);
					uint16_t RelHumidityRaw 	= (data[3]<<8)+(data[4]<<0);
					_TemperatureCeil =	((float) TemperatureRaw) * 0.00267033 - 45.;
					_RelHumidity =	((float) RelHumidityRaw) * 0.0015259;
					_Error = noError;
				}
				else
				{
					_Error = DataCorrupted;
				}
			} 
			else //Timeout
			{
				_Error = Timeout;
			}
		} 
		else //Error after message send
		{
			// Nothing to do, measurement commands will return NULL because of Error != noError
		}	
	}
	else //LastUpdate was too recently
	{
		//Nothing to do, wait for next call
	}
}


// simplified function if I just want one single reading.
void SHT3x::GetData()
{
	_Error = noError;

    SendCommand(_MeasMSB,_MeasLSB);
    if (_Error == noError)
    {
        i2c->requestFrom(_Address, (uint8_t)6);
        uint32_t WaitingBeginTime = millis();
        while ((i2c->available()<6) && ((millis() - WaitingBeginTime) < TimeoutMillisec))
        {
            //Do nothing, just wait
        }
        if ((millis() - WaitingBeginTime) < TimeoutMillisec)
        {
            uint8_t data[6];
            for (uint8_t i = 0; i<6; i++)
            {
                data[i] = i2c->read();
            }
            
            if ((CRC8(data[0],data[1],data[2])) && (CRC8(data[3],data[4],data[5])))
            {
                uint16_t TemperatureRaw 	= (data[0]<<8)+(data[1]<<0);
                uint16_t RelHumidityRaw 	= (data[3]<<8)+(data[4]<<0);
                _TemperatureCeil =	((float) TemperatureRaw) * 0.00267033 - 45.;
                _RelHumidity =	((float) RelHumidityRaw) * 0.0015259;
                _Error = noError;
            }
            else
            {
                _Error = DataCorrupted;
            }
        } 
        else //Timeout
        {
            _Error = Timeout;
        }
    } 
    else //Error after message send
    {
        // Nothing to do, measurement commands will return NULL because of Error != noError
    }	
}

float	SHT3x::GetTemperature()
{
    return _TemperatureCeil;
}

float	SHT3x::GetRelHumidity()
{
	return ReturnValueIfError(_RelHumidity);
}

float	SHT3x::GetAbsHumidity(AbsHumidityScale Scale)
{
	float 	millikelvins = (GetTemperature() +273.15) / 1000.;
	float 	Pressure = 0.;
    float _AbsHumPoly[6] ={-157.004, 3158.0474, -25482.532, 103180.197, -209805.497, 171539.883}; 
	for (uint8_t i = 0; i<6; i++)
	{
		float term = 1.;
		for (uint8_t j = 0; j<i; j++)
		{
			term *= millikelvins;
		}
		Pressure += term*_AbsHumPoly[i];
	}
	Pressure *= GetRelHumidity();
	switch (Scale)
	{
		case mmHg:
		{
			//Already in mm Hg
			break;
		}
		case Torr:
		{
			//Already in Torr
			break;
		}
		case Pa:
		{
			Pressure *= 133.322;
			break;
		}
		case Bar:
		{
			Pressure *= 0.0013332;
			break;
		}
		case At:
		{
			Pressure *= 0.0013595;
			break;
		}
		case Atm:
		{
			Pressure *= 0.0013158;
			break;
		}
		case mH2O:
		{
			Pressure *= 0.013595;
			break;
		}
		case psi:
		{
			Pressure *= 0.019337;
			break;
		}
		default:
		{
			break;
		}
	}
	return ReturnValueIfError(Pressure);
}


void 	SHT3x::SoftReset()
{
	SendCommand(0x30, 0xA2);
}

void	SHT3x::HardReset(uint8_t HardResetPin)
{
	if (HardResetPin<100) //MEGA2560 have only 68 pins
	{
		pinMode(HardResetPin,OUTPUT);
		digitalWrite(HardResetPin,LOW);
		delayMicroseconds(1); //Reset pin have to be LOW at least 350ns, so 1 usec is enough. I think, there is no need for micros() for 1 usec.
		digitalWrite(HardResetPin,HIGH);
	}
}

void	SHT3x::HeaterOn()
{
	SendCommand(0x30, 0x6D);
}

void	SHT3x::HeaterOff()
{
	SendCommand(0x30, 0x66);
}

void	SHT3x::SetAddress(uint8_t Address)
{
	if ((Address == 0x44) || (Address == 0x45))
	{
		_Address = Address;
	}
	else
	{
		_Error = WrongAddress;
	}
}



void 	SHT3x::I2CError(uint8_t I2Canswer)
{
    switch (I2Canswer)
    {
        case 0:
        {
            _Error = noError;
            break;
        }
        case 1:
        {
            _Error = TooMuchData;
            break;
        }
        case 2:
        {
            _Error = AddressNACK;
            break;
        }
        case 3:
        {
            _Error = DataNACK;
            break;
        }
        case 4:
        {
            _Error = OtherI2CError;
            break;
        }
        default:
        {
            _Error = UnexpectedError;
            break;
        }
    }
}

void SHT3x::SendCommand(uint8_t MSB, uint8_t LSB)
{
	i2c->beginTransmission(_Address);
	// Send Soft Reset command
	i2c->write(MSB);
	i2c->write(LSB);
	// Stop I2C transmission
    i2c->endTransmission();
}

bool 	SHT3x::CRC8(uint8_t MSB, uint8_t LSB, uint8_t CRC)
{
	/*
	*	Name  : CRC-8
	*	Poly  : 0x31	x^8 + x^5 + x^4 + 1
	*	Init  : 0xFF
	*	Revert: false
	*	XorOut: 0x00
	*	Check : for 0xBE,0xEF CRC is 0x92
	*/
	uint8_t crc = 0xFF;
	uint8_t i;
	crc ^=MSB;

		for (i = 0; i < 8; i++)
			crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;

	crc ^= LSB;
	for (i = 0; i < 8; i++)
				crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;

	if (crc==CRC)
	{
		return true;
	}
	else
	{
		return false;
	}
}

uint8_t	SHT3x::GetError()
{
	return _Error;
}

float	SHT3x::ReturnValueIfError(float Value)
{
	if (_Error == noError)
	{
		return Value;
	}
	else
    {
        return 0.;
    }
        /*
	{
		if (_ValueIfError == PrevValue)
		{
			return Value;
		}
		else
		{
			return 0.;
		}
	}
    */
    
}
