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
 *  File	: request.cpp
 *  Purpose	: Request handling routines.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/05/27
 *
 *========================================================================
 */

/* workaround for stdint.h */
#define __STDC_LIMIT_MACROS

#include <Arduino.h>
#include <stdint.h>
#include "command.h"
#include "config.h"
#include "globals.h"
#include "request.h"

//Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#define ACK(msg)				\
{						\
    if (!silent) {				\
	if (control2) {				\
	    Serial.println();			\
	    Serial.print(F((msg)));		\
	}					\
	if (control1) {				\
	    Serial.write((byte)CONTROL_ACK);	\
	}					\
    }						\
}

#define NAK(msg)				\
{						\
    if (!silent) {				\
	if (control2) {				\
	    Serial.println();			\
	    Serial.print(F((msg)));		\
	}					\
	if (control1) {				\
	    Serial.write((byte)CONTROL_NAK);	\
	}					\
    }						\
}

#define INIT() 					\
{						\
    parse = PARSE_INIT;				\
    if (!silent) {				\
	if (control2) {				\
	    Serial.println();			\
	    if (cfg_get_debug()) {		\
		Serial.print(freemem());	\
		Serial.print(" ");		\
	    }					\
	    Serial.print(F("GSOSD>"));		\
	}					\
	if (control1) {				\
	    Serial.write((byte)CONTROL_EOT);	\
	    sent_soh = false;			\
	}					\
    }						\
}

#define BUFSIZE 	(15)	/* buffer size in bytes */
#define MSG_E_INTERNAL	"<INTERNAL ERROR>"
#define MSG_E_TIMEOUT	"<TIMEOUT>"
#define MSG_E_FAIL	"<FAIL>"
#define MSG_E_OVERFLOW	"<OVERFLOW>"
#define MSG_E_SYNTAX	"<SYNTAX ERROR>"
#define MSG_E_ARG	"<INVALID ARG>"
#define MSG_OK		"<OK>"
#define NARGS 		(4)	/* max. number of integer command arguments */

typedef enum {	/* parser state */
    PARSE_INIT,	/* initialising, this is the default */
    PARSE_CMD,	/* parsing command string */
    PARSE_ARG,	/* parsing argument string */
    PARSE_LEN,	/* parsing data length */
    PARSE_DATA	/* parsing data */
} parse_t;


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


/*------------------------------------------------------------------------
 *  Function	: request
 *  Purpose	: Handle serial request if any.
 *  Method	: Parse request over multiple calls and handle data.
 *
 *  This function is non-blocking and returns control to the caller when
 *  there's no serial data on input. This may be in the middle of a request.
 *
 *  Request specification (BNF):
 *
 *  <syntax>   		::= <initial> <request> | <request>
 *  <request>  		::= <datarequest> | <nondatarequest>
 *  <datarequest>  	::= <command> <space> <len> <space> <data>
 *  <nondatarequest>  	::= <command>
 *  <initial>  		::= one or more non-alphanumeric characters
 *  <command>  		::= one or more alphanumeric characters
 *  <space>    		::= " "
 *  <len>      		::= one or more digits
 *  <data>     		::= one or more arbitrary characters
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
request(void)
{
    static char 		buf[BUFSIZE];	/* request buffer */
    static uint8_t 		i;		/* buf index */
    static uint16_t 		di;		/* data index */
    static parse_t		parse;		/* current parser state */
    static int32_t		len_pending;	/* num of data bytes pending */
    static cmdprops_p		cmdprops;	/* command properties */
    static uint8_t		argi;		/* command argument index */
    static int32_t		args[NARGS];	/* command arguments */
    static unsigned long 	time;		/* current time */
    int 			ch;		/* char received from serial */
    long long			intval;		/* big integer */
    int 			j;		/* loop counter */
    int 			avail;		/* serial available bytes */
    bool			silent;		/* whether to be silent */
    bool			echo;		/* whether to echo input */
    bool			control1;	/* whether to send control characters */
    bool			control2;	/* whether to send control text */
    static bool			sent_pause;	/* whether to requested transmission pause */
    static bool			sent_soh;	/* whether sent SOH char */

    silent = cfg_get_silent();
    echo = !silent && cfg_get_echo();
    control1 = !silent && cfg_get_control() & 0x01;
    control2 = !silent && cfg_get_control() & 0x02;
    if (time != 0 && millis() - time > cfg_get_timeout()) {
	time = 0;
	i = 0;
	NAK(MSG_E_TIMEOUT);
	INIT();
    }
    while ((avail = Serial.available()) > 0) {
	if (control1) {
	    if (avail > 50) {
	        if (avail >= 63) {
		    Serial.write((byte)CONTROL_CAN);
	        } else {
		    if (!sent_pause) {
		        Serial.write((byte)CONTROL_DC3);
		        sent_pause = true;
		    }
	        }
	    }
	    if (parse == PARSE_INIT && i == 0 && !sent_soh) {
		Serial.write((byte)CONTROL_SOH);
		sent_soh = true;
	    }
	}
	ch  = Serial.read();

	/*
	 * Echo character back to caller, non-printable data in hex format.
	 * Decrement buffer index on backspace for non-data character.
	 */
	if (echo) {
	    if (isprint(ch)) {
		Serial.write(ch);
	    } else {
		if (ch == 0x7F && parse != PARSE_DATA) {
		    /* backspace/del character */
		    if (i > 0) {
			i--;
			Serial.write(8);
		    }
		} else {
		    if (ch == 0x0D && parse != PARSE_DATA) {
			Serial.println();
		    } else {
			Serial.print(F("\\0x"));
			Serial.print(ch, HEX);
		    }
		}
	    }
	}

	/* handle buffer overflow */
	if (parse != PARSE_DATA && i >= sizeof(buf)) {
	    i = 0;
	    NAK(MSG_E_OVERFLOW);
	    INIT();
	    continue;
	}

	/* handle current character */
	switch (parse) {
	case PARSE_INIT:
	    if (isalnum(ch) || ispunct(ch)) {
	        buf[i++] = (char)ch;
	        parse = PARSE_CMD;
	    } else {
	        /* ignore any other character */
	    }
	    break;
	case PARSE_CMD:
	    if (isalnum(ch) || ispunct(ch)) {
	        buf[i++] = (char)ch;
	    } else if (isspace(ch)) {
	        buf[i] = '\0';
		for (j = 0; j < COMMAND_COUNT; j++) {
		    if (strcmp_P(
			buf, (char *)pgm_read_word(&(CMD_NAMES[j]))) == 0) {
			break;
		    }
		}
		if (j >= COMMAND_COUNT) {
		    i = 0;
		    NAK(MSG_E_SYNTAX);
		    INIT();
		    break;
		}
		cmdprops = &CMD_PROPS[j];
		if (cmdprops->argc > NARGS) {
		    i = 0;
		    NAK(MSG_E_INTERNAL);
		    INIT();
		    break;
		}
		argi = 0;
		for (j = 0; j < NARGS; j++) {
		    args[j] = 0;
		}
		if (cmdprops->argc != 0) {
		    parse = PARSE_ARG;
		} else if (cmdprops->use_data) {
		    parse = PARSE_LEN;
		} else {
		    if (!cmdprops->command(NULL)) {
			NAK(MSG_E_FAIL);
			/* fallthrough */
		    } else ACK(MSG_OK);
		    INIT();
		}
		i = 0;
	    } else {
	        /* ignore any other character */
	    }
	    break;
	case PARSE_ARG:
	    if (isalnum(ch) || ispunct(ch)) {
	        buf[i++] = (char)ch;
	    } else if (isspace(ch)) {
	        buf[i] = '\0';
		intval = atol(buf);
		j = i;
		if (intval == 0) {
		    for (j = 0; j < i; j++) {
			if (buf[j] != '0') {
			    break;
			}
		    }
		}
		if (j < i || intval > INT32_MAX || intval < INT32_MIN) {
		    NAK(MSG_E_ARG);
		    i = 0;
		    INIT();
		    break;
		}
		args[argi++] = intval;
		i = 0;
		if (argi < cmdprops->argc) {
		    parse = PARSE_ARG;
		} else {
		    if (cmdprops->use_data) {
		        parse = PARSE_LEN;
		    } else {
			if (!cmdprops->command(args)) {
			    NAK(MSG_E_FAIL);
			    /* fallthrough */
		    	} else ACK(MSG_OK);
			INIT();
		    }
		}
	    } else {
	        /* ignore any other character */
	    }
	    break;
	case PARSE_LEN:
	    if (isalnum(ch) || ispunct(ch)) {
	        buf[i++] = (char)ch;
	    } else if (isspace(ch)) {
	        buf[i] = '\0';
		len_pending = atol(buf);
		i = 0;
		if (len_pending != 0) {	/* negative == infinite length */
	            time = millis();
		    di = 0;
		    parse = PARSE_DATA;
		} else {
	            INIT();
		}
	    } else {
	        /* ignore any other character */
	    }
	    break;
	case PARSE_DATA:
	    time = millis();
	    if (ch == '\0') {
		len_pending = 0;
	    } else {
		buf[i] = (char)ch;
		i++;
	    }
            if (i == sizeof(buf)) {
	        if (!cmdprops->command(args, di, sizeof(buf), buf)) {
		    NAK(MSG_E_FAIL);
		    time = 0;
		    i = 0;
	            INIT();
		    break;
		}
		di += sizeof(buf);
		i = 0;
	    }
	    if (len_pending >= 0 && --len_pending <= 0) {
	        if (i != 0) {
		    /* flush buffer */
		    if (!cmdprops->command(args, di, i, buf)) {
		        NAK(MSG_E_FAIL);
			time = 0;
			i = 0;
			INIT();
			break;
		    }
		    i = 0;
		}
		ACK(MSG_OK);
		time = 0;
	        INIT();
	    }
	    break;
	default:
	    i = 0;
	    NAK(MSG_E_INTERNAL);
	    INIT();
	    break;
	}
    }

    /* flush buffer */
    if (parse == PARSE_DATA && i != 0) {
	if (!cmdprops->command(args, di, i, buf)) {
	    NAK(MSG_E_FAIL);
	    time = 0;
	    INIT();
	    /* fallthrough */
	}
	di += i;
	i = 0;
    }

    /* input buffer should be empty */
    if (control1 && sent_pause) {
	Serial.write((byte)CONTROL_DC1);
	sent_pause = false;
    }
} /* request() */
