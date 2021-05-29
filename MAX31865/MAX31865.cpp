/**************************************************************************
 * Arduino driver library for the MAX31865.
 *
 * Copyright (C) 2015 Ole Wolf <wolf@blazingangles.com>
 *
 *
 * Wire the circuit as follows, assuming that level converters have been
 * added for the 3.3V signals:
 *
 *    Arduino Uno            -->  MAX31865
 *    ------------------------------------
 *    CS: any available pin  -->  CS
 *    MOSI: pin 11           -->  SDI (mandatory for hardware SPI)
 *    MISO: pin 12           -->  SDO (mandatory for hardware SPI)
 *    SCK: pin 13            -->  SCLK (mandatory for hardware SPI)
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <MAX31865.h>

/**
 * The constructor for the MAX31865_RTD class registers the CS pin and
 * configures it as an output.
 *
 * @param [in] type type of RTD .
 * @param [in] cs_pin Arduino pin selected for the CS signal.
 * @param [in] rtd_rref reference resistor value. if omitted will be setup to default
              400 for PT100 and 4000 for PT1000
 */
MAX31865_RTD::MAX31865_RTD( ptd_type type, uint8_t cs_pin, uint16_t rtd_rref )
{
  /* Set the type of PTD. */
  this->type = type;

  /* CS pin for the SPI device. */
  this->cs_pin = cs_pin;
  pinMode( this->cs_pin, OUTPUT );

  /* Pull the CS pin high to avoid conflicts on SPI bus. */
  digitalWrite( this->cs_pin, HIGH );

  // reference resistor value set ?
  if (rtd_rref)
    this->configuration_rtd_rref = rtd_rref;
  else
    this->configuration_rtd_rref = type == RTD_PT100 ? RTD_RREF_PT100 : RTD_RREF_PT1000;
}

/**
 * Configure the MAX31865.  The parameters correspond to Table 2 in the MAX31865
 * datasheet.  The parameters are combined into a control bit-field that is stored
 * internally in the class for later reconfiguration, as are the fault threshold values.
 *
 * @param [in] v_bias Vbias enabled (@a true) or disabled (@a false).
 * @param [in] conversion_mode Conversion mode auto (@a true) or off (@a false).
 * @param [in] one_shot 1-shot measurement enabled (@a true) or disabled (@a false).
 * @param [in] three_wire 3-wire enabled (@a true) or 2-wire/4-wire (@a false).
 * @param [in] fault_detection Fault detection cycle control (see Table 3 in the MAX31865
 *             datasheet).
 * @param [in] fault_clear Fault status auto-clear (@a true) or manual clear (@a false).
 * @param [in] filter_50hz 50 Hz filter enabled (@a true) or 60 Hz filter enabled
 *             (@a false).
 * @param [in] low_threshold Low fault threshold.
 * @param [in] high_threshold High fault threshold.
*/


void MAX31865_RTD::configure( bool v_bias, bool conversion_mode, bool one_shot,
                              bool three_wire, uint8_t fault_cycle, bool fault_clear,
                              bool filter_50hz, uint16_t low_threshold,
                              uint16_t high_threshold )
{
  uint8_t control_bits = 0;

  /* Assemble the control bit mask. */
  control_bits |= ( v_bias ? VBIAS_ENABLED : VBIAS_DISABLED );
  control_bits |= ( conversion_mode ? CONVERSION_MODE_AUTO : CONVERSION_MODE_OFF );
  control_bits |= ( one_shot ? ONESHOT_ENABLED : ONESHOT_DISABLED);
  control_bits |= ( three_wire ? 0x10 : 0 );
  control_bits |= fault_cycle & 0b00001100;
  control_bits |= ( fault_clear ? FAULT_STATUS_AUTO_CLEAR : 0 );
  control_bits |= ( filter_50hz ? FILTER_50HZ : 0 );

  /* Store the control bits and the fault threshold limits for reconfiguration
     purposes. */
  this->configuration_control_bits   = control_bits;
  this->configuration_low_threshold  = low_threshold;
  this->configuration_high_threshold = high_threshold;
  
  /* Perform an full reconfiguration */
  reconfigure( true );
}

/**
 * ReConfigure the MAX31865.  The parameters correspond to Table 2 in the MAX31865
 * datasheet.  The parameters use other control bit-field that is stored internally 
 * in the class and change only new values
 *
 * @param [in] v_bias Vbias enabled (@a true) or disabled (@a false).
 * @param [in] conversion_mode Conversion mode auto (@a true) or off (@a false).
 * @param [in] one_shot 1-shot measurement enabled (@a true) or disabled (@a false).
 * @param [in] fault_detection Fault detection cycle control (see Table 3 in the MAX31865
 *             datasheet).
*/
void MAX31865_RTD::configure( bool v_bias, bool conversion_mode, bool one_shot, uint8_t fault_cycle )
{
  /* Use the stored the control bits, and set new ones only */
  this->configuration_control_bits &= ~ (0x80 | 0x40 | 0x20 | 0b00001100);

  this->configuration_control_bits |= ( v_bias ? 0x80 : 0 );
  this->configuration_control_bits |= ( conversion_mode ? 0x40 : 0 );
  this->configuration_control_bits |= ( one_shot ? 0x20 : 0 );
  this->configuration_control_bits |= fault_cycle & 0b00001100;

  /* Perform light configuration */
  reconfigure( false );
}

/**
 * Reconfigure the MAX31865 by writing the stored control bits and the stored fault
 * threshold values back to the chip.
 *
 * @param [in] full true to send also threshold configuration
 */ 
void MAX31865_RTD::reconfigure( bool full )
{
  /* Write the threshold values. */
  if (full)
  {
    uint16_t threshold ;

    digitalWrite( this->cs_pin, LOW );
    SPI.transfer( 0x83 );
    threshold = this->configuration_high_threshold ;
    SPI.transfer( ( threshold >> 8 ) & 0x00ff );
    SPI.transfer(   threshold        & 0x00ff );
    threshold = this->configuration_low_threshold ;
    SPI.transfer( ( threshold >> 8 ) & 0x00ff );
    SPI.transfer(   threshold        & 0x00ff );
    digitalWrite( this->cs_pin, HIGH );
  }

  /* Write the configuration to the MAX31865. */
  /*
  digitalWrite( this->cs_pin, LOW );
  SPI.transfer( 0x80 );
  SPI.transfer( this->configuration_control_bits );
  digitalWrite( this->cs_pin, HIGH );
  */
  
  writeRegister8(MAX31865_CONFIG_REG, this->configuration_control_bits);
}

void MAX31865_RTD::writeRegister8(uint8_t r, uint8_t value)
{
  digitalWrite( this->cs_pin, LOW );
  SPI.transfer( r | 0x80 );
  SPI.transfer( value );
  digitalWrite( this->cs_pin, HIGH );
}


uint8_t MAX31865_RTD::readRegister8(uint8_t reg)
{
  uint8_t content;
  digitalWrite( this->cs_pin, LOW );
  SPI.transfer( reg );
  content = SPI.transfer(0x00);
  digitalWrite( this->cs_pin, HIGH );
  return content;
}

/**************************************************************************/
/*!
    @brief Enable the bias voltage on the RTD sensor
    @param b If true bias is enabled, else disabled
*/
/**************************************************************************/
void MAX31865_RTD::enableBias(bool b) 
{
  
  uint8_t t = readRegister8(MAX31865_CONFIG_REG);
  if (b) 
  {
        t |= VBIAS_ENABLED; // enable bias
  }
  else 
  {
        t &= ~VBIAS_ENABLED; // disable bias
  }
  writeRegister8(MAX31865_CONFIG_REG, t);
}


/**
 * Apply the Callendar-Van Dusen equation to convert the RTD resistance
 * to temperature:
 *
 *   \f[
 *   t=\frac{-A\pm \sqrt{A^2-4B\left(1-\frac{R_t}{R_0}\right)}}{2B}
 *   \f],
 *
 * where
 *
 * \f$A\f$ and \f$B\f$ are the RTD coefficients, \f$R_t\f$ is the current
 * resistance of the RTD, and \f$R_0\f$ is the resistance of the RTD at 0
 * degrees Celcius.
 *
 * For more information on measuring with an RTD, see:
 * <http://newton.ex.ac.uk/teaching/CDHW/Sensors/an046.pdf>.
 *
 * @param [in] resistance The measured RTD resistance.
 * @return Temperature in degrees Celcius.
 */
double MAX31865_RTD::temperature() const
{
  static const double a2   = 2.0 * RTD_B;
  static const double b_sq = RTD_A * RTD_A;

  const double rtd_resistance =
    ( this->type == RTD_PT100 ) ? RTD_RESISTANCE_PT100 : RTD_RESISTANCE_PT1000;

  double c = 1.0 - resistance( ) / rtd_resistance;
  double D = b_sq - 2.0 * a2 * c;
  double temperature_deg_C = ( -RTD_A + sqrt( D ) ) / a2;

  return( temperature_deg_C );
}

/**
 * Read all settings and measurements from the MAX31865 and store them
 * internally in the class.
 *
 * @return Fault status byte
 */
uint8_t MAX31865_RTD::read_all()
{
  uint16_t combined_bytes;

  /* Start the read operation. */
  digitalWrite( this->cs_pin, LOW );
  /* Tell the MAX31865 that we want to read, starting at register 0. */
  SPI.transfer( 0x00 );

  /* Read the MAX31865 registers in the following order:
       Configuration
       RTD
       High Fault Threshold
       Low Fault Threshold
       Fault Status */

  this->measured_configuration = SPI.transfer( 0x00 );

  combined_bytes  = SPI.transfer( 0x00 ) << 8;
  combined_bytes |= SPI.transfer( 0x00 );
  this->measured_resistance = combined_bytes >> 1;

  combined_bytes  = SPI.transfer( 0x00 ) << 8;
  combined_bytes |= SPI.transfer( 0x00 );
  this->measured_high_threshold = combined_bytes ;

  combined_bytes  = SPI.transfer( 0x00 ) << 8;
  combined_bytes |= SPI.transfer( 0x00 );
  this->measured_low_threshold = combined_bytes ;

  this->measured_status = SPI.transfer( 0x00 );

  digitalWrite( this->cs_pin, HIGH );

  /* Reset the configuration if the measured resistance is
     zero or a fault occurred. */
  if(   this->measured_resistance == 0 || this->measured_status != 0  )
    reconfigure( true );

  return( status() );
}

/**
 * Read fault status
 * internally in the class.
 *
 * @return Fault status byte
 */
uint8_t MAX31865_RTD::fault_status()
{
  

  /* Start the read operation. */
  
  //
  this->measured_status = readRegister8(MAX31865_FAULTSTAT_REG);
  return this->measured_status;
  //
  /*
  uint8_t fault_status ;
  digitalWrite( this->cs_pin, LOW );
  //Tell the MAX31865 that we want to read, starting at register 7
  SPI.transfer( MAX31865_FAULTSTAT_REG );
  fault_status = this->measured_status = SPI.transfer( 0x00 );
  digitalWrite( this->cs_pin, HIGH );
  return( fault_status );
  */
}

/**
 * Read configuration register
 * internally in the class.
 *
 * @return config register byte
 */
uint8_t MAX31865_RTD::config_register()
{
  //  
  this->measured_status = readRegister8(MAX31865_CONFIG_REG);
  return this->measured_status;
  //
  
  /*
  uint8_t config_register ;
  // Start the read operation. 
  digitalWrite( this->cs_pin, LOW );
  // Tell the MAX31865 that we want to read, starting at register 0. 
  SPI.transfer( 0x00 );
  config_register = this->measured_status = SPI.transfer( 0x00 );
  digitalWrite( this->cs_pin, HIGH );
  return( config_register );
  */
}




