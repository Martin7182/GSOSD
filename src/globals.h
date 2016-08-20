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
 *  File	: globals.h
 *  Purpose	: Global defines.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/08/01
 *
 *========================================================================
 */

#ifndef GLOBALS_H
#define GLOBALS_H

/* transmission flow control characters */
#define CONTROL_SOH (0x01)	/* start of header */
#define CONTROL_STX (0x02)	/* start of text */
#define CONTROL_ETX (0x03)	/* end of text */
#define CONTROL_EOT (0x04)	/* end of transmission */
#define CONTROL_ACK (0x06)	/* acknowledge */
#define CONTROL_DC1 (0x11)	/* XON; resume transmission */
#define CONTROL_DC3 (0x13)	/* XOFF; pause transmission */
#define CONTROL_NAK (0x15)	/* negative acknowledge */
#define CONTROL_CAN (0x18)	/* cancel */

#endif /* GLOBALS_H */

