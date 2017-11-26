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
#  File		: gif2osd.php
#  Purpose	: Convert 192x288 pixel gif image to osd upload font file.
#  Author(s)	: Martin7182
#  Creation	: 2016/05/20
#
#=========================================================================

if ($argc < 3) {
    fprintf(STDERR, "Expecting gif image file as first argument and format as second argument; 1=MWOSD, 2=GSOSD-serial, 3=GSOSD-hardcoded.\n");
    exit(1);
}
// format 1: MWOSD format
// format 2: GSOSD format for serial input
// format 3: GSOSD format for hard-coded development
$format = $argv[2];

$gifimg = $argv[1];
$im = @imagecreatefromgif($gifimg);
if (!$im) {
    fprintf(STDERR, "Loading %s failed.\n", $gifimg);
    exit(1);
} 

if (imagesx($im) != 192) {
    fprintf(STDERR, "Wrong image size; expected %d horizontal pixels.\n", 192);
    exit(1);
}
if (imagesy($im) != 288) {
    fprintf(STDERR, "Wrong image size; expected %d vertical pixels.\n", 288);
    exit(1);
}
$format == 1 && printf("MAX7456\n");

$pc = 0; 	// no pixels stored yet
$byte = 0x00;	// output byte
$bi = 0;	// bit index in $byte
$bytec = 0;	// bytes written per character

// The NVM contains 256 rows of 64 bytes i.e. 512 bits per character; 432 data
// bits followed by 80 unused bits. The order of pixels is from top to bottom
// and from left to right. See the manual for more info.

// Walk 16 vertical characters.
for ($cy = 0; $cy < 16; $cy++) {

    // Walk 16 horizontal characters.
    for ($cx = 0; $cx < 16; $cx++) {

	if ($format == 2) {
	    printf("SET_FONT %d %d", $cy * 16 + $cx, 54 * 8 + 53);
	}
        // Walk 18 vertical character pixels.
	for ($cpy = 0; $cpy < 18; $cpy++) {

            // Walk 12 horizontal character pixels.
	    for ($cpx = 0; $cpx < 12; $cpx++) {

		$ipy = $cy * 18 + $cpy; // vertical pixel position
		$ipx = $cx * 12 + $cpx; // horizontal pixel position

		// See manual:
		// "00" : black opaque
		// "10" : white opaque
		// "x1" : transparent
		if (imagecolorat($im, $ipx, $ipy) == 0) {
		    // set white pixel
		    $byte |= (1 << (7 - $bi));
		} else {
		    if ($format == 1 || $format == 2) {
			// Bad runtime performance doesn't really matter.
			// Remembering the last value would be faster.
			if (($cpx == 0
				|| imagecolorat($im, $ipx - 1, $ipy) != 0)
			    && ($cpx == 11
				|| imagecolorat($im, $ipx + 1, $ipy) != 0)
			    && ($cpy == 0
				|| imagecolorat($im, $ipx, $ipy - 1) != 0)
			    && ($cpy == 17
				|| imagecolorat($im, $ipx, $ipy + 1) != 0)) {
			    // set transparent pixel at minimal distance 1 from
			    // character pixels, all pixels directly next to
			    // character pixels remain black.
			    $byte |= (1 << (6 - $bi));
			}
		    }
		}
		$pc++;
		if ($format == 3) {
		    $bi++;
		} else {
		    $bi += 2;
		}
		if ($bi == 8) {
		    $bytec++;
		    if ($format == 1) {
			printf("%08d\n", decbin($byte));
		    } else if ($format == 2) {
			printf(" %08d", decbin($byte));
		    } else if ($format == 3) {
			if ($bytec == 1) {
			    printf("{ ");
			} else if ($bytec == 15) {
			    printf("  ");
			}
			printf("0x%02X", $byte);
			if ($bytec != 27) {
			    printf(",");
			} else {
			    printf(" },\n");
			}
			if ($bytec == 14) {
			    printf("\n");
			}
		    }
		    $byte = 0x00;
		    $bi = 0;
		}

		// Fill the gap between byte 53 and 64.
		// Just fill with "01" pattern for convenience.
		if ($format == 1 && $pc % 216 == 0) {
		    $byte = 0x55;
		    for ($i = 0; $i < 10; $i++) {
		        printf("%08d\n", decbin($byte));
		    }
		}
	    }
	}
	if ($format == 2) {
	    printf("\n");
	}
	$bytec = 0;
    }
}

?>
