<?php
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
#  File		: battery.php
#  Purpose	: Show OSD battery voltage and remaining runtime.
#		  https://www.youtube.com/watch?v=WCpxvjb2Bmo
#  Author(s)	: Martin7182
#  Creation	: 2016/07/31
#
#=========================================================================

# interrupt flag
$stop = false;

#
# Lipo cell state of charge; voltage vs. percentage. This is the open voltage.
# Actual measured voltage will be a little lower, but will be corrected.
# Data is taken from HobbyKing HK-010 Wattmeter & Voltage Analyzer; http://www.hobbyking.com/hobbyking/store/__10786__HobbyKing_HK_010_Wattmeter_Voltage_Analyzer.html
#
$lipogauge = array(
    array(3.000, 0),
    array(3.431, 2),
    array(3.508, 4),
    array(3.573, 6),
    array(3.630, 8),
    array(3.671, 10),
    array(3.695, 12),
    array(3.705, 14),
    array(3.710, 16),
    array(3.719, 18),
    array(3.725, 20),
    array(3.731, 22),
    array(3.739, 24),
    array(3.744, 26),
    array(3.752, 28),
    array(3.759, 30),
    array(3.763, 32),
    array(3.770, 34),
    array(3.778, 36),
    array(3.788, 38),
    array(3.795, 40),
    array(3.800, 42),
    array(3.807, 44),
    array(3.817, 46),
    array(3.827, 48),
    array(3.837, 50),
    array(3.843, 52),
    array(3.847, 54),
    array(3.857, 56),
    array(3.865, 58),
    array(3.876, 60),
    array(3.887, 62),
    array(3.896, 64),
    array(3.905, 66),
    array(3.915, 68),
    array(3.925, 70),
    array(3.935, 72),
    array(3.945, 74),
    array(3.964, 76),
    array(3.974, 78),
    array(3.984, 80),
    array(3.998, 82),
    array(4.013, 84),
    array(4.028, 86),
    array(4.042, 88),
    array(4.062, 90),
    array(4.086, 92),
    array(4.101, 94),
    array(4.135, 96),
    array(4.170, 98),
    array(4.200, 100),
);


#-------------------------------------------------------------------------
#  Function	: sendrecv
#  Purpose	: Send command, receive response.
#  Method	: Write/read to/from serial port given by handle.
#
#  Returns	: The data read between STX and ETX characters.
#-------------------------------------------------------------------------
function
sendrecv(
    $handle,
    $cmd)
{
    fputs($handle, $cmd);
    unset($buf);
    $soh = false;
    $data = '';
    while (($ch = fgetc($handle)) !== false) {
        print($ch);
	if ($ch == "\x01") $soh = true;		/* SOH */
	if (!$soh) continue;			/* skip to next response */
	if ($ch == "\x04") break;		/* EOT */
	if ($ch == "\x03") $data = $buf;	/* ETX */
	if (isset($buf)) $buf .= $ch;
	if ($ch == "\x02") $buf = '';		/* STX */
    }
    return $data;
} /* sendrecv() */


#-------------------------------------------------------------------------
#  Function	: initialise
#  Purpose	: Install interrupt handler.
#  Method	: pcntl_signal( .
#
#  Returns	: Nothing.
#-------------------------------------------------------------------------
function
initialise()
{
    declare(ticks = 1);

    pcntl_signal(SIGINT,  'handle_interrupt');
} /* initialise() */


#-------------------------------------------------------------------------
#  Function	: handle_interrupt
#  Purpose	: Handle user interrupt.
#  Method	: Set boolean flag.
#
#  Returns	: Nothing.
#-------------------------------------------------------------------------
function
handle_interrupt()
{
    global $stop;

    $stop = true;
} /* handle_interrupt() */


#-------------------------------------------------------------------------
#  Function	: application
#  Purpose	: Show OSD battery level.
#  Method	: Read GSOSD sensor 0, calculate remaining runtime,
#		  send results to GSOSD.
#
#  This application runs from the command line with the following optional
#  arguments:
#  -b <baudrate>		// default: 9600
#  -p <serial port>		// default: /dev/ttyUSB0
#  -s <voltag sag under load>	// default: 0
#
#  Returns	: Nothing.
#-------------------------------------------------------------------------
function
application()
{
    global $stop;
    global $voltage_sag;
    global $lipogauge;

    $options = getopt("b:p:s:");

    # Baud rate for serial connection. This value should match with the
    # configured GSOSD value or with the BT-module if applicable.
    if (array_key_exists('b', $options)) {
        $baudrate = $options['b'];
    } else {
        $baudrate = 9600;
    }

    # Serial port to which the (Micro) MinimOSD is connected.
    if (array_key_exists('p', $options)) {
        $port = $options['p'];
    } else {
        $port = '/dev/ttyUSB0';
    }

    # Voltage sag per cel. This highly depends on battery quality.
    # The measured cell voltage will be corrected with this value before
    # looking up the voltage in $lipogauge.
    if (array_key_exists('s', $options)) {
        $voltage_sag = $options['s'];
    } else {
        $voltage_sag = 0;
    }

    `stty -F $port $baudrate`;
    $handle = fopen($port, 'r+');
    if ($handle != NULL) {
	unset($ini_percentage);
	unset($ini_time);
	unset($numcells);
	$updatetime = 10;
	$x = 10;
	$y = 1;
	$maxcellvoltage = 4.5;
	$maxarglen = 0;
	$silent = sendrecv($handle, " GET_SILENT ");
	$control = sendrecv($handle, " GET_CONTROL ");
	sendrecv($handle, " SET_SILENT 0 ");
	sendrecv($handle, " SET_CONTROL 3 ");
	sendrecv($handle, " CLEAR ");
	while (!($stop)) {
	    $data = sendrecv($handle, " GET_SENSOR 0 ");
	    $voltage = floatval($data);
	    if (!isset($numcells)) {
		$numcells = 1 + floor($voltage / $maxcellvoltage);
	    }
	    $cellvoltage = $voltage / $numcells;
	    $cellvoltage += $voltage_sag;
	    list($min_voltage, $min_percentage) = current($lipogauge);
	    list($max_voltage, $max_percentage) = current($lipogauge);
	    foreach ($lipogauge as $tuple) {
		$g_cellvoltage = $tuple[0];
		$g_percentage = $tuple[1];
		if ($cellvoltage > $g_cellvoltage) {
		    $min_voltage = $max_voltage;
		    $min_percentage = $max_percentage;
		    $max_voltage = $g_cellvoltage;
		    $max_percentage = $g_percentage;
		}
	    }
	    if ($max_voltage > $min_voltage) {
		$percentage = (float)$min_percentage
			      + ((float)($max_percentage - $min_percentage)
				      * ($cellvoltage - $min_voltage))
				/ ($max_voltage - $min_voltage);
	    } else {
		$percentage = 0;
	    }
	    if (!isset($ini_percentage)) {
		$ini_percentage = $percentage;
		$oldpercentage = $percentage;
		$ini_time = time()/60.0;
		$oldtime = $ini_time;
		$remainingtime = -1;
		$rate = 0;
	    } else{
		$curtime = time()/60.0;
		if ($ini_percentage > $percentage) {
		    $difftime = $curtime - $ini_time;
		    $rate = ($ini_percentage - $percentage) / $difftime;
		    if ($percentage < $oldpercentage) {
			$oldpercentage = $percentage;
			$oldtime = $curtime;
			$correction = 0;
		    } else {
			$difftime = $curtime - $oldtime;
			$correction = $rate * $difftime;
		    }
		    printf("\n===\n", $rate);
		    printf("discharge rate: %.2f %% per minute\n", $rate);
		    printf("correction: %.2f %%\n", $correction);
		    printf("===\n", $rate);
		    $remainingtime = round(($percentage - $correction)/ $rate);
		}
	    }
	    if ($remainingtime < 0) {
		$timestr = "";
	    } else {
		if ($remainingtime >= 60) {
		    $timestr = sprintf("%dh%dm",
				       floor($remainingtime / 60),
				       $remainingtime % 60);
		} else {
		    $timestr = sprintf("%dm", $remainingtime);
		}
	    }
	    $arg = sprintf("%.2fV %d%% (%s)",
			   $voltage, round($percentage), $timestr);
	    $arglen = strlen($arg);
	    if ($arglen > $maxarglen) $maxarglen = $arglen;
	    $arg = str_pad($arg, $maxarglen);
	    $cmd = sprintf(" P_RAW %d %d %d %s", $x, $y, $maxarglen, $arg);
	    sendrecv($handle, $cmd);
	    sleep($updatetime);
	}
	sendrecv($handle, " CLEAR ");
	/* restore values (like already in eeprom) */
	sendrecv($handle, " SET_SILENT $silent ");
	sendrecv($handle, " SET_CONTROL $control ");
	fclose($handle);
    }
} /* application() */


initialise();
application();
?>
