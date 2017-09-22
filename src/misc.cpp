/*========================================================================
 *  Copyright (c) 2017 Martin7182
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
 *  File	: misc.cpp
 *  Purpose	: Miscellaneous routines.
 *  Author(s)	: Martin7182
 *  Creation	: 2017/03/30
 *
 *========================================================================
 */


/*------------------------------------------------------------------------
 *  Function	: freemem
 *  Purpose	: Return available heap memory in bytes.
 *  Method	: Use externals.
 *
 *  Returns	: Number in bytes.
 *------------------------------------------------------------------------
 */
int
freemem(void)
{
    extern int 	__heap_start;
    extern int 	*__brkval;
    int 	v;

    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
} /* freemem() */


