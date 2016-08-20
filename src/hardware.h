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
 *  File	: hardware.h
 *  Purpose	: Hardware details for the Micro Minim OSD board.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/07/18
 *
 *========================================================================
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#define MAX_SELECTPIN	(6)		/* hardware select pin */
#define MAX_RESETPIN	(10)		/* hardware reset pin */
#define MAX_MOSIPIN	(11)		/* hardware mosi pin */
#define MAX_MISOPIN	(12)		/* hardware miso pin */
#define MAX_SCKPIN	(13)		/* hardware sck pin */
#define MAX_VSYNCPIN	(2)		/* hardware vsync pin */

#define ATMEL_SENS0PIN	(A2)		/* hardware voltage0 pin (BAT1) */
#define ATMEL_SENS1PIN	(A0)		/* hardware voltage1 pin (BAT2) */
#define ATMEL_SENS2PIN	(A3)		/* hardware voltage2 pin (RSSI) */
#define ATMEL_SENS3PIN	(A1)		/* hardware voltage3 pin (CURR) */

#endif /* HARDWARE_H */

