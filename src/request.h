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
 *  File	: request.h
 *  Purpose	: Declarations for request handling routines.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/05/27
 *
 *========================================================================
 */

#ifndef REQUEST_H
#define REQUEST_H

/*
 * serial_request() and sensor_request() are made public to avoid compiler
 * warning: ‘void xxx_request()’ defined but not used.
 * See request() for more info.
 */
void serial_request(void);
void sensor_request(void);
void request(void);

#endif /* REQUEST_H */

