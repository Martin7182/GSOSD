#!/usr/bin/ksh
#=========================================================================
#  Copyright (c) 2016 Martin7182
#
#  This file is part of GSOSD.
#
#  GSOSD is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  GSOSD is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GSOSD.  If not, see <http://www.gnu.org/licenses/>.
#
#  File		: examples.ksh
#  Purpose	: example code for GSOSD.
#  Author(s)	: Martin7182
#  Creation	: 2016/08/16
#
#=========================================================================

OUT=/dev/ttyUSB0

# Show the logo.
printf " CLEAR " >$OUT;
printf " P_WINDOW 9 5 12 2 24 " >$OUT;
printf "\\x80\\x81\\x82\\x83\\x84\\x85" >$OUT
printf "\\x86\\x87\\x88\\x89\\x8a\\x8b" >$OUT
printf "\\x90\\x91\\x92\\x93\\x94\\x95" >$OUT
printf "\\x96\\x97\\x98\\x99\\x9a\\x9b" >$OUT
sleep 5;
printf " CLEARPART 9 5 12 2 " >$OUT;


# Extract all font characters.
printf " SET_CONTROL 2 " >$OUT;
for i in {0..255}; do
    printf " GET_FONT $i " >$OUT;
    sleep 1;
done
printf " SET_CONTROL 3 " >$OUT;
