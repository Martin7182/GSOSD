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
 *  This file contains two parts which for convenience are kept together.
 *  First part handles serial request and second part handles sensor requests.
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
#include "misc.h"
#include "sensor.h"
#include "max7456.h"
#include "font.h"
#include "request.h"

//Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif


/*------------------------------------------------------------------------
 *  Function	: request_init1
 *  Purpose	: Primary request initialisation.
 *  Method	: Handle primary serial or sensor request init.
 *
 *  To avoid compiler warnings, serial_request_init1() and
 *  sensor_request_init1() are made public instead of static.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
request_init1(void)
{
#ifdef STANDALONE
    sensor_request_init1();
#else
    serial_request_init1();
#endif
} /* request_init1() */


/*------------------------------------------------------------------------
 *  Function	: request_init2
 *  Purpose	: Secundary request initialisation.
 *  Method	: Handle secundary serial or sensor request init.
 *
 *  To avoid compiler warnings, serial_request_init2() and
 *  sensor_request_init2() are made public instead of static.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
request_init2(void)
{
#ifdef STANDALONE
    sensor_request_init2();
#else
    serial_request_init2();
#endif
} /* request_init2() */


/*------------------------------------------------------------------------
 *  Function	: request
 *  Purpose	: Handle input request if any.
 *  Method	: Handle serial or sensor request.
 *
 *  Beforehand you'll have to choose between the normal and standalone usage.
 *  The first only handles serial requests while the latter only handles sensor
 *  requests. This is done purely to save memory; enabling both would require
 *  more than the available memory. Standalone usage can be specified in the
 *  Makefile as follows:
 *
 *  CXXFLAGS_STD = -D STANDALONE
 *
 *  To avoid compiler warnings, serial_request() and sensor_request() are made
 *  public instead of static.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
request(void)
{
#ifdef STANDALONE
    sensor_request(false);
#else
    serial_request();
#endif
} /* request() */


#ifndef STANDALONE
/*------------------------------------------------------------------------
 * Serial request handling starts here.
 *------------------------------------------------------------------------
 */

#define INIT() \
    init_parse(silent, control_msg, control_chr, &parse, &sent_soh);
#define ACK(msg) \
    send_control((msg), (byte)CONTROL_ACK, silent, control_msg, control_chr);
#define NAK(msg) \
    send_control((msg), (byte)CONTROL_NAK, silent, control_msg, control_chr);

#ifndef NO_DEBUG
#define DEBUG_FREEMEM()				\
{						\
    if (cfg_get_debug()) {			\
	Serial.print(freemem());		\
	Serial.print(" ");			\
    }						\
}
#else
#define DEBUG_FREEMEM() {}
#endif

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
 *  Function	: init_parse
 *  Purpose	: Initialise parser state.
 *  Method	: Direct assingment of variables & write to serial.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
init_parse(
	bool 	silent,		/* whether to be silent */
    	bool	control_msg,	/* whether to send control message */
	bool 	control_chr,	/* whether to send control chars */
	parse_t *parse,		/* current parser state */
	bool 	*sent_soh)	/* whether to sent SOH char */
{
    *parse = PARSE_INIT;
    if (!silent) {
	if (control_msg) {
	    Serial.println();
	    DEBUG_FREEMEM();
	    Serial.print(F("GSOSD>"));
	}
	if (control_chr) {
	    Serial.write((byte)CONTROL_EOT);
	    *sent_soh = false;
	}
    }
} /* init_parse() */


/*------------------------------------------------------------------------
 *  Function	: serial_request_init1
 *  Purpose	: Primary serial request initialisation.
 *  Method	: None, this function is for symmetry only.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
serial_request_init1(void)
{
} /* serial_request_init1() */


/*------------------------------------------------------------------------
 *  Function	: serial_request_init2
 *  Purpose	: Secundary serial request initialisation.
 *  Method	: Write message and control characters to serial.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
serial_request_init2(void)
{
    bool	control_chr;	/* whether to send control characters */
    bool	control_msg;	/* whether to send control messages */
    parse_t 	p;		/* dummy variale */
    bool 	s;		/* dummy variable */

    if (!cfg_get_silent()) {
        if ((control_chr = ((cfg_get_control() & 0x01) != 0x00))) {
    	    Serial.write((byte)CONTROL_SOH);
	}
        if ((control_msg = ((cfg_get_control() & 0x02) != 0x00))) {
	    Serial.print(F("Welcome to GS-OSD, version: "));
	    Serial.println(cfg_get_version());
	}
        init_parse(false, control_msg, control_chr, &p, &s);
    }
} /* serial_request_init2() */


/*------------------------------------------------------------------------
 *  Function	: send_control
 *  Purpose	: Send control-  message and/or character to serial.
 *  Method	: Write to serial.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
send_control(
	const char 	*msg,		/* control msg to print to serial */
	byte		chr,		/* control char to print to serial */
	bool 		silent,		/* whether to be silent */
    	bool		control_msg,	/* whether to send control message */
	bool 		control_chr)	/* whether to send control chars */
{
    if (!silent) {
	if (control_msg) {
	    Serial.println();
	    Serial.print(msg);
	}
	if (control_chr) {
	    Serial.write(chr);
	}
    }
} /* send_control() */


/*------------------------------------------------------------------------
 *  Function	: serial_request
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
serial_request(void)
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
    uint8_t 			j;		/* loop counter */
    uint8_t 			avail;		/* serial available bytes */
    bool			silent;		/* whether to be silent */
    bool			echo;		/* whether to echo input */
    bool			control_chr;	/* whether to send control
						   characters */
    bool			control_msg;	/* whether to send control
						   messages */
    static bool			sent_pause;	/* whether to requested
						   transmission pause */
    static bool			sent_soh;	/* whether sent SOH char */

    silent = cfg_get_silent();
    echo = !silent && cfg_get_echo();
    control_chr = !silent && (cfg_get_control() & 0x01) != 0x00;
    control_msg = !silent && (cfg_get_control() & 0x02) != 0x00;
    if (time != 0 && millis() - time > cfg_get_timeout()) {
	time = 0;
	i = 0;
	NAK(MSG_E_TIMEOUT);
	INIT();
    }
    while ((avail = Serial.available()) > 0) {
	if (control_chr) {
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
    if (control_chr && sent_pause) {
	Serial.write((byte)CONTROL_DC1);
	sent_pause = false;
    }
} /* serial_request() */

#else

/*------------------------------------------------------------------------
 * Sensor request handling starts here.
 *------------------------------------------------------------------------
 */

/* A list of all menu items.      				*/
/* key			name					*/
#define MENUITEM_TABLE						\
X(MI_YES,		"Yes")					\
X(MI_NO,		"No")					\
X(MI_PLUS,		"+")					\
X(MI_MINUS,		"-")					\
X(MI_HORIZONTAL,	"Horizontal")				\
X(MI_VERTICAL,		"Vertical")				\
X(MI_VISIBLE,		"Visible")				\
X(MI_INVISIBLE,		"Invisible")				\
X(MI_SENSOR0,		"Sensor0")				\
X(MI_SENSOR1,		"Sensor1")				\
X(MI_SENSOR2,		"Sensor2")				\
X(MI_SENSOR3,		"Sensor3")				\
X(MI_RUNTIME,		"Runtime")				\
X(MI_ENABLE,		"Enable")				\
X(MI_HOS,            	"Horizontal offset")			\
X(MI_VOS,            	"Vertical offset")			\
X(MI_CBL,            	"Character black level")		\
X(MI_CWL,            	"Character white level")		\
X(MI_INSMUX1,        	"Sharpness 1")				\
X(MI_INSMUX2,        	"Sharpness 2")				\
X(MI_FONT,        	"Font")					\
X(MI_FONTEFFECT,       	"Font effect")				\
X(MI_BORDER,   		"Black border")				\
X(MI_SHADOW,   		"Shadow")				\
X(MI_TRANSWHITE,     	"Trans/white")				\
X(MI_BLACKWHITE,     	"Black/white")				\
X(MI_INVERT,     	"Invert")				\
X(MI_FONTRESET,        	"Font reset")				\
X(MI_LAYOUT,		"Layout")				\
X(MI_CALIBRATION,	"Calibration")				\
X(MI_SCREEN,		"Screen")				\
X(MI_ABOUT,		"About")				\
X(MI_EMPTY,		"")					\
X(MI_NONE,		"")					\

/* X-macros generating menu names. */
#define X(key, name) \
    const char s_ ## key[] PROGMEM = name;
    MENUITEM_TABLE
#undef X
const char * const MENU_NAMES[] PROGMEM = {
#define X(key, name) s_ ## key,
    MENUITEM_TABLE
#undef X
};

/* X-macro generating menu ids. Note that enums are 16 bits by default. */
typedef enum menuitem16_t {
#define X(key, name) key,
    MENUITEM_TABLE
#undef X
    MENUITEM_COUNT
} menuitem16_t;

typedef uint8_t menuitem_t;	/* use as short enum as possible */

/* Unique 32 bits flag for each menu item. */
#define FLAG(id) ((uint32_t)1 << (id))

typedef struct menu_t {		/* single menu element */
    menuitem_t 	id;		/* id referring to name in progmem */
    menu_t 	*children;	/* list of sub-menus if any */
    menu_t 	*parent;	/* parent menu */
    uint8_t 	ci;		/* index of current child */
} menu_t;

typedef struct menus_t {	/* all menus linked together */
    uint32_t 	act_flags;	/* shortcut to find active menus */
    menu_t	*active;	/* active menu */
    menu_t 	*root;		/* root menu */
    const char 	*msg;		/* message to print in middle of screen */
} menus_t;

typedef enum adjust_t {		/* adjust element types */
    ADJ_MENUCHANGE,		/* menu change */
    ADJ_CALIBRATION,		/* calibrate sensor */
    ADJ_SMALLSTEPS,		/* plus/minus small steps */
    ADJ_BIGSTEPS,		/* plus/minus big steps */
} adjust_t;


/*------------------------------------------------------------------------
 *  Function	: menu_get_name
 *  Purpose	: Get menu element name from progmem.
 *  Method	: Use progmem routines, thus saving precious heap space.
 *
 *  Returns	: The name found in a static buffer.
 *------------------------------------------------------------------------
 */
static const char *
menu_get_name(
    menuitem_t 	id)		/* id referring to name in progmem */
{
#define MAX_NAMELEN 25
    static char buf[MAX_NAMELEN];

    strlcpy_P(buf, (char *)pgm_read_word(&(MENU_NAMES[id])), sizeof(buf));
    return buf;
#undef MAX_NAMELEN
} /* menu_get_name() */


/*------------------------------------------------------------------------
 *  Function	: menus_init
 *  Purpose	: Initialise menus.
 *  Method	: Define static menus and link them together.
 *
 *  Returns	: Pointer to root menu.
 *------------------------------------------------------------------------
 */
static menus_t *
menus_init(void)
{
    static menu_t yesno[] = {
	{ MI_YES,	NULL, NULL, 0 },
	{ MI_NO, 	NULL, NULL, 0 },
	{ MI_NONE }
    };
    static menu_t plusminus[] = {
	{ MI_PLUS, 	NULL, NULL, 0 },
	{ MI_MINUS, 	NULL, NULL, 0 },
	{ MI_NONE }
    };
    static menu_t position[] = {
	{ MI_HORIZONTAL,&plusminus[0], 	NULL, 0 },
	{ MI_VERTICAL, 	&plusminus[0], 	NULL, 0 },
	{ MI_NONE }
    };
    static menu_t showelement[] = {
	{ MI_VISIBLE, 	&position[0], 	NULL, 0 },
	{ MI_INVISIBLE, NULL, 		NULL, 0 },
	{ MI_NONE }
    };

    /* Use actual elements' visibility as starting point. */
    static menu_t layout[] = {
	{ MI_SENSOR0, &showelement[0], 	NULL,
	    cfg_get_sensv0() ? (uint8_t)0 : (uint8_t)1 },
	{ MI_SENSOR1, &showelement[0], 	NULL,
	    cfg_get_sensv1() ? (uint8_t)0 : (uint8_t)1 },
	{ MI_SENSOR2, &showelement[0], 	NULL,
	    cfg_get_sensv2() ? (uint8_t)0 : (uint8_t)1 },
	{ MI_SENSOR3, &showelement[0], 	NULL,
	    cfg_get_sensv3() ? (uint8_t)0 : (uint8_t)1 },
	{ MI_RUNTIME, &showelement[0], 	NULL,
	    cfg_get_timev() ? (uint8_t)0 : (uint8_t)1 },
	{ MI_NONE }
    };
    static menu_t calibration[] = {
	{ MI_SENSOR0, 	&plusminus[0], 	NULL, 0 },
	{ MI_SENSOR1, 	&plusminus[0], 	NULL, 0 },
	{ MI_SENSOR2, 	&plusminus[0], 	NULL, 0 },
	{ MI_SENSOR3, 	&plusminus[0], 	NULL, 0 },
	{ MI_NONE }
    };
    static menu_t fonteffect[] = {
	{ MI_BORDER,	&yesno[0], 	NULL, 1 },
	{ MI_SHADOW,	&yesno[0], 	NULL, 1 },
	{ MI_TRANSWHITE,&yesno[0], 	NULL, 1 },
	{ MI_BLACKWHITE,&yesno[0], 	NULL, 1 },
	{ MI_INVERT,	&yesno[0], 	NULL, 1 },
	{ MI_NONE }
    };
    static menu_t font[] = {
	{ MI_FONTRESET,	&yesno[0], 	NULL, 1 },
	{ MI_FONTEFFECT,&fonteffect[0],	NULL, 0 },
	{ MI_NONE }
    };
    static menu_t screen[] = {
	{ MI_ENABLE, 	&yesno[0], 	NULL, 0 },
	{ MI_HOS, 	&plusminus[0], 	NULL, 0 },
	{ MI_VOS, 	&plusminus[0], 	NULL, 0 },
	{ MI_CBL, 	&plusminus[0], 	NULL, 0 },
	{ MI_CWL, 	&plusminus[0], 	NULL, 0 },
	{ MI_INSMUX1, 	&plusminus[0], 	NULL, 0 },
	{ MI_INSMUX2, 	&plusminus[0], 	NULL, 0 },
	{ MI_FONT,	&font[0], 	NULL, 0 },
	{ MI_NONE }
    };
    static menu_t mainmenu[] = {
	{ MI_LAYOUT, 		&layout[0], 	NULL, 0 },
	{ MI_CALIBRATION,	&calibration[0],NULL, 0 },
	{ MI_SCREEN, 		&screen[0], 	NULL, 0 },
	{ MI_ABOUT, 		NULL, 		NULL, 0 },
	{ MI_NONE }
    };
    static menu_t nomenu[] = {
	{ MI_EMPTY, 	&mainmenu[0], 	NULL, 0 },
	{ MI_NONE }
    };
    static menu_t root = { MI_NONE, &nomenu[0], NULL, 0 };
    static menus_t menus = {
	0,
	&root,
	&root,
	NULL
    };
    return &menus;
} /* menus_init() */


/*------------------------------------------------------------------------
 *  Function	: print_elements
 *  Purpose	: Print sensor voltages and runtime on screen.
 *  Method	: Convert data to string format and print.
 *
 *  Parameter values is a static array that we re-use here to save precious
 *  heap memory.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
print_elements(
	uint32_t 	act_flags,	/* indication of selected menus */
        unsigned long 	thistime,	/* current timestamp */
	const char 	*msg,		/* string to print in screen center */
	float 		*values)	/* sensor values */
{
#define AVGC 10
    static uint8_t 	ac;			/* average counter */
    int32_t 		pos[2] = { 0, 0 };	/* position to print */
    char 		buf[6];			/* print buffer */
    uint8_t 		sensor;			/* sensor index */
    uint8_t 		hours;			/* part of runtime */
    uint8_t 		minutes;		/* part of runtime */
    uint16_t 		seconds;		/* part of runtime */
    int 		num;			/* number of bytes printed */
    int 		len;			/* string length */
    const char		*str;			/* string */
    bool 		(*get_v)(void);		/* get element's visibility */
    uint8_t 		(*get_x)(void);		/* get element's x-position */
    uint8_t 		(*get_y)(void);		/* get element's y-position */
    const char *	(*get_t)(void);		/* get element's text */
    float 		sensor_value;		/* sensor voltage value */

    /* Clear whole screen while manipulating elements. */
    if (act_flags != 0) {
	cmd_clear(NULL);

	/* Make sure counting starts at zero when averaging. */
	ac = 0;

	if (act_flags & FLAG(MI_ABOUT)) {
            msg = cfg_get_version();
	}
    } else {

	/* Count number of values before calculating average value. */
	ac = (ac + 1) % AVGC;
    }

    /* Print sensor voltages. */
    for (sensor = SENSOR0; sensor < SENSOR_COUNT; sensor++) {
	switch (sensor) {
	case SENSOR0 :
	    get_v = cfg_get_sensv0;
	    get_x = cfg_get_sensx0;
	    get_y = cfg_get_sensy0;
	    get_t = cfg_get_senst0;
	    break;
	case SENSOR1 :
	    get_v = cfg_get_sensv1;
	    get_x = cfg_get_sensx1;
	    get_y = cfg_get_sensy1;
	    get_t = cfg_get_senst1;
	    break;
	case SENSOR2 :
	    get_v = cfg_get_sensv2;
	    get_x = cfg_get_sensx2;
	    get_y = cfg_get_sensy2;
	    get_t = cfg_get_senst2;
	    break;
	case SENSOR3 :
	    get_v = cfg_get_sensv3;
	    get_x = cfg_get_sensx3;
	    get_y = cfg_get_sensy3;
	    get_t = cfg_get_senst3;
	    break;
	}
	if (get_v()) {
            if (act_flags != 0) {
		sensor_value = sensor_get_value((sensor_t)sensor);
	    } else {
		values[sensor] +=
		    sensor_get_value((sensor_t)sensor) / (float)AVGC;
		if (ac == 0) {
		    sensor_value = values[sensor];
		    values[sensor] = 0;
		} else {
		    sensor_value = -1;
		}
	    }

	    /* Negative voltage means busy averaging, don't print yet. */
	    if (sensor_value > 0) {
		pos[0] = get_x();
		pos[1] = get_y();
		dtostrf(sensor_value, sizeof(buf) - 1, 2, buf);
		len = strlen(buf);
		cmd_p_raw(pos, 0, len, buf);
		num = len;
		str = get_t();
		len = strlen(str);
		if (len > 0) cmd_p_raw(pos, num, len, str);
	    }
	}
    }

    /* Print run time. */
    if (!cfg_get_timev()) return;
    pos[0] = cfg_get_timex();
    pos[1] = cfg_get_timey();
    seconds = thistime / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    minutes = minutes % 60;
    seconds = seconds % 60;
    utoa(hours, buf, 10);
    len = strlen(buf);
    num = 0;
    cmd_p_raw(pos, num, len, buf);
    num += len;
    buf[0] = ':';
    if (minutes < 10) {
	buf[1] = '0';
	len = 2;
    } else {
	len = 1;
    }
    utoa(minutes, buf + len, 10);
    cmd_p_raw(pos, num, 3, buf);
    num += 3;
    if (seconds < 10) {
	buf[1] = '0';
	len = 2;
    } else {
	len = 1;
    }
    utoa(seconds, buf + len, 10);
    cmd_p_raw(pos, num, 3, buf);
    num += 3;
    str = cfg_get_timet();
    len = strlen(str);
    if (len > 0) cmd_p_raw(pos, num, len, str);

    /* Print message. */
    if (msg != NULL) {
        len = strlen(msg);
	pos[0] = 14 - len/2;
	pos[1] = 6;
        cmd_p_raw(pos, 0, len, msg);
    }
} /* print_elements() */


/*------------------------------------------------------------------------
 *  Function	: print_menu
 *  Purpose	: Print active menu contents and all parent menu names.
 *  Method	: Walk from root menu to active menu, print all child menus
 *  		  and highlight current child.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
print_menu(
	menu_t *root,	/* root menu */
	menu_t *menu)	/* active menu */
{
#define X 9
#define Y 5
    int32_t 	pos[2] = { X, Y };	/* position to print */
    const char 	*str;			/* string to print */
    int 	len;			/* string length */
    int 	maxlen = 0;		/* max string length */
    int 	num;			/* number of bytes printed */
    int 	i;			/* loop couinter */
    menu_t 	*m;			/* current menu, starting at root */

    if (menu->children == NULL) return;
    cmd_clear(NULL);
    pos[0] = 1;
    pos[1] = 1;
    m = root;
    while (m != menu) {
	str = menu_get_name(m->children[m->ci].id);
	len = strlen(str);
	cmd_p_raw(pos, 0, len, str);
	m = &m->children[m->ci];
	pos[0] += 2;
	pos[1]++;
    }

    pos[0] -= 2;
    for (i = 0; menu->children[i].id != MI_NONE; i++) {
	str = menu_get_name(menu->children[i].id);
	len = strlen(str);
	if (len > maxlen) maxlen = len;
    }
    for (i = 0; menu->children[i].id != MI_NONE; i++) {
	num = 0;
	str = menu_get_name(menu->children[i].id);
	len = strlen(str);
	if (i == menu->ci && *str != '\0') {
	    cmd_p_raw(pos, num, 3, ">> ");
	}
	num += 3;
	cmd_p_raw(pos, num, len, str);
	num += len;
	if (i == menu->ci && *str != '\0') {
	    while (len < maxlen) {
	        cmd_p_raw(pos, num, 1, " ");
		num++;
		len++;
	    }
	    cmd_p_raw(pos, num, 3, " <<");
	}
	num += 3;
	pos[1]++;
    }
#undef X
#undef Y
} /* print_menu() */


/*------------------------------------------------------------------------
 *  Function	: adjust_element
 *  Purpose	: Adjust screen / element based on selected menus.
 *  Method	: Determine selected element and change it if needed.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
adjust_element(
	adjust_t	adjust,		/* adjust type */
	uint32_t 	act_flags,	/* indication of selected menus */
	const char 	**msg,		/* message */
	float 		*values,	/* sensor values */
	bool		*inv_enable)	/* whether to inverse enable on save */
{
    bool 	(*get_v)(void);		/* get element's visibility */
    uint8_t 	(*get_x)(void);		/* get element's x-position */
    uint8_t 	(*get_y)(void);		/* get element's y-position */
    bool 	(*set_v)(bool);		/* set element's visibility */
    bool 	(*set_x)(uint8_t);	/* set element's x-position */
    bool 	(*set_y)(uint8_t);	/* set element's y-position */
    sensor_t	sensor = SENSOR_NONE;	/* sensor element */
    static char	buf[6];			/* message buffer */
    int8_t 	val;			/* small integer value */
    const char 	*str;			/* static string */
    bool 	menu_change = false;
    bool 	bigsteps = false;

    if (act_flags & FLAG(MI_SENSOR0)) {
	get_v = cfg_get_sensv0;
	get_x = cfg_get_sensx0;
	get_y = cfg_get_sensy0;
	set_v = cfg_set_sensv0;
	set_x = cfg_set_sensx0;
	set_y = cfg_set_sensy0;
	sensor = SENSOR0;
    } else if (act_flags & FLAG(MI_SENSOR1)) {
	get_v = cfg_get_sensv1;
	get_x = cfg_get_sensx1;
	get_y = cfg_get_sensy1;
	set_v = cfg_set_sensv1;
	set_x = cfg_set_sensx1;
	set_y = cfg_set_sensy1;
	sensor = SENSOR1;
    } else if (act_flags & FLAG(MI_SENSOR2)) {
	get_v = cfg_get_sensv2;
	get_x = cfg_get_sensx2;
	get_y = cfg_get_sensy2;
	set_v = cfg_set_sensv2;
	set_x = cfg_set_sensx2;
	set_y = cfg_set_sensy2;
	sensor = SENSOR2;
    } else if (act_flags & FLAG(MI_SENSOR3)) {
	get_v = cfg_get_sensv3;
	get_x = cfg_get_sensx3;
	get_y = cfg_get_sensy3;
	set_v = cfg_set_sensv3;
	set_x = cfg_set_sensx3;
	set_y = cfg_set_sensy3;
	sensor = SENSOR3;
    } else if (act_flags & FLAG(MI_RUNTIME)) {
	get_v = cfg_get_timev;
	get_x = cfg_get_timex;
	get_y = cfg_get_timey;
	set_v = cfg_set_timev;
	set_x = cfg_set_timex;
	set_y = cfg_set_timey;
    } else {
	get_v = NULL;
	get_x = NULL;
	get_y = NULL;
	set_v = NULL;
	set_x = NULL;
	set_y = NULL;
    }

    switch (adjust) {
    case ADJ_CALIBRATION:
        if ((act_flags & FLAG(MI_PLUS)) != (act_flags & FLAG(MI_MINUS))
	    && act_flags & FLAG(MI_CALIBRATION)
	    && sensor != SENSOR_NONE) {
	    sensor_set_value(sensor, values[sensor]);
	    dtostrf(values[sensor], sizeof(buf) - 1, 2, buf);
	    *msg = buf;
	}
	break;
    case ADJ_MENUCHANGE:
	menu_change = true;
	/* FALLTHROUGH */
    case ADJ_BIGSTEPS:
	bigsteps = true;
	/* FALLTHROUGH */
    case ADJ_SMALLSTEPS:
        *msg = NULL;
        if ((act_flags & FLAG(MI_PLUS)) == (act_flags & FLAG(MI_MINUS))) {
	    if (act_flags & FLAG(MI_LAYOUT)
		&& set_v != NULL) {
		if (act_flags & FLAG(MI_VISIBLE)) {
		    set_v(true);
		} else if (act_flags & FLAG(MI_INVISIBLE)) {
		    set_v(false);
		}
	    }
	    if (act_flags & FLAG(MI_ENABLE)) {
		if (act_flags & FLAG(MI_YES)) {
		    cfg_set_enable(true);
		} else if (act_flags & FLAG(MI_NO)) {
		    cfg_set_enable(false);

		    /*
		     * Save settings in advance. They will also be saved when
		     * the menu becomes idle (in button_is_up()), but that
		     * could take too long if the user turns off the power
		     * earlier. Anyway, the second cfg_save() doesn't do much
		     * as it only saves pending changes; nothing in this case.
		     */
		    cfg_save(false);
		}
		*inv_enable = false;
	    }
	    if (act_flags & FLAG(MI_YES)) {
	        if (act_flags & FLAG(MI_FONTRESET)) {
		    cmd_font_reset(NULL);
		}
	        if (act_flags & FLAG(MI_FONTEFFECT)) {
		    int32_t fonteffect = FE_NONE;

	            if (act_flags & FLAG(MI_BORDER)) {
			fonteffect = FE_BORDER;
		    } else if (act_flags & FLAG(MI_SHADOW)) {
			fonteffect = FE_SHADOW;
		    } else if (act_flags & FLAG(MI_TRANSWHITE)) {
			fonteffect = FE_TRANSWHITE;
		    } else if (act_flags & FLAG(MI_BLACKWHITE)) {
			fonteffect = FE_BLACKWHITE;
		    } else if (act_flags & FLAG(MI_INVERT)) {
			fonteffect = FE_INVERT;
		    }
		    cmd_font_effect(&fonteffect);
		}
	    }
	    break;
	}
	/* (act_flags & FLAG(MI_PLUS)) != (act_flags & FLAG(MI_MINUS)) */
	if (act_flags & FLAG(MI_LAYOUT)
	    && get_v != NULL
	    && get_v()
	    && !menu_change
	    && act_flags & FLAG(MI_VISIBLE)) {
	    if (act_flags & FLAG(MI_HORIZONTAL)) {
		if (act_flags & FLAG(MI_PLUS)) {
		    set_x(get_x() + 1);
		    if (get_x() >= screenbuf.cols) {
			set_x(0);
		    }
		} else {
		    set_x(get_x() - 1);
		    if ((int8_t)get_x() < 0) {
			set_x(screenbuf.cols - 1);
		    }
		}
	    }
	    if (act_flags & FLAG(MI_VERTICAL)) {
		if (act_flags & FLAG(MI_PLUS)) {
		    set_y(get_y() + 1);
		    if (get_y() >= screenbuf.rows) {
			set_y(0);
		    }
		} else {
		    set_y(get_y() - 1);
		    if ((int8_t)get_y() < 0) {
			set_y(screenbuf.rows - 1);
		    }
		}
	    }
	}
	if (act_flags & FLAG(MI_CALIBRATION)
	    && sensor != SENSOR_NONE) {
	    if (!menu_change) {
		if (act_flags & FLAG(MI_PLUS)) {
		    values[sensor] += (bigsteps ? 0.1 : 0.01);
		} else {
		    values[sensor] -= (bigsteps ? 0.1 : 0.01);
		}
	    }
	    dtostrf(values[sensor], sizeof(buf) - 1, 2, buf);
	    *msg = buf;
	}
	if (act_flags & FLAG(MI_HOS)) {
	    if (!menu_change) {
		if (act_flags & FLAG(MI_PLUS)) {
		    val = max_hos(1);
		} else {
		    val = max_hos(-1);
		}
	    } else {
		val = max_hos(0);
	    }
	    if (val < 0) return;
	    val -= 32;
	    itoa(val, buf, 10);
	    *msg = buf;
	}
	if (act_flags & FLAG(MI_VOS)) {
	    if (!menu_change) {
		if (act_flags & FLAG(MI_PLUS)) {
		    val = max_vos(1);
		} else {
		    val = max_vos(-1);
		}
	    } else {
		val = max_vos(0);
	    }
	    if (val < 0) return;
	    val -= 16;
	    itoa(val, buf, 10);
	    *msg = buf;
	}
	if (act_flags & FLAG(MI_CBL)) {
	    if (!menu_change) {
		if (act_flags & FLAG(MI_PLUS)) {
		    val = max_cbl(-1, 1);
		} else {
		    val = max_cbl(-1, -1);
		}
	    } else {
	        val = max_cbl(-1, 0);
	    }
	    if (val < 0) return;
	    switch (val) {
	    case 0: str = "0%"; break;
	    case 1: str = "10%"; break;
	    case 2: str = "20%"; break;
	    case 3: str = "30%"; break;
	    default: str = "??%";
	    }
	    strcpy(buf, str);
	    *msg = buf;
	}
	if (act_flags & FLAG(MI_CWL)) {
	    if (!menu_change) {
		if (act_flags & FLAG(MI_PLUS)) {
		    val = max_cwl(-1, -1);
		} else {
		    val = max_cwl(-1, 1);
		}
	    } else {
	        val = max_cwl(-1, 0);
	    }
	    if (val < 0) return;
	    switch (val) {
	    case 0: str = "120%"; break;
	    case 1: str = "100%"; break;
	    case 2: str = "90%"; break;
	    case 3: str = "80%"; break;
	    default: str = "??%";
	    }
	    strcpy(buf, str);
	    *msg = buf;
	}
	if (act_flags & FLAG(MI_INSMUX1)) {
	    if (!menu_change) {
		if (act_flags & FLAG(MI_PLUS)) {
		    val = max_insmux1(1);
		} else {
		    val = max_insmux1(-1);
		}
	    } else {
		val = max_insmux1(0);
	    }
	    if (val < 0) return;
	    switch (val) {
	    case 0: str = "20ns"; break;
	    case 1: str = "30ns"; break;
	    case 2: str = "35ns"; break;
	    case 3: str = "60ns"; break;
	    case 4: str = "80ns"; break;
	    case 5: str = "110ns"; break;
	    default: str = "??ns";
	    }
	    strcpy(buf, str);
	    *msg = buf;
	}
	if (act_flags & FLAG(MI_INSMUX2)) {
	    if (!menu_change) {
		if (act_flags & FLAG(MI_PLUS)) {
		    val = max_insmux2(1);
		} else {
		    val = max_insmux2(-1);
		}
	    } else {
		val = max_insmux2(0);
	    }
	    if (val < 0) return;
	    switch (val) {
	    case 0: str = "30ns"; break;
	    case 1: str = "35ns"; break;
	    case 2: str = "50ns"; break;
	    case 3: str = "75ns"; break;
	    case 4: str = "100ns"; break;
	    case 5: str = "120ns"; break;
	    default: str = "??ns";
	    }
	    strcpy(buf, str);
	    *msg = buf;
 	}
	break;
    }
} /* adjust_element() */


/*------------------------------------------------------------------------
 *  Function	: read_sensors
 *  Purpose	: Populate sensor values array.
 *  Method	: Walk all sensors.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
read_sensors(
	float *values)	/* sensor values */
{
    for (uint8_t sensor = SENSOR0; sensor < SENSOR_COUNT; sensor++) {
	values[sensor] = sensor_get_value((sensor_t)sensor);
    }
} /* read_sensors() */


/*------------------------------------------------------------------------
 *  Function	: button_goes_down
 *  Purpose	: Handle case where sensor goes from high to low.
 *  Method	: 
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
button_goes_down(
	menus_t 	*menus,		/* active menu */
        unsigned long 	thistime,	/* current timestamp */
        unsigned long 	sensor_change,	/* timestamp last sensor change */
        unsigned long 	*menu_change,	/* timestamp last menu change */
	float 		*values,	/* sensor values */
	bool		*inv_enable)	/* whether to inverse enable on save */
{
} /* button_goes_down() */


/*------------------------------------------------------------------------
 *  Function	: button_is_down
 *  Purpose	: Handle case where sensor is low.
 *  Method	: Detect long button press and adjust menu/elements.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
button_is_down(
	menus_t 	*menus,		/* active menu */
        unsigned long 	thistime,	/* current timestamp */
        unsigned long 	sensor_change,	/* timestamp last sensor change */
        unsigned long 	*menu_change,	/* timestamp last menu change */
	float 		*values,	/* sensor values */
	bool		*inv_enable)	/* whether to inverse enable on save */
{
#define LONGPRESSTIME 1000
#define LONGLONGPRESSTIME 10000
    static unsigned long oldtime = thistime;	/* previous timestamp */
    unsigned long 	 span;			/* time span button pressed */

    /*
     * Don't open the menu when OSD image is off.
     * The user would not have a clue what he/she is doing.
     */
    if (!cfg_get_enable()) return;

    span = thistime -
	(sensor_change > *menu_change ? sensor_change : *menu_change);

    /*
     * We don't want to trigger element adjusting right away when choosing "+"
     * or "-". Therefore the test for smaller than OR equal to LONGPRESSTIME.
     * It is highly likely that the time between next calls of this function
     * is less that a millisecond; hence span exactly equals LONGPRESSTIME
     * at next call after choosing "+" or "-".
     */
    if (span <= LONGPRESSTIME) return;

    /* Handle button-hold while the elements are visible. */
    if (menus->active->children == NULL
        && thistime - oldtime > 100) {
        adjust_element(span > LONGLONGPRESSTIME
		       ? ADJ_BIGSTEPS : ADJ_SMALLSTEPS,
		       menus->act_flags, &menus->msg, values, inv_enable);
	oldtime = thistime;
	return;
    }

    /* Handle button-hold while the menu is visible. */
    if (&menus->active->children[menus->active->ci] != NULL) {
	menus->active->children[menus->active->ci].parent = menus->active;
	menus->act_flags |= FLAG(menus->active->children[menus->active->ci].id);
        adjust_element(ADJ_MENUCHANGE, menus->act_flags, &menus->msg, values,
		       inv_enable);
	menus->active = &menus->active->children[menus->active->ci];
	*menu_change = thistime;
	print_menu(menus->root, menus->active);
    }
#undef LONGPRESSTIME
#undef LONGLONGPRESSTIME
} /* button_is_down() */


/*------------------------------------------------------------------------
 *  Function	: button_goes_up
 *  Purpose	: Handle case where sensor goes from low to high.
 *  Method	: Detect button release and adjust menu/elements.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
button_goes_up(
	menus_t 	*menus,		/* active menu */
        unsigned long 	thistime,	/* current timestamp */
        unsigned long 	sensor_change,	/* timestamp last sensor change */
        unsigned long 	*menu_change,	/* timestamp last menu change */
	float 		*values,	/* sensor values */
	bool		*inv_enable)	/* whether to inverse enable on save */
{
    /* Toggle enable|disable while menu is idle. */
    if (menus->active == menus->root) {
	cfg_set_enable(!cfg_get_enable());
	*inv_enable = !*inv_enable;
    }

    if (menus->active->children != NULL) {

        /*
	 * Cache sensor values if not yet calibrating.
	 * While the button is up, we can be sure that all sensor values are
	 * normal. While calibrating in adjust_element() we use cached values
	 * while adjusting the voltage up or down. Note that while pressing
	 * the button, its sensor value is zero so we can't use that.
	 *
	 * Don't read sensors if menu is idle. In that case the values are
	 * used as buffer to calculate average values.
	 */
	if (menus->active != menus->root) read_sensors(values);
    } else {

        /*
	 * Adjust calibration of selected sensor if any.
	 * This uses a cached value to set the sensor at the correct voltage.
	 */
        adjust_element(ADJ_CALIBRATION, menus->act_flags, &menus->msg, values,
		       inv_enable);
    }

    /* Menu change is handled by button_is_down() already. */
    if (sensor_change < *menu_change) return;

    /* Handle short-clicks while the elements are visible. */
    if (menus->active->children == NULL) {
        adjust_element(ADJ_SMALLSTEPS, menus->act_flags, &menus->msg, values,
		       inv_enable);
        return;
    }

    /* Handle short-clicks while the menu is visible. */
    if (menus->active->children[menus->active->ci + 1].id != MI_NONE) {
	menus->active->ci++;
    } else {
	menus->active->ci = 0;
    }
    print_menu(menus->root, menus->active);
    *menu_change = thistime;
} /* button_goes_up() */


/*------------------------------------------------------------------------
 *  Function	: button_is_up
 *  Purpose	: Handle case where sensor is high.
 *  Method	: Detect long button inactivity and adjust menu.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
button_is_up(
	menus_t 	*menus,		/* active menu */
        unsigned long 	thistime,	/* current timestamp */
        unsigned long 	sensor_change,	/* timestamp last sensor change */
        unsigned long 	*menu_change,	/* timestamp last menu change */
	float 		*values,	/* sensor values */
	bool		*inv_enable)	/* whether to inverse enable on save */
{
#define NOPRESSRETURNTIME 2000
    if (thistime - (sensor_change > *menu_change
	? sensor_change : *menu_change) <= NOPRESSRETURNTIME) return;

    /* Handle long button inactivity. */
    if (menus->active->parent != NULL) {
	menus->active = menus->active->parent;
	menus->act_flags &=
	    ~FLAG(menus->active->children[menus->active->ci].id);

	/* Adjust element to remove the message if present. */
        adjust_element(ADJ_MENUCHANGE, menus->act_flags, &menus->msg, values,
		       inv_enable);

	*menu_change = thistime;
        print_menu(menus->root, menus->active);

	/* Save changes if any to eeprom, on idle. */
	if (menus->active == menus->root) {

	    /*
	     * This would generate 4 bytes more machine-code:
	     * if (*inv_enable) cfg_set_enable(!cfg_get_enable());
	     * cfg_save(false);
	     * if (*inv_enable) cfg_set_enable(!cfg_get_enable());
	     */
	    if (*inv_enable) {
		bool enable;	/* current configured value */

		enable = cfg_get_enable();
		cfg_set_enable(!enable);
		cfg_save(false);
		cfg_set_enable(enable);
	    } else {
	        cfg_save(false);
	    }

	    /*
	     * Set values to zero so that they can be used to calculate
	     * averages. See print_elements().
	     */
	    for (uint8_t sensor = SENSOR0; sensor < SENSOR_COUNT; sensor++) {
		values[sensor] = 0;
	    }
	}
    }
#undef NOPRESSRETURNTIME
} /* button_is_up() */


/*------------------------------------------------------------------------
 *  Function	: is_any_sensor_low
 *  Purpose	: Test whether a sensor is low, i.e. whether button is pressed.
 *  Method	: Convert value to boolean.
 *
 *  Returns	: Whether any sensor is low.
 *------------------------------------------------------------------------
 */
static bool
is_any_sensor_low(
	sensor_t *sensors)	/* usable sensors to test */
{
#define SENSOR_THRESHOLD 50

    for (uint8_t i = 0; sensors[i] != SENSOR_NONE; i++) {
        /* To avoid chicken & egg problem, use raw value. */
        if (sensor_get_raw(sensors[i]) < SENSOR_THRESHOLD) return true;
    }
    return false;
#undef SENSOR_THRESHOLD
} /* is_any_sensor_low() */


/*------------------------------------------------------------------------
 *  Function	: control_init
 *  Purpose	: Initialise sensor array with usable sensors to control
 *  		  the OSD. Control can be done with single push button.
 *  Method	: Store ids of sensors that are currently high.
 *
 *  Returns	: Static array ending with SENSOR_NONE.
 *------------------------------------------------------------------------
 */
static sensor_t *
control_init()
{
    static sensor_t sensors[SENSOR_COUNT + 1];	/* usable sensors array */
    uint8_t 	    sensor;			/* sensor index */
    uint8_t 	    num = 0;			/* number of usable sensors */

    /*
     * This code relies on the fact that SENSOR_NONE is the last (dummy)-
     * sensor. It stores all sensor ids of high sensors in the array and stores
     * SENSOR_NONE as the last element. E.g. if SENSOR0 and SENSOR2 appear to
     * be high, the resulting array will be as follows:
     * sensors[0] == SENSOR0
     * sensors[1] == SENSOR2
     * sensors[2] == SENSOR_NONE
     */
    for (sensor = SENSOR0; sensor < SENSOR_COUNT; sensor++) {
	sensors[num] = (sensor_t)sensor;
	sensors[num + 1] = SENSOR_NONE;
	if (!is_any_sensor_low(sensors + num)) {
	    num++;
	} else {
	    sensors[num] = SENSOR_NONE;	/* fix last value */
	}
    }
    return sensors;
} /* control_init() */


/*------------------------------------------------------------------------
 *  Function	: sensor_count
 *  Purpose	: Count number of usable sensors.
 *  Method	: Check array contents.
 *
 *  Returns	: Number of usable sensors to control the OSD.
 *------------------------------------------------------------------------
 */
static uint8_t
sensor_count(
	sensor_t *sensors)	/* usable sensors to test */
{
    uint8_t i = 0;

    while (sensors[i] != SENSOR_NONE) {
	i++;
    }
    return i;
} /* sensor_count() */


/*------------------------------------------------------------------------
 *  Function	: sensor_request_init1
 *  Purpose	: Primary sensor request initialisation.
 *  Method	: Call sensor_request() for the first time so that it
 *  		  initialises its static data.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
sensor_request_init1(void)
{
    sensor_request(false);
} /* sensor_request_init1() */


/*------------------------------------------------------------------------
 *  Function	: sensor_request_init2
 *  Purpose	: Secundary sensor request initialisation.
 *  Method	: Call sensor_request() for the second time while requesting
 *  		  to check for font reset.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
sensor_request_init2(void)
{
    sensor_request(true);
} /* sensor_request_init2() */


/*------------------------------------------------------------------------
 *  Function	: sensor_request
 *  Purpose	: Handle sensor request if any.
 *  Method	: Scan sensors for low values, use switch debouncing.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
sensor_request(
	bool	check_fontreset)	/* whether to check for font reset */
{
#define PRINTTIME 100
#define BOUNCETIME 50
    static menus_t		*menus = NULL;		/* menu structure */
    static sensor_t		*sensors = NULL;	/* usable sensors */
    static bool			sensor_low = false;	/* sensor state */
    static unsigned long	sensor_ch = millis();	/* last sensor change */
    static unsigned long 	menu_ch = millis();	/* last menu change */
    static unsigned long 	printtime = millis();	/* print timestamp */
    static unsigned long 	bouncetime = millis();	/* bounce timestamp */
    unsigned long 		thistime = millis();	/* current timestamp */
    static float		values[SENSOR_COUNT];	/* sensor values */
    static bool			inv_enable = false;	/* whether enable
							   should be inversed
							   on save */

    /* Initialise once. */
    if (menus == NULL) {
        menus = menus_init();
    }
    if (sensors == NULL) {
	sensors = control_init();
	/*
	 * Cache sensor values to allow calibration to be the first menu.
	 * Otherwise, if the button is held down until calibration of first
	 * sensor starts, an uninitialised value would be used.
	 * See button_goes_up() for more info.
	 */
	read_sensors(values);
    }
    if (check_fontreset && sensors != NULL) {
	uint8_t count;	/* number of usable sensors during previous call */

	/*
	 * Count the number of usable sensors to detect font reset sequence
	 * at startup. Resetting the font is done by holding a button down
	 * while starting up, and releasing it within a certain amount of
	 * time. If the number increases, a button was held down during
	 * startup, for which we reset the font. This can be used to load
	 * the font after installing GSOSD or when font is broken/empty.
	 * For those cases we can't use the menu to reset the font.
	 */
	count = sensor_count(sensors);
	sensors = control_init();
	if (count < sensor_count(sensors)) cmd_font_reset(NULL);
    }

    /*
     * Print element values a number of times per second. Too fast would result
     * in flickering (only when manipulating elements using menus) because
     * clearing/filling the screen buffer and printing it are done
     * asynchronous. We want to avoid too many empty screen prints.
     */
    if (thistime - printtime > PRINTTIME) {
	if (menus->active == menus->root
	    || menus->active->children == NULL) {
	    print_elements(menus->act_flags, thistime, menus->msg,
		           values);
	}
	printtime = thistime;
    }

    if (is_any_sensor_low(sensors)) {

	/* Button pressed. */
	if (!sensor_low) {
	    bouncetime = thistime;
	    sensor_low = true;
	}

	/*
	 * Debouncing.
	 * When the sensor changes state, it will bounce. So we allow some
	 * bouncing time before registering a sensor change.
	 */
	if (thistime - bouncetime > BOUNCETIME) {
	    if (sensor_ch < bouncetime) {
		button_goes_down(menus, thistime, sensor_ch, &menu_ch, values,
			         &inv_enable);
		sensor_ch = thistime;
	    }
	    button_is_down(menus, thistime, sensor_ch, &menu_ch, values,
		           &inv_enable);
	}
    } else {

	/* Button released. */
	if (sensor_low) {
	    bouncetime = thistime;
	    sensor_low = false;
	}

	/* Debouncing, see comment above. */
	if (thistime - bouncetime > BOUNCETIME) {
	    if (sensor_ch < bouncetime) {
		button_goes_up(menus, thistime, sensor_ch, &menu_ch, values,
			       &inv_enable);
		sensor_ch = thistime;
	    }
	    button_is_up(menus, thistime, sensor_ch, &menu_ch, values,
		         &inv_enable);
	}
    }
#undef PRINTTIME
#undef BOUNCETIME
} /* sensor_request() */

#endif
