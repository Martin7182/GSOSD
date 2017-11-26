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
 *  File	: command.cpp
 *  Purpose	: API commands handling.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/05/26
 *
 *========================================================================
 */

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include "max7456.h"
#include "sensor.h"
#include "config.h"
#include "font.h"
#include "globals.h"
#include "command.h"

//Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

/* X-macros generating command names array. */
#define X(key, name, func, nargs, data) \
    const char s_ ## func[] PROGMEM = name;
    COMMAND_TABLE
#undef X
#define X(key, name, func, type, stype, def) \
    const char s_cmd_cfg_set_ ## func[] PROGMEM = "SET_" name;
    CONFIG_TABLE1
#undef X
#define X(key, name, func, type, stype, def) \
    const char s_cmd_cfg_get_ ## func[] PROGMEM = "GET_" name;
    CONFIG_TABLE1
#undef X
const char * const CMD_NAMES[] PROGMEM = {
#define X(key, name, func, nargs, data) s_ ## func,
    COMMAND_TABLE
#undef X
#define X(key, name, func, type, stype, def) s_cmd_cfg_set_ ## func,
    CONFIG_TABLE1
#undef X
#define X(key, name, func, type, stype, def) s_cmd_cfg_get_ ## func,
    CONFIG_TABLE1
#undef X
};

/* X-macro generating command property matrix. */
cmdprops_t CMD_PROPS[] = {
#define X(key, name, func, nargs, data) { func, nargs, data },
    COMMAND_TABLE
#undef X
#define X(key, name, func, type, stype, def)				\
{ cmd_cfg_set_ ## func, 1, false },
    CONFIG_TABLE1
#undef X
#define X(key, name, func, type, stype, def)				\
{ cmd_cfg_get_ ## func, 0, false },
    CONFIG_TABLE1
#undef X
};


#if 1
#define CH_BORDER_L 	(0x01)
#define CH_BORDER_R 	(0x02)
#define CH_BORDER_T 	(0x03)
#define CH_BORDER_B 	(0x04)
#define CH_BORDER_LT 	(0x05)
#define CH_BORDER_RT 	(0x06)
#define CH_BORDER_LB 	(0x07)
#define CH_BORDER_RB 	(0x08)
#else
# Alternative borders:
#define CH_BORDER_L 	(0x8c)
#define CH_BORDER_R 	(0x9c)
#define CH_BORDER_T 	(0x8d)
#define CH_BORDER_B 	(0x9d)
#define CH_BORDER_LT 	(0x8e)
#define CH_BORDER_RT 	(0x8f)
#define CH_BORDER_LB 	(0x9e)
#define CH_BORDER_RB 	(0x9f)
#endif

/* X-macro generating setters. */
#define X(key, name, func, type, stype, def) 	\
bool cmd_cfg_set_ ## func(int32_t *args, ...)	\
{ 						\
    return cfg_set_ ## func((type)args[0]);	\
}
CONFIG_TABLE1
#undef X

/* X-macro generating getters. */
#define X(key, name, func, type, stype, def)	\
bool cmd_cfg_get_ ## func(int32_t *args, ...)	\
{						\
    bool stx = false; /* whether STX printed */ \
    						\
    if (!cfg_get_silent()			\
        && (cfg_get_control() & 0x01) != 0x00) {\
        Serial.write((byte)CONTROL_STX);	\
	stx = true;				\
    }						\
    Serial.print(cfg_get_ ## func()); 		\
    if (stx) {					\
        Serial.write((byte)CONTROL_ETX);	\
    }						\
    return true;				\
}
CONFIG_TABLE1
#undef X


/*------------------------------------------------------------------------
 *  Function	: draw_borders
 *  Purpose	: Draw borders around given window coordinates.
 *  Method	: Fill shadow screen buffer with graphic characters.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
draw_borders(
    int16_t 	x,	/* X-coordinate */
    int16_t 	y,	/* Y-coordinate */
    int16_t 	w,	/* width */
    int16_t 	h)	/* height */
{
    int8_t 	xw;	/* X-coordinate of window position */
    int8_t 	yw;	/* Y-coordinate of window position */
    bool	lborder;/* whether to draw left border */
    bool	rborder;/* whether to draw right1 border */
    bool	tborder;/* whether to draw top border */
    bool	bborder;/* whether to draw bottom border */

    if (w < 0) {
	w = -w;
        lborder = rborder = true;
    } else {
        lborder = rborder = false;
    }
    if (h < 0) {
	h = -h;
        tborder = bborder = true;
    } else {
        tborder = bborder = false;
    }

    /*
     * Only draw visible borders.
     * Borders falling off the screen are no error.
     */
    lborder = lborder && (x > 0);
    rborder = rborder && (x + w < screenbuf.cols);
    tborder = tborder && (y > 0);
    bborder = bborder && (y + h < screenbuf.rows);

    /* draw visible corners */
    if (lborder && tborder) {
	xw = x - 1;
	yw = y - 1;
	SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_LT);
    }
    if (rborder && tborder) {
	xw = x + w;
	yw = y - 1;
	SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_RT);
    }
    if (lborder && bborder) {
	xw = x - 1;
	yw = y + h;
	SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_LB);
    }
    if (rborder && bborder) {
	xw = x + w;
	yw = y + h;
	SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_RB);
    }

    if (lborder) {
	/* draw left border */
	xw = x - 1;
	for (yw = y; yw < y + h; yw++) {
	    SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_L);
	}
    }
    if (rborder) {
	/* draw right border */
	xw = x + w;
	for (yw = y; yw < y + h; yw++) {
	    SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_R);
	}
    }
    if (tborder) {
	/* draw top border */
	yw = y - 1;
	for (xw = x; xw < x + w; xw++) {
	    SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_T);
	}
    }
    if (bborder) {
	/* draw bottom border */
	yw = y + h;
	for (xw = x; xw < x + w; xw++) {
	    SCREENSET(screenbuf, yw * screenbuf.cols + xw, CH_BORDER_B);
	}
    }
} /* draw_borders() */


/*------------------------------------------------------------------------
 *  Function	: print_raw
 *  Purpose	: Print raw data at pos to shadow screen buffer.
 *  Method	: Use SCREENSET macro.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
static bool
print_raw(
    int16_t	x,	/* X-coordinate */
    int16_t	y,	/* Y-coordinate */
    int		i,	/* num of bytes processed in earlier calls */
    int		len,	/* data length in bytes */
    const char	*data)	/* data to process */
{
    if (y < 0 || x < 0 || y >= screenbuf.rows || x >= screenbuf.cols) {
	return false;	/* obviously wrong */
    }
    x = x + i;
    for (int j = 0; j < len; j++, x++) {
	if (x >= screenbuf.cols) {
	    break; 	/* silently abort */
	}
	SCREENSET(screenbuf, y * screenbuf.cols + x, data[j]);
    }
    return true;
} /* print_raw() */


/*------------------------------------------------------------------------
 *  Function	: cmd_output_uint8
 *  Purpose	: Output uint8_t value possibly wrapped in STX/ETX.
 *  Method	: Serial print/write.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
cmd_output_uint8(
	uint8_t val)	/* value to output */
{
    bool stx = false; 	/* whether STX printed */

    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    Serial.print(val);
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
} /* cmd_output_uint8() */


/*------------------------------------------------------------------------
 *  Function	: cmd_about
 *  Purpose	: Show about this project.
 *  Method	: Serial print/write.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_about(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    bool stx = false; 	/* whether STX printed */

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<about>");
    }
#endif
    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    Serial.println();
    Serial.print("This is GS-OSD, version: ");
    Serial.println(cfg_get_version());
    Serial.println("For more info, visit: https://github.com/Martin7182/GSOSD");
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return true;
} /* cmd_about() */


/*------------------------------------------------------------------------
 *  Function	: cmd_list
 *  Purpose	: List all commands.
 *  Method	: Walk the static command list and print to serial output.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_list(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
#define NAME "name         "
#define NAMELEN sizeof(NAME)

    cmdprops_p 	cmdprops;	/* command properties */
    char 	buf[CMD_SIZE];	/* buffer for PROGMEM strings */
    bool 	stx = false; 	/* whether STX printed */

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<list>");
    }
#endif

    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    Serial.println();
    Serial.print("number");
    Serial.print('\t');
    Serial.print(NAME);
    Serial.print('\t');
    Serial.print("argc");
    Serial.print('\t');
    Serial.print("use_data");
    Serial.println();
    for (uint8_t i = 0; i < COMMAND_COUNT; i++) {
	strlcpy_P(buf, (char *)pgm_read_word(&(CMD_NAMES[i])), sizeof(buf));
	cmdprops = &CMD_PROPS[i];
	Serial.print(i);
	Serial.print('\t');
	Serial.print(buf);
	for (uint8_t len = strlen(buf); len < NAMELEN; len++) {
	    Serial.print(' ');
	}
	Serial.print('\t');
	Serial.print(cmdprops->argc);
	Serial.print('\t');
	Serial.print(cmdprops->use_data ? "true" : "false");
	Serial.println();
    }
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return true;
} /* cmd_list() */


/*------------------------------------------------------------------------
 *  Function	: cmd_reset
 *  Purpose	: Reset the Max7456 chip.
 *  Method	: Call max_reset().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_reset(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<reset>");
    }
#endif

    return max_reset(false);
} /* cmd_reset() */


/*------------------------------------------------------------------------
 *  Function	: cmd_clearpart
 *  Purpose	: Clear part of screen.
 *  Method	: Fill shadow screen buffer with empty values.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_clearpart(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	x;	/* X-coordinate; negative means all */
    int16_t	y;	/* Y-coordinate; negative means all */
    int16_t	w;	/* width, negative means all */
    int16_t	h;	/* height, negative means all */

    x = args[0];
    y = args[1];
    w = args[2];
    h = args[3];
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<clearpart");
	Serial.print(" x=");
	Serial.print(x);
	Serial.print(" y=");
	Serial.print(y);
	Serial.print(" w=");
	Serial.print(w);
	Serial.print(" h=");
	Serial.print(h);
	Serial.print(">");
    }
#endif

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w < 0) w = screenbuf.cols;
    if (h < 0) h = screenbuf.rows;
    if (x >= screenbuf.cols) x = screenbuf.cols - 1;
    if (y >= screenbuf.rows) y = screenbuf.rows - 1;
    if (w > screenbuf.cols - x) w = screenbuf.cols - x;
    if (h > screenbuf.rows - y) h = screenbuf.rows - y;
    for (int wy = y; wy < y + h; wy++) {
        for (int wx = x; wx < x + w; wx++) {
	    /* 0x00 acts as empty value */
	    SCREENSET(screenbuf, wy * screenbuf.cols + wx, 0x00);
	}
    }
    return true;
} /* cmd_clearpart() */


/*------------------------------------------------------------------------
 *  Function	: cmd_clear
 *  Purpose	: Clear screen.
 *  Method	: Fill shadow screen buffer with empty values.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_clear(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    uint16_t	pos;	/* screen buffer position */

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<clear>");
    }
#endif

    pos = screenbuf.cols * screenbuf.rows;
    do {
	pos--;
	SCREENSET(screenbuf, pos, 0x00); /* 0x00 acts as empty value */
    } while (pos > 0);
    return true;
} /* cmd_clear() */


/*------------------------------------------------------------------------
 *  Function	: cmd_dump
 *  Purpose	: Dump config settings to serial output.
 *  Method	: Call cfg_dump().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_dump(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<dump>");
    }
#endif

    return cfg_dump();
} /* cmd_dump() */


/*------------------------------------------------------------------------
 *  Function	: cmd_load
 *  Purpose	: Load config settings from eeprom.
 *  Method	: Call cfg_load().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_load(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<load>");
    }
#endif

    return cfg_load(false);
} /* cmd_load() */


/*------------------------------------------------------------------------
 *  Function	: cmd_defaults
 *  Purpose	: Load default config settings from eeprom.
 *  Method	: Call cfg_load_defaults().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_defaults(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<defaults>");
    }
#endif

    cfg_load_defaults();
    return true;
} /* cmd_defaults() */


/*------------------------------------------------------------------------
 *  Function	: cmd_save
 *  Purpose	: Save config settings to eeprom.
 *  Method	: Call cfg_save().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_save(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<save>");
    }
#endif

    return cfg_save(false);
} /* cmd_save() */


/*------------------------------------------------------------------------
 *  Function	: cmd_hos
 *  Purpose	: Shift OSD text left or right.
 *  Method	: Call max_hos().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_hos(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	num;	/* number of pixels to shift left or right */
    bool 	result;	/* return value */
    uint8_t	val;	/* new horizontal offset value */

    num = args[0];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<hos");
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(">");
    }
#endif

    if ((result = (val = max_hos(num)) >= 0)) cmd_output_uint8(val);
    return result;
} /* cmd_hos() */


/*------------------------------------------------------------------------
 *  Function	: cmd_vos
 *  Purpose	: Shift OSD text up or down.
 *  Method	: Call max_vos().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_vos(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	num;	/* number of pixels to shift down or up */
    bool 	result;	/* return value */
    uint8_t	val;	/* new vertical offset value */

    num = args[0];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<vos");
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(">");
    }
#endif

    if ((result = (val = max_vos(num)) >= 0)) cmd_output_uint8(val);
    return result;
} /* cmd_vos() */


/*------------------------------------------------------------------------
 *  Function	: cmd_insmux1
 *  Purpose	: Adjust OSD Rise and Fall Time - typical transition times
 *                between adjacent OSD pixels.
 *  Method	: Call max_insmux1().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_insmux1(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	num;	/* number to inc or dec */
    bool 	result;	/* return value */
    uint8_t	val;	/* new value */

    num = args[0];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<insmux1");
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(">");
    }
#endif

    if ((result = (val = max_insmux1(num)) >= 0)) cmd_output_uint8(val);
    return result;
} /* cmd_insmux1() */


/*------------------------------------------------------------------------
 *  Function	: cmd_insmux2
 *  Purpose	: Adjust OSD Insertion Mux Switching Time – typical transition
 *                times between input video and OSD pixels.
 *  Method	: Call max_insmux2().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_insmux2(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	num;	/* number to inc or dec */
    bool 	result;	/* return value */
    uint8_t	val;	/* new value */

    num = args[0];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<insmux2");
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(">");
    }
#endif

    if ((result = (val = max_insmux2(num)) >= 0)) cmd_output_uint8(val);
    return result;
} /* cmd_insmux2() */


/*------------------------------------------------------------------------
 *  Function	: cmd_cbl
 *  Purpose	: Adjust Character Black Level.
 *  Method	: Call max_cbl().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_cbl(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int8_t	line;	/* line number to adjust, -1 for all */
    int16_t	num;	/* number to inc or dec */
    bool 	result;	/* return value */
    uint8_t	val;	/* new value */

    line = (int8_t)args[0];
    num = args[1];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<cbl");
	Serial.print(" line=");
	Serial.print(num);
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(">");
    }
#endif

    if ((result = (val = max_cbl(line, num)) >= 0)) cmd_output_uint8(val);
    return result;
} /* cmd_cbl() */


/*------------------------------------------------------------------------
 *  Function	: cmd_cwl
 *  Purpose	: Adjust Character White Level.
 *  Method	: Call max_cwl().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_cwl(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int8_t	line;	/* line number to adjust, -1 for all */
    int16_t	num;	/* number to inc or dec */
    bool 	result;	/* return value */
    uint8_t	val;	/* new value */

    line = (int8_t)args[0];
    num = args[1];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<cwl");
	Serial.print(" line=");
	Serial.print(num);
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(">");
    }
#endif

    if ((result = (val = max_cwl(line, num)) >= 0)) cmd_output_uint8(val);
    return result;
} /* cmd_cwl() */


/*------------------------------------------------------------------------
 *  Function	: cmd_p_raw
 *  Purpose	: Print raw data from serial connection.
 *  Method	: Call print_raw().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_p_raw(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	x;	/* X-coordinate */
    int16_t	y;	/* Y-coordinate */
    int		i;	/* num of bytes processed in earlier calls */
    int		len;	/* data length in bytes */
    const char	*data;	/* data to process */
    va_list	ap;	/* va_list handle */
    bool 	result;	/* return value */

    x = args[0];
    y = args[1];
    va_start(ap, args);
    i = va_arg(ap, int);
    len = va_arg(ap, int);
    data = va_arg(ap, const char *);
    va_end(ap);

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<p_raw");
	Serial.print(" x=");
	Serial.print(x);
	Serial.print(" y=");
	Serial.print(y);
	Serial.print(" i=");
	Serial.print(i);
	Serial.print(" len=");
	Serial.print(len);
	Serial.print(" data=");
	for (int j = 0; j < len; j++) {
            char ch;	/* data character */
	    if (isprint(ch = data[j])) {
		Serial.write(ch);
	    } else {
		Serial.print("\\0x");
		Serial.print(ch, HEX);
	    }
	}
	Serial.print(">");
    }
#endif

    result = print_raw(x, y, i, len, data);
    return result;
} /* cmd_p_raw() */


/*------------------------------------------------------------------------
 *  Function	: cmd_p_window
 *  Purpose	: Print interpreted data from serial connection to window.
 *  Method	: Use SCREENSET macro.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_p_window(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	x;	/* X-coordinate */
    int16_t	y;	/* Y-coordinate */
    int16_t	w;	/* width; negative means with left & right border */
    int16_t	h;	/* height negative means with top & bottom border */
    int		i;	/* num of bytes processed in earlier calls */
    int		len;	/* data length in bytes */
    const char	*data;	/* data to process */
    va_list	ap;	/* va_list handle */
    int8_t 	xw;	/* X-coordinate of window position */
    int8_t 	yw;	/* Y-coordinate of window position */
    int 	j;	/* loop counter  */

    x = args[0];
    y = args[1];
    w = args[2];
    h = args[3];
    va_start(ap, args);
    i = va_arg(ap, int);
    len = va_arg(ap, int);
    data = va_arg(ap, const char *);
    va_end(ap);

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<p_window");
	Serial.print(" x=");
	Serial.print(x);
	Serial.print(" y=");
	Serial.print(y);
	Serial.print(" w=");
	Serial.print(w);
	Serial.print(" h=");
	Serial.print(h);
	Serial.print(" i=");
	Serial.print(i);
	Serial.print(" len=");
	Serial.print(len);
	Serial.print(" data=");
	for (j = 0; j < len; j++) {
            char ch;	/* data character */
	    if (isprint(ch = data[j])) {
		Serial.write(ch);
	    } else {
		Serial.print("\\0x");
		Serial.print(ch, HEX);
	    }
	}
	Serial.print(">");
    }
#endif

    if (y < 0 || x < 0 || w == 0 || h == 0
        || y + abs(h) > screenbuf.rows || x + abs(w) > screenbuf.cols) {
	return false;	/* obviously wrong */
    }

    if (i == 0) {
	draw_borders(x, y, w, h);
    }
    w = abs(w);
    h = abs(h);

    /*
     * Get first empty position.
     * Scan last column top to bottom.
     */
    yw = y;
    xw = x + w - 1;
    while (yw < y + h && screenbuf.buf[yw * screenbuf.cols + xw]) {
	yw++;
    }
    if (yw < y + h) {
	/*
	 * At least lower right is empty.
	 * Scan current row left to right.
	 */
	xw = x;
	while (screenbuf.buf[yw * screenbuf.cols + xw]) {
	    xw++;
	}
    }

    for (j = 0; j < len; j++) {
	if (yw == y + h) {
	    /* scroll up */
	    for (yw = y; yw < y + h - 1; yw++) {
	        for (xw = x; xw < x + w; xw++) {
		    SCREENSET(screenbuf, yw * screenbuf.cols + xw,
		              screenbuf.buf[(yw + 1) * screenbuf.cols + xw]);
	        }
	    }
	    for (xw = x; xw < x + w; xw++) {
	        SCREENSET(screenbuf, yw * screenbuf.cols + xw, 0x00);
	    }
	    yw = y + h - 1;
	    xw = x;
	}
	if (data[j] == 0x0D) {
	    /* newline, fill rest of line with spaces */
	    while (xw < x + w) {
	        SCREENSET(screenbuf, yw * screenbuf.cols + xw, ' ');
	        xw++;
	    }
	    yw++;
	    xw = x;
	} else {
	    SCREENSET(screenbuf, yw * screenbuf.cols + xw, data[j]);
	    if (++xw == x + w) {
		yw++;
		xw = x;
	    }
	}
    }
    return true;
} /* cmd_p_window() */


/*------------------------------------------------------------------------
 *  Function	: cmd_p_banner
 *  Purpose	: Print raw data from serial connection as rolling banner.
 *  Method	: Use SCREENSET macro.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_p_banner(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int16_t	x;	/* X-coordinate */
    int16_t	y;	/* Y-coordinate */
    int16_t	w;	/* width */
    int16_t	h;	/* height */
    int		i;	/* num of bytes processed in earlier calls */
    int		len;	/* data length in bytes */
    const char	*data;	/* data to process */
    va_list	ap;	/* va_list handle */
    int8_t 	xw;	/* X-coordinate of window position */
    int8_t 	yw;	/* Y-coordinate of window position */

    x = args[0];
    y = args[1];
    w = args[2];
    h = args[3];
    va_start(ap, args);
    i = va_arg(ap, int);
    len = va_arg(ap, int);
    data = va_arg(ap, const char *);
    va_end(ap);

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<p_banner");
	Serial.print(" x=");
	Serial.print(x);
	Serial.print(" y=");
	Serial.print(y);
	Serial.print(" w=");
	Serial.print(w);
	Serial.print(" h=");
	Serial.print(h);
	Serial.print(" i=");
	Serial.print(i);
	Serial.print(" len=");
	Serial.print(len);
	Serial.print(" data=");
	for (int j = 0; j < len; j++) {
            char ch;	/* data character */
	    if (isprint(ch = data[j])) {
		Serial.write(ch);
	    } else {
		Serial.print("\\0x");
		Serial.print(ch, HEX);
	    }
	}
	Serial.print(">");
    }
#endif

    if (y < 0 || x < 0 || w == 0 || h == 0
        || y + abs(h) > screenbuf.rows || x + abs(w) > screenbuf.cols) {
	return false;	/* obviously wrong */
    }

    if (i == 0) {
	draw_borders(x, y, w, h);
    }
    w = abs(w);
    h = abs(h);

    /*
     * Get first empty position. As a banner, likely all space is occupied;
     * scan backwards for first filled position, for performance.
     */
    yw = y + h - 1;
    xw = 0;
    while (yw >= y) {
        xw = x + w - 1;
	while (xw >= x && !screenbuf.buf[yw * screenbuf.cols + xw]) {
	    xw--;
	}
	if (xw >= x) {
	    /* filled position found */
	    if (xw == x + w - 1) {
		/* at last pos of current line */
		if (++yw == y + h) {
		    /* at last line */
		    xw = x + w;
		} else {
		    /* before last line */
		    xw = x;
		}
	    } else {
		/* before last pos of current line */
		xw++;
	    }
	    break;
        }
	/* filled position not found yet */
	yw--;
    }
    if (yw < y) {
	/* filled position not found */
        yw = y;
        xw = x;
    }

    for (int j = 0; j < len; j++) {
	if (yw == y + h) {
	    /* scroll left and possibly up */
	    for (yw = y; yw < y + h; yw++) {
		for (xw = x; xw < x + w - 1; xw++) {
		    SCREENSET(screenbuf, yw * screenbuf.cols + xw,
		              screenbuf.buf[yw * screenbuf.cols + xw + 1]);
		}
		/* xw = x + w - 1 */
		if (yw < y + h - 1) {
		    SCREENSET(screenbuf, yw * screenbuf.cols + xw,
		              screenbuf.buf[(yw + 1) * screenbuf.cols + x]);
		}
	    }
	    yw--; /* yw = y + h - 1 */
	}
	SCREENSET(screenbuf, yw * screenbuf.cols + xw, data[j]);
	if (++xw == x + w) {
	    /* current line is full, move to first position of next line */
	    yw++;
	    xw = x;
	}
    }

    return true;
} /* cmd_p_banner() */


/*------------------------------------------------------------------------
 *  Function	: cmd_font_effect
 *  Purpose	: Apply effect to current font in Max7456 chip (eeprom).
 *  Method	: Call font_apply().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_font_effect(
	int32_t *args,	/* integer arguments */
	...)		/* data arguments */
{
    fonteffect_t	effect;	/* font effect to apply */

    effect = (fonteffect_t)args[0];

#ifndef no_debug
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<font_effect");
	Serial.print(" effect=");
	Serial.print(effect);
	Serial.print(">");
    }
#endif

    return font_apply(&effect);
} /* cmd_font_effect() */


/*------------------------------------------------------------------------
 *  Function	: cmd_font_reset
 *  Purpose	: Load default font into Max7456 chip (eeprom).
 *  Method	: Call font_apply().
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_font_reset(
	int32_t *args,	/* integer arguments */
	...)		/* data arguments */
{
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
        Serial.print("<font_reset>");
    }
#endif

    return font_apply(NULL);
} /* cmd_font_reset() */


/*------------------------------------------------------------------------
 *  Function	: cmd_set_font
 *  Purpose	: Put font character to Max7456 chip (eeprom).
 *  Method	: Fill local buffer, write font character when last byte
 *  		  received.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_set_font(
	int32_t *args,	/* integer arguments */
	...)		/* data arguments */
{
    int32_t		num;	/* number of character to set [0 .. 255] */
    int			i;	/* num of bytes processed in earlier calls */
    int			j;	/* loop counter */
    int			len;	/* data length in bytes */
    char		ch;	/* data character */
    int8_t 		biti;	/* bit index in byte [0 .. 7]; MSB = 7 */
    uint8_t 		bytei;	/* byte index in buf [0 .. sizeof(buf) - 1] */
    const char		*data;	/* data to process */
    va_list		ap;	/* va_list handle */
    static fontbuf_t 	buf;	/* font character buffer */
    static uint16_t	bits;	/* number of logical bits processed */

    num = args[0];
    va_start(ap, args);
    i = va_arg(ap, int);
    len = va_arg(ap, int);
    data = va_arg(ap, const char *);
    va_end(ap);
#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<set_font");
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(" i=");
	Serial.print(i);
	Serial.print(" len=");
	Serial.print(len);
	Serial.print(" data=");
	for (j = 0; j < len; j++) {
	    if (isprint(ch = data[j])) {
		Serial.write(ch);
	    } else {
		Serial.print("\\0x");
		Serial.print(ch, HEX);
	    }
	}
	Serial.print(">");
    }
#endif

    if (num < 0 || num > 255) {
	return false;
    }

    if (i == 0) {
	/* New request, clear static buffer. */
        for (j = 0; j < (int)sizeof(buf); j++) {
	    buf[j] = 0;
	}
	bits = 0;
    }
    for (j = 0; j < len; j++) {
	ch = data[j];
	if (isspace(ch)) continue;			/* skip white space */
	if (ch != '0' && ch != '1') return false;	/* unexpected data */
	bytei = bits / 8;
	if (bytei >= sizeof(buf)) {
	    return false; 				/* internal error */
	}

	/*
	 * Communication is done human readable; all logical bits are
	 * sent using bytes '0' and '1'. All logical bytes are sent
	 * over the line with MSB first.
	 */
	biti = 7 - bits % 8;
	if (ch == '1') {
	    buf[bytei] |= (0x01 << biti);
	}
        if (++bits == 8 * sizeof(buf)) {
	    /* Last bit received, execute. */
	    return max_fontcharput((uint8_t)num, &buf);
	}
    }
    return true;
} /* cmd_set_font() */


/*------------------------------------------------------------------------
 *  Function	: cmd_get_font
 *  Purpose	: Get font character from Max7456 chip (eeprom).
 *  Method	: Read font character into buffer, print buffer contents.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_get_font(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    int32_t	num;		/* number of char to get [0 .. 255] */
    fontbuf_t 	buf;		/* font character buffer */
    bool 	stx = false; 	/* whether STX printed */

    num = args[0];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<get_font");
	Serial.print(" num=");
	Serial.print(num);
	Serial.print(">");
    }
#endif

    if (num < 0 || num > 255) {
	return false;
    }

    if(!max_fontcharget((uint8_t)num, &buf)) {
	return false;
    }
    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    for (uint8_t bytei = 0; bytei < sizeof(buf); bytei++) {
        for (int8_t biti = 7; biti >= 0; biti--) {

	    /*
	     * Communication is done human readable; all logical bits are
	     * sent using bytes '0' and '1'. All logical bytes are sent
	     * over the line with MSB first.
	     */
	    if ((buf[bytei] & (0x01 << biti)) != 0) {
                Serial.print("1");
	    } else {
                Serial.print("0");
	    }
        }
        Serial.print(" ");
    }
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return true;
} /* cmd_get_font() */


/*------------------------------------------------------------------------
 *  Function	: cmd_get_sensor
 *  Purpose	: Get analogue sensor value.
 *  Method	: Call minim_get_sensor() and serial write value.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_get_sensor(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
#define EPS (-0.001)

    int16_t	id;		/* sensor id */
    float	val;		/* sensor value */
    bool 	stx = false; 	/* whether STX printed */

    id = args[0];

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<get_sensor");
	Serial.print(" id=");
	Serial.print(id);
	Serial.print(">");
    }
#endif

    if ((val = sensor_get_value((sensor_t)id)) < EPS) {
	return false;
    }
    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    Serial.print(val);
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return true;
} /* cmd_get_sensor() */


/*------------------------------------------------------------------------
 *  Function	: cmd_get_width
 *  Purpose	: Get screen width.
 *  Method	: Write to serial.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_get_width(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    bool stx = false; 	/* whether STX printed */

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<get_width>");
    }
#endif
    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    Serial.print(screenbuf.cols);
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return true;
} /* cmd_get_width() */


/*------------------------------------------------------------------------
 *  Function	: cmd_get_height
 *  Purpose	: Get screen height.
 *  Method	: Write to serial.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_get_height(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    bool stx = false; 	/* whether STX printed */

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<get_height>");
    }
#endif
    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    Serial.print(screenbuf.rows);
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return true;
} /* cmd_get_height() */


/*------------------------------------------------------------------------
 *  Function	: cmd_get_time
 *  Purpose	: Get runtime in milliseconds since last start.
 *  Method	: Write to serial.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cmd_get_time(
    int32_t 	*args,	/* integer arguments */
		...)	/* data arguments */
{
    bool stx = false; 	/* whether STX printed */

#ifndef NO_DEBUG
    if (!cfg_get_silent() && cfg_get_debug()) {
	Serial.print("<get_time>");
    }
#endif
    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    Serial.print(millis());
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return true;
} /* cmd_get_time() */
