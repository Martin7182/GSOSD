/*========================================================================
 *  Copyright (c) 2016 Martin7182
 *
 *  This file is part of GSOSD.
 *
 *  GSOSD is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GSOSD is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GSOSD.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  File	: sensor.cpp
 *  Purpose	: Micro MinimOSD sensor functionality.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/06/16
 *
 *========================================================================
 */

/* workaround for stdint.h */
#define __STDC_LIMIT_MACROS

#include <Arduino.h>
#include <stdint.h>
#include "hardware.h"
#include "config.h"
#include "sensor.h"

//Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

/*------------------------------------------------------------------------
 *  Function	: sensor_setup
 *  Purpose	: Setup the sensor board.
 *  Method	: Set sensor pin modes.
 *
 *  The Micro MinimOSD board with KV-mod uses a 1k/14k6 resistor divider at
 *  pins VBAT1/VBAT2 so that a full scale measurement will be a bit more than
 *  17V based on a 1.1V reference voltage. So this suits up to 4s lipos
 *  perfectly. Therefore the analogue reference should be set to INTERNAL
 *  (1.1V) and not to DEFAULT (5V).
 *  The RSSI/CURR pins have no resistor divider so their max. voltage will be
 *  1.1V. Unfortunately we can only choose one analogue reference voltage. If
 *  you need to measure higher voltages than 1.1V, put your own resistor
 *  divider in front.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
sensor_setup(
    bool 	init)	/* whether called by initial setup */
{
    pinMode(ATMEL_SENS0PIN, INPUT);
    pinMode(ATMEL_SENS1PIN, INPUT);
    pinMode(ATMEL_SENS2PIN, INPUT);
    pinMode(ATMEL_SENS3PIN, INPUT);
    analogReference(INTERNAL);	/* internal 1.1V */

    /*
     * The first few measurements may be inaccurate after changing the
     * reference voltage. So we do them here.
     */
    sensor_get_value(SENSOR0);
    sensor_get_value(SENSOR1);
    sensor_get_value(SENSOR2);
    sensor_get_value(SENSOR3);
} /* sensor_setup() */


/*------------------------------------------------------------------------
 *  Function	: sensor_get_value
 *  Purpose	: Get voltage of requested sensor.
 *  Method	: Use inbuilt resistor voltage divider and ATmega328 ADC set
 *  		  to 1.1V reference voltage.
 *
 *  Returns	: The measured voltage in volts.
 *------------------------------------------------------------------------
 */
float
sensor_get_value(
    sensor_t 	sensor)	/* sensor id */
{
    uint8_t 	pin;	/* hardware sensor pin */
    uint16_t	sensadj;/* calibrated value */
    float	factor;	/* multiplication factor */

    switch (sensor) {
	case SENSOR0:
	    pin = ATMEL_SENS0PIN;
	    sensadj = cfg_get_sensadj0();
	    factor = 0.001;
	    break;
	case SENSOR1:
	    pin = ATMEL_SENS1PIN;
	    sensadj = cfg_get_sensadj1();
	    factor = 0.001;
	    break;
	case SENSOR2:
	    pin = ATMEL_SENS2PIN;
	    sensadj = cfg_get_sensadj2();
	    factor = 0.0001;
	    break;
	case SENSOR3:
	    pin = ATMEL_SENS3PIN;
	    sensadj = cfg_get_sensadj3();
	    factor = 0.0001;
	    break;
	default: return -1;
    }
    return factor * (float)sensadj * analogRead(pin) / (float)1023;
}  /* sensor_get_value() */
