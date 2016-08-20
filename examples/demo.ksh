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
#  File		: demo.ksh
#  Purpose	: demo application for GSOSD.
#                 https://www.youtube.com/watch?v=zLIC-bxLw2U
#  Author(s)	: Martin7182
#  Creation	: 2016/06/22
#
#=========================================================================

throttle () {
    if test "$1" = ""; then delay="0.1"; else delay="$1"; fi;
    while read -N 1 ch; do printf "$ch"; sleep $delay; done
}

throttle0 () {
    throttle $1;
    printf "\0";
}

OUT=/dev/ttyUSB0
FAST=0.05
FASTER=0.025
SUBMAX=0.005
MAX=0.002
WIDTH=30
HEIGHT=13

# set serial speed
stty -F $OUT 57600

# set max refresh rate and silence to avoid buffer overflows
printf ' SET_REFRESH 0 ' >$OUT
printf ' SET_SILENT 1 ' >$OUT
printf ' SET_TIMEOUT 500 ' >$OUT
printf ' CLEAR ' >$OUT

# Show logo
printf " P_WINDOW 9 0 12 $HEIGHT -1 " >$OUT
for i in {1..$HEIGHT}; do
    printf "            " >$OUT;
done
printf "\\x80\\x81\\x82\\x83\\x84\\x85" >$OUT
printf "\\x86\\x87\\x88\\x89\\x8a\\x8b" >$OUT
printf "\\x90\\x91\\x92\\x93\\x94\\x95" >$OUT
printf "\\x96\\x97\\x98\\x99\\x9a\\x9b" >$OUT
for i in {1..6}; do
    printf "            " | throttle $SUBMAX >$OUT;
done
sleep 1

for phase in {1..2}; do
    y=0
    while (( y < $HEIGHT )); do
	if (( y < 6 )); then
	    ch="\\x03"
	else
	    ch="\\x04"
	fi
	if [[ $phase = 2 ]]; then
	    ch=" "
	fi
	if [[ $y = 5 ]]; then
	    printf "P_RAW 0 $y 9 " >$OUT;
	    for x in {0..8}; do
		printf "$ch" >$OUT;
	    done
	    printf "P_RAW 21 $y 9 " >$OUT;
	    for x in {21..29}; do
		printf "$ch" >$OUT;
	    done
	    sleep 0.03
	elif [[ $y != 6 ]]; then
	    printf "P_RAW 0 $y $WIDTH " >$OUT;
	    for x in {1..$WIDTH}; do
		printf "$ch" >$OUT;
	    done
	    sleep 0.03
	fi
	let y++
    done
done

for phase in {1..2}; do
    if [[ $phase = 1 ]]; then
	ch1="\\x01"
	ch2="\\x02"
	ch3="\\x03"
	ch4="\\x04"
    else
	ch1=" "
	ch2=" "
	ch3=" "
	ch4=" "
    fi
    for iter in {1..5}; do
	if [[ $phase = 1 ]]; then
	    line=$iter
	else
	    line=$((6-$iter))
	fi
	height=$((2*(5-$line)+2))
	x1=$((8-(5-$line)))
	x2=$((21+(5-$line)))
	x3=$(($x1+1))
	y0=$(($line+$height))
	y1=$(($line))
	y2=$(($y1-1))
	length=$((12+2*(5-$line)))
	while (( y1 < $y0 )); do
	    printf "P_RAW $x1 $y1 1 $ch1" >$OUT
	    printf "P_RAW $x2 $y1 1 $ch2" >$OUT
	    let y1++
	done
	printf "P_RAW $x3 $y2 $length " >$OUT
	i=0
	while (( i < $length )); do
	    printf "$ch3" >$OUT
	    let i++
	done
	printf "P_RAW $x3 $y0 $length " >$OUT
	i=0
	while (( i < $length )); do
	    printf "$ch4" >$OUT
	    let i++
	done
	sleep 0.03
    done
done

sleep 1
printf " P_WINDOW 9 0 12 $HEIGHT -1 " >$OUT
for i in {1..7}; do
    printf "            " | throttle $SUBMAX >$OUT
done
printf "\0" >$OUT
printf ' CLEAR ' >$OUT

printf ' P_RAW 1 3 29 Next line is printed with:   ' >$OUT
printf ' P_RAW 1 5 27 ' >$OUT
printf 'P_RAW 1 5 27 Yada Yada Yada' | throttle >$OUT
sleep 2
printf ' CLEAR ' >$OUT
printf ' P_RAW 1 3 29 Multiple lines 12x3 viewport: ' >$OUT
printf ' P_WINDOW 9 6 12 3 -1 ' >$OUT
printf 'P_WINDOW 9 6  12  3 -1 Yada Yada Yada...scrolls per line off the \
screen at the top' | throttle >$OUT
sleep 2
printf ' CLEAR ' >$OUT
printf ' P_RAW 1 3 29 Same with borders:           ' >$OUT
printf ' P_WINDOW 9 6 -12 -3 -1 ' >$OUT
printf 'P_WINDOW 9 6 -12 -3 -1 Yada Yada Yada...scrolls per line off the \
screen at the top' | throttle >$OUT
sleep 2
for i in {1..30}; do
    let "val = $i"
    printf " P_RAW 3 10 -1 HOS +1 (horz offset $val) \0" >$OUT
    printf ' HOS +1 ' >$OUT
    sleep 0.05
done
for i in {1..30}; do
    let "val = 30 - $i"
    printf " P_RAW 3 10 -1 HOS -1 (horz offset $val) \0" >$OUT
    printf ' HOS -1 ' >$OUT
    sleep 0.05
done
for i in {1..15}; do
    let "val = $i"
    printf " P_RAW 3 10 -1 VOS +1 (vert offset $val)   \0" >$OUT
    printf ' VOS +1 ' >$OUT
    sleep 0.05
done
for i in {1..15}; do
    let "val = 15 - $i"
    printf " P_RAW 3 10 -1 VOS -1 (vert offset $val)   \0" >$OUT
    printf ' VOS -1 ' >$OUT
    sleep 0.05
done
sleep 1
printf " P_RAW 3 10 -1 INSMUX +5 (min sharpness)\0" >$OUT
printf ' INSMUX1 +5 ' >$OUT
printf ' INSMUX2 +5 ' >$OUT
sleep 1
printf " P_RAW 3 10 -1 INSMUX -5 (max sharpness)\0" >$OUT
printf ' INSMUX1 -5 ' >$OUT
printf ' INSMUX2 -5 ' >$OUT
sleep 1
printf " P_RAW 3 10 -1 INSMUX +3 (restore)             \0" >$OUT
printf ' INSMUX1 +3 ' >$OUT
printf ' INSMUX2 +3 ' >$OUT
sleep 1
printf " P_RAW 3 10 -1 CBL -1 -3 (black level 0\%)  \0" >$OUT
printf ' CBL -1 -3 ' >$OUT
sleep 0.5
for i in {1..3}; do
    let "val = 10 * $i"
    printf " P_RAW 3 10 -1 CBL -1 +1 (black level $val\%)  \0" >$OUT
    printf ' CBL -1 +1 ' >$OUT
    sleep 1
done
printf " P_RAW 3 10 -1 CBL -1 -3 (restore)                \0" >$OUT
printf ' CBL -1 -3 ' >$OUT
sleep 1
printf " P_RAW 3 10 -1 CWL -1 -3 (white level 120\%)  \0" >$OUT
printf ' CWL -1 -3 ' >$OUT
sleep 1
for i in {1..3}; do
    let "val = 120 - 10 * $i"
    printf " P_RAW 3 10 -1 CWL -1 +1 (white level $val\%)  \0" >$OUT
    printf ' CWL -1 +1 ' >$OUT
    sleep 1
done
printf " P_RAW 3 10 -1 CWL -1 -2 (restore)                \0" >$OUT
printf ' CWL -1 -2 ' >$OUT
sleep 1
printf " P_RAW 3 10 -1 Turn OSD off:                      \0" >$OUT
printf " P_RAW 3 11 -1 SET_ENABLE 0   \0" | throttle0 >$OUT
printf " SET_ENABLE 0 " >$OUT
sleep 1
printf " P_RAW 3 10 -1 Turn OSD on again:                 \0" >$OUT
printf " P_RAW 3 11 -1 SET_ENABLE 1   \0" >$OUT
printf " SET_ENABLE 1 " >$OUT
sleep 1

wget -O- http://www.nasdaq.com/quotes/nasdaq-100-stocks.aspx?render=download \
2>/dev/null | sort | awk -F ',' '{ print $2 $3 $4 " " $5 "%" "     " }' \
>/tmp/nasdaq.out

printf ' CLEAR ' >$OUT
printf " P_WINDOW 1 3 29 4 99 \
For stock market addicts,\r\
the Nasdaq 100. Always keep\r\
an eye out for bargains,\r\
even when flying :-)" >$OUT

# Show banner
let "y = $HEIGHT - 2"
printf " P_BANNER 0 $y $WIDTH -2 60 " >$OUT
printf '                              ' | throttle $MAX >$OUT
printf '                              ' | throttle $MAX >$OUT
printf " P_BANNER 0 $y $WIDTH -2 150 " >$OUT
cat /tmp/nasdaq.out | head -c 150 | throttle >$OUT

printf ' CLEARPART 1 3 29 4 ' >$OUT
printf " P_RAW 12 5 7 Faster!" >$OUT
printf " P_BANNER 0 $y $WIDTH -2 150 " >$OUT
cat /tmp/nasdaq.out | tail -c +151 | head -c 150 |
throttle $FAST >$OUT

printf " P_RAW 12 5 10 Go faster!" >$OUT
printf " P_BANNER 0 $y $WIDTH -2 300 " >$OUT
cat /tmp/nasdaq.out | tail -c +301 | head -c 300 |
throttle $FASTER >$OUT

printf " P_RAW 12 5 11 Full beans!" >$OUT
printf " P_BANNER 0 $y $WIDTH -2 -1 " >$OUT
cat /tmp/nasdaq.out | tail -c +601 | throttle $MAX >$OUT
sleep 1

# Restore settings
printf ' SET_SILENT 0 ' >$OUT
printf ' SET_TIMEOUT 5000 ' >$OUT
printf ' CLEARPART 1 3 29 4 ' >$OUT
printf " P_RAW 5 5 20 Thanks for watching!" >$OUT
printf " P_BANNER 0 $y $WIDTH -2 -1 " >$OUT
printf "                              For more info: \
https://github.com/Martin7182/GSOSD" | throttle >$OUT
printf "\0" >$OUT
sleep 2
printf " CLEARPART 5 5 $WIDTH 3 " >$OUT

let "rangex = $WIDTH - 10"
let "rangey = $HEIGHT - 3"
sleeptime=1.0
for i in {1..100}; do
    x=$RANDOM
    let "x %= $rangex"
    y=$RANDOM
    let "y %= $rangey"
    printf " P_RAW $x $y 10 GAME OVER!" >$OUT
    let "sleeptime /= 1.1"
    sleep $sleeptime
    printf " CLEARPART $x $y 10 1 " | throttle $MAX >$OUT
done

printf " CLEAR " >$OUT
printf ' SET_REFRESH 100 ' >$OUT
