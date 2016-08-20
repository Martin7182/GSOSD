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
 *  File	: GSOSD.ino
 *  Purpose	: Main routines.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/05/24
 *
 *========================================================================
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include "request.h"
#include "max7456.h"
#include "config.h"
#include "sensor.h"
#include "command.h"
#include "globals.h"

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif


/*------------------------------------------------------------------------
 *  Function	: show_logo
 *  Purpose	: Show logo at startup if defined.
 *  Method	: Print text window in the middle of the screen.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
show_logo(void)
{
    int32_t logo;	/* configured time in ms to show the logo */

    if ((logo = cfg_get_logo()) > 0) {
	int32_t args[4] = { 9, 5, 12, 2 };
	char buf[2];

	buf[0] = '\x80';
	buf[1] = '\0';

	for (uint8_t i = 0; i < 24; i++) {
	    cmd_p_window(args, i, 1, buf);
	    if (++(buf[0]) == '\x8c') buf[0] = '\x90';
	}
	delay(logo);
	cmd_clearpart(args);
    }
} /* show_logo() */


/*------------------------------------------------------------------------
 *  Function	: setup
 *  Purpose	: Initialise system.
 *  Method	: Setup serial connection SPI and max7456.
 *
 *  This function is automatically called once by the Arduino framework.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
setup(void)
{
    /*
     * Prevent non-recoverable error by a hard coded delay. When you
     * accidentally corrupt the program, being unable to overwrite with a new
     * sketch, there's no way to recover except installing a new bootloader
     * (and wiping the software). But this isn't easy on a Micro MinimOSD.
     *
     * I made this error once and had the luck that show_logo() was still ok
     * which gave me two seconds (default delay) to start uploading a new
     * sketch before the corruption occurred.
     */
    delay(3000);
    SPI.begin();
    cfg_load_defaults();
    max_setup(true);
    sensor_setup(true);
    cfg_load(true);
    Serial.begin(cfg_get_baudrate());
    max_videodetect();
    show_logo();
    if (!cfg_get_silent()) {
        if (cfg_get_control() & 0x01) {
    	    Serial.write((byte)CONTROL_SOH);
	}
        if (cfg_get_control() & 0x02) {
	    Serial.print(F("Welcome to GS-OSD, version: "));
	    Serial.println(cfg_get_version());
	    if (cfg_get_debug()) {
		Serial.print(freemem());
		Serial.print(" ");
	    }
	    Serial.print(F("GSOSD>"));
	}
        if (cfg_get_control() & 0x01) {
    	    Serial.write((byte)CONTROL_EOT);
	}
    }
} /* setup() */


/*------------------------------------------------------------------------
 *  Function	: loop
 *  Purpose	: Mainloop of program.
 *  Method	: Handle requests.
 *		  Try to detect attached video signal.
 *  		  Restore image visibility if needed.
 *
 *  This function is automatically called repeatably by the Arduino framework.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
loop(void)
{
    static unsigned long 	oldtime1 = millis();	/* old timestamp */
    static unsigned long 	oldtime2 = millis();	/* old timestamp */
    unsigned long 		thistime = millis();	/* current timestamp */

    if (thistime - oldtime1 > cfg_get_vdetect()) {
        max_videodetect();
	oldtime1 = thistime;
    }

    /*
     * The OSD seems to reset the enable bit of reg VM0 now and then.
     * We restore it here.
     */
    if (thistime - oldtime2 > 100) {
        max_enable(cfg_get_enable());
	oldtime2 = thistime;
    }
    request();
} /* loop() */
