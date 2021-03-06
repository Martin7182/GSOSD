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
 *  File	: sensor.h
 *  Purpose	: Declarations for sensors on the Micro MinimOSD board.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/06/16
 *
 *========================================================================
 */

#ifndef SENSOR_H
#define SENSOR_H

typedef enum sensor_t {		/* voltage sensor ids */
    SENSOR0 = 0,		/* BAT1 sensor */
    SENSOR1,			/* BAT2 sensor */
    SENSOR2,			/* RSSI sensor */
    SENSOR3,			/* CURR sensor */
    SENSOR_NONE,		/* dummy value, must be last element! */
} sensor_t;

#define SENSOR_COUNT SENSOR_NONE

void sensor_setup(bool);

float sensor_get_value(sensor_t);

bool sensor_set_value(sensor_t, float);

int16_t sensor_get_raw(sensor_t);

#endif /* SENSOR_H */

