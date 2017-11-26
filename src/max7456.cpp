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
 *  File	: max7456.cpp
 *  Purpose	: Routines that manipulate the Max7456 chip.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/06/03
 *
 *========================================================================
 */

#include <Arduino.h>
#include <SPI.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"
#include "hardware.h"
#include "max7456.h"

//Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

/*
 * X-macro generating write register addresses.
 * W_VM0 = 0x00, W_VM1 = 0x01, etc.
 */
#define X(addr, key, name, def, restore, stat, dmm) W_ ## key = addr & 0x7F,
typedef enum regw_t {
    REG_MAP_RW
} regw_t;
#undef X

/*
 * X-macro generating read register addresses.
 * R_VM0 = 0x80, R_VM1 = 0x81, etc.
 */
typedef enum regr_t {
#define X(addr, key, name, def, restore, stat, dmm) R_ ## key = addr,
    REG_MAP_RW
#undef X
#define X(addr, key, name) R_ ## key = addr,
    REG_MAP_R
#undef X
} regr_t;

#define W2R(reg) ((regr_t)((regw_t)(reg) | 0x80)) /* convert write to read */
#define R2W(reg) ((regw_t)((regr_t)(reg) & 0x7F)) /* convert read to write */

/* Register bit positions. */
#define STAT_DONTCARE 		(7)
#define STAT_RESET 		(6)
#define STAT_CHARMEM_UNAVAIL 	(5)
#define STAT_VSYNC_INACTIVE 	(4)
#define STAT_HSYNC_INACTIVE 	(3)
#define STAT_LOS 		(2)
#define STAT_NTSC 		(1)
#define STAT_PAL 		(0)
#define VM0_DONTCARE 		(7)
#define VM0_VIDEOSELECT_PAL	(6)
#define VM0_SYNCSELECT_NOAUTO 	(5)
#define VM0_SYNCSELECT_INTERNAL	(4)
#define VM0_ENABLE 		(3)
#define VM0_ENABLE_VSYNC	(2)
#define VM0_RESET 		(1)
#define VM0_VIDEOBUF_DISABLE 	(0)
#define DMM_DONTCARE 		(7)
#define DMM_MODE_8BIT		(6)
#define DMM_LBC_VIDEOIN 	(5)
#define DMM_BLINK		(4)
#define DMM_INVERT	 	(3)
#define DMM_CLEAR		(2)
#define DMM_CLEAR_VSYNC 	(1)
#define DMM_AUTOINCREMENT 	(0)
#define OSDBL_DONTCARE0 	(7)
#define OSDBL_DONTCARE1 	(6)
#define OSDBL_DONTCARE2 	(5)
#define OSDBL_DISABLE		(4)
#define OSDBL_FACTORY0 		(3)
#define OSDBL_FACTORY1 		(2)
#define OSDBL_FACTORY2 		(1)
#define OSDBL_FACTORY3 		(0)


/* X-macro generating default register values as per Max7456 manual */
#define X(addr, key, name, def, restore, stat, dmm) def,
regset_t DEFREGVALS = {
    { REG_MAP_RW }
};
#undef X

typedef struct regsave_t {		/* register save eeprom info */
    regw_t	reg;			/* writable register */
    bool	restore_from_eeprom;	/* whether to restore from eeprom */
} regsave_t, *regsave_p;

/* X-macro generating a lookup list of registers to save in eeprom */
#define X(addr, key, name, def, restore, stat, dmm) { W_ ## key, restore },
regsave_t REGSAVE[] = {
    REG_MAP_RW
};
#undef X

#define PALROWS (MAXROWS)
#define NTSCROWS (MAXROWS - 3)

/*
 * Shadow screenbuffer for Max7456 video memory. This buffer is filled by
 * command.cpp and is written to screen during Max7456's vertical
 * synchronisation (vsync).
 */
screenbuf_t 	screenbuf = { {0}, {0}, NTSCROWS, MAXCOLS, false};


/*------------------------------------------------------------------------
 *  Function	: reg_check_STAT_CHARMEM_UNAVAIL
 *  Purpose	: Check whether register write should wait for STAT register.
 *  Method	: Use X-macro that returns true for all involved registers.
 *
 *  Returns	: True if write has to wait, false otherwise.
 *------------------------------------------------------------------------
 */
static bool
reg_check_STAT_CHARMEM_UNAVAIL(
    regw_t 	reg)	/* register to write */
{
    return false
#define X(addr, key, name, def, restore, stat, dmm) \
	|| (reg == R2W(addr) && stat)
REG_MAP_RW
#undef X
   ;
} /* reg_check_STAT_CHARMEM_UNAVAIL() */


/*------------------------------------------------------------------------
 *  Function	: reg_check_DMM_CLEAR
 *  Purpose	: Check whether register write should wait for DMM register.
 *  Method	: Use X-macro that returns true for all involved registers.
 *
 *  Returns	: True if write has to wait, false otherwise.
 *------------------------------------------------------------------------
 */
static bool
reg_check_DMM_CLEAR(
    regw_t 	reg)	/* register to write */
{
    return false
#define X(addr, key, name, def, restore, stat, dmm) \
        || (reg == R2W(addr) && dmm)
REG_MAP_RW
#undef X
   ;
} /* reg_check_DMM_CLEAR() */


/*------------------------------------------------------------------------
 *  Function	: reg_read
 *  Purpose	: Read a register.
 *  Method	: Use SPI.
 *
 *  Returns	: The value read.
 *------------------------------------------------------------------------
 */
static uint8_t
reg_read(
    regr_t 	reg)	/* register to read */
{
    uint8_t val;	/* return value */

    val = SPI.transfer(reg);
    val = SPI.transfer(0xFF);
    return val;
} /* reg_read() */


/*------------------------------------------------------------------------
 *  Function	: reg_getbit
 *  Purpose	: Get register bit value.
 *  Method	: Read register and select bit.
 *
 *  Returns	: true if the bit is 1, false otherwise.
 *------------------------------------------------------------------------
 */
static bool
reg_getbit(
    regr_t 	reg,	/* register to read */
    uint8_t 	bitnr)	/* bit number to read [0 .. 7] */
{
    uint8_t val;	/* register value */

    val = reg_read(reg);
    return (val & (0x01 << bitnr)) != 0 ? true : false;
} /* reg_getbit() */


/*------------------------------------------------------------------------
 *  Function	: reg_write
 *  Purpose	: Write a register.
 *  Method	: Possibly wait for conditions to be met, use SPI.
 *
 *  As per Max7456 manual, some register writes are subject to any of these
 *  conditions:
 *  1) STAT[5] = 0, the character memory is not busy.
 *  2) DMM[2] = 0, the display memory is not in the process of being cleared.
 *  Therefore we check these conditions and wait for them to be met.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
static bool
reg_write(
    regw_t 	reg,	/* register to write */
    uint8_t 	value)	/* value to write */
{
#define MAXWAIT 100

    int i;		/* number of milliseconds we had to wait */

    if (reg_check_STAT_CHARMEM_UNAVAIL(reg)) {
	for (i = 0;
	     i < MAXWAIT && reg_getbit(R_STAT, STAT_CHARMEM_UNAVAIL);
	     i++) {
	    delay(1);
	}
	if (i >= MAXWAIT) return false;
    }
    if (reg_check_DMM_CLEAR(reg)) {
	for (i = 0;
	     i < MAXWAIT && reg_getbit(R_DMM, DMM_CLEAR);
	     i++) {
	    delay(1);
	}
	if (i >= MAXWAIT) return false;
    }
    SPI.transfer(reg);
    SPI.transfer(value);
    return true;
#undef MAXWAIT
} /* reg_write() */


/*------------------------------------------------------------------------
 *  Function	: reg_write_check
 *  Purpose	: Write a register and check its new value.
 *  Method	: Call reg_write() and reg_read().
 *
 *  Returns	: Whether the value written could be read back.
 *------------------------------------------------------------------------
 */
static bool
reg_write_check(
    regw_t 	reg,	/* register to write */
    uint8_t 	value)	/* value to write */
{
    if (!reg_write(reg, value)) return false;
    return reg_read(W2R(reg)) == value;
} /* reg_write_check() */


/*------------------------------------------------------------------------
 *  Function	: reg_newval
 *  Purpose	: Calculate new register value when setting single bit.
 *  Method	: Read (converted) register, mask with value.
 *
 *  Returns	: New calculated value.
 *------------------------------------------------------------------------
 */
static uint8_t
reg_newval(
    regw_t 	reg,	/* register to write */
    uint8_t 	bitnr,	/* bit number to write [0 .. 7] */
    bool 	value)	/* value to write; true => 1, false => 0 */
{
    uint8_t val;	/* new register value */

    val = reg_read(W2R(reg));	/* convert to read address */
    if (value) {
	val |= (0x01 << bitnr);
    } else {
	val &= ~(0x01 << bitnr);
    }
    return val;
} /* reg_newval() */


/*------------------------------------------------------------------------
 *  Function	: reg_setbit
 *  Purpose	: Set a register bit to 1 or 0.
 *  Method	: Calculate new value, write register.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
reg_setbit(
    regw_t 	reg,	/* register to write */
    uint8_t 	bitnr,	/* bit number to write [0 .. 7] */
    bool 	value)	/* value to write; true => 1, false => 0 */
{
    uint8_t val;	/* new register value */

    val = reg_newval(reg, bitnr, value);
    reg_write(reg, val);
} /* reg_setbit() */


/*------------------------------------------------------------------------
 *  Function	: reg_setbit_check
 *  Purpose	: Set a register bit to 1 or 0 and check its new value.
 *  Method	: Calculate new value, write register with check.
 *
 *  Returns	: Whether setting the bit succeeded.
 *------------------------------------------------------------------------
 */
static bool
reg_setbit_check(
    regw_t 	reg,	/* register to write */
    uint8_t 	bitnr,	/* bit number to write [0 .. 7] */
    bool 	value)	/* value to write; true => 1, false => 0 */
{
    uint8_t val;	/* register value */

    val = reg_newval(reg, bitnr, value);
    return reg_write_check(reg, val);
} /* reg_setbit_check() */


/*------------------------------------------------------------------------
 *  Function	: max_refreshscreen
 *  Purpose	: Write the shadow screenbuffer to screen.
 *  Method	: Use 16 bit-mode and no auto-increment.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_refreshscreen()
{
    uint16_t	 pos; 		/* screenbuf position */
    uint16_t	 highpos;	/* highest screen position + 1 */
    static bool	 busy;		/* whether we're still busy */

    if (!screenbuf.dirty || busy || digitalRead(MAX_SELECTPIN) == LOW) {
	/*
	 * No screen changes.
	 * Or we were interrupted by a new vsync.
	 * Or we're interrupting another Max7456 routine.
	 */
	return true;
    }
    digitalWrite(MAX_SELECTPIN, LOW);
    busy = true;
    screenbuf.dirty = false;

    /*
     * We choose not to use the auto-increment mode although there would be no
     * better example like this. But we'd have to give up char 0xFF which would
     * be unacceptable. Besides that, also auto-increment cannot be fast
     * enough to complete within one vsync. SPI seems to run on 8 MHz, so
     * writing 480 characters costs at least 480 microseconds. Measurements
     * show that the vsync pin is low for approximately 1% of the time. So for
     * PAL (50 hz vsync pulses) this is only 200 microseconds and for NTSC
     * (60 hz pulses) this is only 170 microseconds.
     *
     * Now the above is even worse when doubling the SPI data like we do here.
     * But we use a little trick that seems to hide screen update artefacts.
     * This even runs smoothly without using vsync at all.
     */
    reg_setbit(W_DMM, DMM_MODE_8BIT, false);   	/* 16 bit operation mode */
    reg_setbit(W_DMM, DMM_LBC_VIDEOIN, false); 	/* background is video in */
    reg_setbit(W_DMM, DMM_BLINK, false);   	/* blinking off */
    reg_setbit(W_DMM, DMM_INVERT, false);	/* invert off */
    reg_setbit(W_DMAH, 0, false);		/* MSB of display address */
    highpos = screenbuf.rows * screenbuf.cols;
    for (pos = 0; pos < highpos; pos++) {
	if (pos == 0x100) {
            reg_setbit(W_DMAH, 0, true);	/* MSB of display address */
	}
	if (SCREENDIRTY(screenbuf, pos)) {
	    reg_write(W_DMAL, pos % 0x100);	/* LSBs of display address */
	    reg_write(W_DMDI, screenbuf.buf[pos]);
	    SCREENUNSETDIRTY(screenbuf, pos);
	}
    }
    digitalWrite(MAX_SELECTPIN, HIGH);
    busy = false;
    return true;
} /* max_refreshscreen() */


/*------------------------------------------------------------------------
 *  Function	: max_vsync
 *  Purpose	: Vertical sync interrupt handler.
 *  Method	: Print the screen during vertical synchronisation.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
max_vsync(void)
{
    static unsigned long oldtime = millis();	/* old timestamp */
    unsigned long thistime;			/* current timestamp */

    sei();	/* enable other interrupts */
    if ((thistime = millis()) - oldtime > cfg_get_refresh()) {
	oldtime = thistime;
        max_refreshscreen();
    }
} /* max_vsync() */


/*------------------------------------------------------------------------
 *  Function	: max_regadj
 *  Purpose	: Adjust register (sub)value.
 *  Method	: Binary shift the value out, update & check, shift in.
 *
 *  Returns	: The new (sub)value on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
static int16_t
max_regadj(
    regw_t	reg,		/* register to adjust */
    int16_t	num,		/* number to inc/dec */
    uint8_t 	minval,		/* minimum value to set */
    uint8_t 	maxval,		/* maximum value to set */
    uint8_t 	minbit,		/* minimum register bit that holds value */
    uint8_t 	maxbit,		/* maximum register bit that holds value */
    bool	setselectpin)	/* whether to set select pin low/high */
{
    uint8_t 	val1;	/* register value */
    uint8_t 	val2;	/* register value */
    int16_t	val;	/* new value before validation */
    uint8_t	mask;	/* 8 bit mask */
    uint8_t	bit;	/* bit number in byte value [0 .. 7] */
    int16_t	result;	/* return value */

    if (setselectpin) digitalWrite(MAX_SELECTPIN, LOW);
    val1 = val2 = reg_read(W2R(reg));
    mask = 0x00;
    for (bit = minbit; bit <= maxbit; bit++) {
	mask |= (0x01 << bit);
    }
    val1 &= mask;
    val1 >>= minbit;
    val = val1 + num;
    if (val < minval) val = minval;
    if (val > maxval) val = maxval;
    val1 = result = val;
    if (num != 0) {
	val1 <<= minbit;
	mask = ~mask;
	val2 &= mask;
	val1 |= val2;
	if (!reg_write_check(reg, val1)) result = -1;
    }
    if (setselectpin) digitalWrite(MAX_SELECTPIN, HIGH);
    return result;
} /* max_regadj() */


/*------------------------------------------------------------------------
 *  Function	: max_cbwl
 *  Purpose	: Adjust character black/white level.
 *  Method	: Call max_regadj() to adjust the Row N Brightness
 *  		  register(s) (RB0-RB15).
 *
 *  Returns	: New (highest) level on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
static int8_t
max_cbwl(
    int8_t 	line,	/* line number, -1 means all */
    int8_t 	num,	/* number to inc/dec */
    uint8_t 	minval,	/* minimum value to set */
    uint8_t 	maxval,	/* maximum value to set */
    uint8_t 	minbit,	/* minimum register bit that holds value */
    uint8_t 	maxbit)	/* maximum register bit that holds value */
{
    regw_t	reg;	/* register to update */
    int8_t 	val;	/* register value */
    int8_t 	res;	/* return value */
    uint8_t 	min;	/* min line number */
    uint8_t 	max;	/* max line number */

    if (line < 0) {
	min = 0;
	max = 15;
    } else {
	min = line;
	max = line;
    }
    if (line > 15) {
	return -1;
    }
    res = 0;
    digitalWrite(MAX_SELECTPIN, LOW);
    for (int i = min; i <= max; i++) {
	switch (i) {
	    case 0: reg = W_RB0; break;
	    case 1: reg = W_RB1; break;
	    case 2: reg = W_RB2; break;
	    case 3: reg = W_RB3; break;
	    case 4: reg = W_RB4; break;
	    case 5: reg = W_RB5; break;
	    case 6: reg = W_RB6; break;
	    case 7: reg = W_RB7; break;
	    case 8: reg = W_RB8; break;
	    case 9: reg = W_RB9; break;
	    case 10: reg = W_RB10; break;
	    case 11: reg = W_RB11; break;
	    case 12: reg = W_RB12; break;
	    case 13: reg = W_RB13; break;
	    case 14: reg = W_RB14; break;
	    case 15: reg = W_RB15; break;
	    default: res = -1;
	}
	if (res >= 0) {
	    if ((val = max_regadj(reg, num, minval, maxval, minbit, maxbit,
			          false)) < 0) {
		res = -1;			/* error */
	    } else {
		if (val > res) res = val;	/* select highest */
	    }
	}
    }
    digitalWrite(MAX_SELECTPIN, HIGH);
    return res;
} /* max_cbwl() */


/*------------------------------------------------------------------------
 *  Function	: max_setup
 *  Purpose	: Setup the max7456 chip.
 *  Method	: Set pin modes, set interrupt handlers, reset.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_setup(
    bool 	init)	/* whether called by initial setup */
{
    pinMode(MAX_RESETPIN, OUTPUT);
    pinMode(MAX_SELECTPIN, OUTPUT);
    pinMode(MAX_MOSIPIN, OUTPUT);
    pinMode(MAX_MISOPIN, INPUT);
    pinMode(MAX_SCKPIN, OUTPUT);
    pinMode(MAX_VSYNCPIN, INPUT);
    digitalWrite(MAX_VSYNCPIN, HIGH);		/* use pullup resistor */
    attachInterrupt(INT0, max_vsync, FALLING);
    delay(75);	/* give time to hard reset before doing SPI operations */
    digitalWrite(MAX_SELECTPIN, LOW);
    /* set automatic black level control; needed after hard reset (POR) */
    reg_setbit(W_OSDBL, OSDBL_DISABLE, false);
    digitalWrite(MAX_SELECTPIN, HIGH);
    return max_reset(init);
} /* max_setup() */


/*------------------------------------------------------------------------
 *  Function	: max_reset
 *  Purpose	: Reset the Max7456 chip.
 *  Method	: Use software reset, enable display.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_reset(
    bool 	init)	/* whether called by initial setup */
{
    bool 	result = true;	/* return value */
    bool 	videomode;	/* current video mode PAL/NTSC */

    digitalWrite(MAX_SELECTPIN, LOW);
    videomode = reg_getbit(R_VM0, VM0_VIDEOSELECT_PAL);
    reg_setbit(W_VM0, VM0_RESET, true);
    do {
	delay(1);	/* typical should be 0.1 ms, wait a little longer */
    } while (reg_getbit(R_STAT, STAT_RESET));
    digitalWrite(MAX_SELECTPIN, HIGH);

    /*
     * Restore the enable flag. For some unknown reason, we need to
     * deselect/reselect again. So this will not work while keeping the
     * selectpin LOW.
     */
    result = max_enable(cfg_get_enable()); /* also done in main loop() */

    /* Restore PAL/NTSC video mode bit. */
    digitalWrite(MAX_SELECTPIN, LOW);
    if (reg_getbit(R_VM0, VM0_VIDEOSELECT_PAL) != videomode) {
        reg_setbit(W_VM0, VM0_VIDEOSELECT_PAL, videomode);
    }
    digitalWrite(MAX_SELECTPIN, HIGH);

    /* Refresh screen. */
    for (uint16_t pos = 0; pos < MAXSCRSIZE; pos++) {
	SCREENSETDIRTY(screenbuf, pos);
    }
    screenbuf.dirty = true;
    return result;
} /* max_reset() */


/*------------------------------------------------------------------------
 *  Function	: max_enable
 *  Purpose	: Enable/disable the OSD image.
 *  Method	: Set/reset the enable flag of register VM0.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_enable(
    bool 	enable)		/* whether to enable */
{
    bool 	result = true;	/* return value */

    digitalWrite(MAX_SELECTPIN, LOW);
    if (reg_getbit(R_VM0, VM0_ENABLE) != enable) {
#ifndef NO_DEBUG
	if (!cfg_get_silent() && cfg_get_debug()) {
	    Serial.println(F("<Restore VM0 register>"));
	}
#endif
	if (!reg_setbit_check(W_VM0, VM0_ENABLE, enable)) {
	    result = false;
	}
    }
    digitalWrite(MAX_SELECTPIN, HIGH);
    return result;
} /* max_enable() */


/*------------------------------------------------------------------------
 *  Function	: max_regsetget
 *  Purpose	: Get a register set to store (possibly to eeprom).
 *  Method	: Read registers.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_regsetget(
    regset_t 	*regset)	/* registerset to get */
{
    if (regset == NULL) return false;
    digitalWrite(MAX_SELECTPIN, LOW);
    for (uint8_t i = 0; i < REGW_COUNT; i++) {
        regset->values[i] = reg_read(W2R(REGSAVE[i].reg));
    }
    digitalWrite(MAX_SELECTPIN, HIGH);
    return true;
} /* max_regsetget() */


/*------------------------------------------------------------------------
 *  Function	: max_regsetput
 *  Purpose	: Restore register set (possibly from eeprom).
 *  Method	: Write registers.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_regsetput(
    regset_t 	*regset)	/* registerset to put */
{
    bool ok = true;	/* whether to proceed */

    if (regset == NULL) return false;
    digitalWrite(MAX_SELECTPIN, LOW);
    for (uint8_t i = 0; ok && i < REGW_COUNT; i++) {
	if (REGSAVE[i].restore_from_eeprom) {
            ok = ok && reg_write_check(REGSAVE[i].reg, regset->values[i]);
	}
    }
    digitalWrite(MAX_SELECTPIN, HIGH);
    return ok;
} /* max_regsetput() */


/*------------------------------------------------------------------------
 *  Function	: max_fontcharget
 *  Purpose	: Read font character from EEPROM.
 *  Method	:
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_fontcharget(
    uint8_t	num,	/* number of character to get [0..255] */
    fontbuf_t 	*buf)	/* font character buffer */
{
    digitalWrite(MAX_SELECTPIN, LOW);
    reg_setbit(W_VM0, VM0_ENABLE, false);
    reg_write(W_CMAH, num);
    reg_write(W_CMM, 0x5F);

    /*
     * Note that because buf is a pointer to fontbuf_t data, sizeof(*buf) gives
     * the correct number of bytes. sizeof(buf) with a fontbuf_t typed buf
     * would return the size of a pointer. This is only true when passed as a
     * function parameter like here.
     */
    for (uint16_t i = 0; i < sizeof(*buf); i++) {
	reg_write(W_CMAL, (uint8_t)i);
	(*buf)[i] = reg_read(R_CMDO);
    }
    reg_setbit(W_VM0, VM0_ENABLE, true);
    digitalWrite(MAX_SELECTPIN, HIGH);
    return true;
} /* max_fontcharget() */


/*------------------------------------------------------------------------
 *  Function	: max_fontcharput
 *  Purpose	: Save font character into EEPROM.
 *  Method	: 
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
max_fontcharput(
    uint8_t	num,	/* number of character to put [0 .. 255] */
    fontbuf_t 	*buf)	/* font character buffer */
{
    digitalWrite(MAX_SELECTPIN, LOW);
    reg_setbit(W_VM0, VM0_ENABLE, false);
    reg_write(W_CMAH, num);

    /*
     * Note that because buf is a pointer to fontbuf_t data, sizeof(*buf) gives
     * the correct number of bytes. sizeof(buf) with a fontbuf_t typed buf
     * would return the size of a pointer. This is only true when passed as a
     * function parameter like here.
     */
    for (uint16_t i = 0; i < sizeof(*buf); i++) {
	reg_write(W_CMAL, (uint8_t)i);
	reg_write(W_CMDI, (*buf)[i]);
    }
    reg_write(W_CMM, 0xAF);
    reg_setbit(W_VM0, VM0_ENABLE, true);
    digitalWrite(MAX_SELECTPIN, HIGH);
    return true;
} /* max_fontcharput() */


/*------------------------------------------------------------------------
 *  Function	: max_videodetect
 *  Purpose	: Detect PAL or NTSC video input, and loss-of-sync.
 *  Method	: Keep track of last known values.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
max_videodetect(void)
{
    static bool pal_detected = false;	/* whether PAL signal detected */
    static bool ntsc_detected = false;	/* whether NTSC signal detected */
    static bool los_detected = false;   /* whether loss-of-sync detected */

    /*
     * There's no need to refresh the screen when switching between PAL and
     * NTSC as both have the same number of screen columns and the
     * screenbuffer is organised row wise. So lines will not wrap, only
     * NTSC will have three lines less.
     */
    digitalWrite(MAX_SELECTPIN, LOW);
    if (pal_detected != reg_getbit(R_STAT, STAT_PAL)) {
	if ((pal_detected = !pal_detected)) {
	    reg_setbit(W_VM0, VM0_VIDEOSELECT_PAL, true);
	    screenbuf.rows = PALROWS;
#ifndef NO_DEBUG
            if (!cfg_get_silent() && cfg_get_debug()) {
                Serial.println(F("<PAL detected>"));
	    }
#endif
	}
    }
    if (ntsc_detected != reg_getbit(R_STAT, STAT_NTSC)) {
	if ((ntsc_detected = !ntsc_detected)) {
	    reg_setbit(W_VM0, VM0_VIDEOSELECT_PAL, false);
	    screenbuf.rows = NTSCROWS;
#ifndef NO_DEBUG
            if (!cfg_get_silent() && cfg_get_debug()) {
	        Serial.println(F("<NTSC detected>"));
	    }
#endif
	}
    }
    if (los_detected != reg_getbit(R_STAT, STAT_LOS)) {
	if ((los_detected = !los_detected)) {
#ifndef NO_DEBUG
            if (!cfg_get_silent() && cfg_get_debug()) {
	        Serial.println(F("<Loss-of-sync detected>"));
	    }
#endif
	}
    }
    digitalWrite(MAX_SELECTPIN, HIGH);
} /* max_videodetect() */


/*------------------------------------------------------------------------
 *  Function	: max_hos
 *  Purpose	: Shift OSD text num pixels left (num < 0) or right (num > 0).
 *  Method	: Call max_regadj() to adjust the Horizontal OffSet
 *  		  register (HOS).
 *
 *  Returns	: New value on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
int8_t
max_hos(
    int16_t 	num)	/* number of pixels to shift left or right */
{
/*
 * 0x00 corresponds with -32 pixels.
 * 0x3F corresponds with +15 pixels.
 */
    return (int8_t)max_regadj(W_HOS, num, 0x00, 0x3F, 0, 7, true);
} /* max_hos() */


/*------------------------------------------------------------------------
 *  Function	: max_vos
 *  Purpose	: Shift OSD text num pixels up (num < 0) or down (num > 0).
 *  Method	: Call max_regadj() to adjust the Vertical OffSet
 *  		  register (HOS).
 *
 *  Returns	: New value on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
int8_t
max_vos(
    int16_t 	num)	/* number of pixels to shift down or up */
{
/*
 * 0x00 corresponds with -16 pixels.
 * 0x1F corresponds with +15 pixels.
 */
    return (int8_t)max_regadj(W_VOS, num, 0x00, 0x1F, 0, 7, true);
} /* max_vos() */


/*------------------------------------------------------------------------
 *  Function	: max_insmux1
 *  Purpose	: Adjust OSD Rise and Fall Time - typical transition times
 *                between adjacent OSD pixels.
 *  Method	: Call max_regadj() to adjust the OSD insertion Mux
 *  		  register (OSDM).
 *
 *  Returns	: New value on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
int8_t
max_insmux1(
    int16_t 	num)	/* number to inc/dec */
{
/*
 * num < 0 improves sharpness.
 * num > 0 improves (minimizes) cross-colour artefacts.
 *
 * Values in bits 5,4,3 of OSDM register:
 * 000: 20ns (maximum sharpness/maximum cross-colour artefacts )
 * 001: 30ns
 * 010: 35ns
 * 011: 60ns
 * 100: 80ns
 * 101: 110ns (minimum sharpness/minimum cross-colour artefacts)
 */
    return max_regadj(W_OSDM, num, 0x00, 0x05, 3, 5, true);
} /* max_insmux1() */


/*------------------------------------------------------------------------
 *  Function	: max_insmux2
 *  Purpose	: Adjust OSD Insertion Mux Switching Time – typical transition
 *                times between input video and OSD pixels.
 *  Method	: Call max_regadj() to adjust the OSD insertion Mux
 *  		  register (OSDM).
 *
 *  Returns	: New value on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
int8_t
max_insmux2(
    int16_t 	num)	/* number to inc/dec */
{
/*
 * num < 0 improves sharpness.
 * num > 0 improves (minimizes) cross-colour artefacts.
 *
 * Values in bits 2,1,0 of OSDM register:
 * 000: 30ns (maximum sharpness/maximum cross-colour artefacts )
 * 001: 35ns
 * 010: 50ns
 * 011: 75ns
 * 100: 100ns
 * 101: 120ns (minimum sharpness/minimum cross-colour artefacts)
 */
    return max_regadj(W_OSDM, num, 0x00, 0x05, 0, 2, true);
} /* max_insmux2() */


/*------------------------------------------------------------------------
 *  Function	: max_cbl
 *  Purpose	: Adjust Character Black Level.
 *  Method	: Call max_cbwl().
 *
 *  Returns	: New value on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
int8_t
max_cbl(
    int8_t 	line,	/* line number, -1 means all */
    int16_t 	num)	/* number to inc/dec */
{
/*
 * Character Black Level. All the characters in row N use these brightness
 * levels for the black pixel, in % of OSD white level.
 *
 * Values in bits 3,2 of RBx register:
 * 00 = 0%
 * 01 = 10%
 * 10 = 20%
 * 11 = 30%
 */
    return max_cbwl(line, num, 0x00, 0x03, 2, 3);
} /* max_cbl() */


/*------------------------------------------------------------------------
 *  Function	: max_cwl
 *  Purpose	: Adjust Character White Level.
 *  Method	: Call max_cbwl().
 *
 *  Returns	: New value on success, -1 otherwise.
 *------------------------------------------------------------------------
 */
int8_t
max_cwl(
    int8_t 	line,	/* line number, -1 means all */
    int16_t 	num)	/* number to inc/dec */
{
/*
 * Character White Level. All the characters in row N use these brightness
 * levels for the white pixel, in % of OSD white level.
 *
 * Values in bits 1,0 of RBx register:
 * 00 = 120%
 * 01 = 100%
 * 10 = 90%
 * 11 = 80%
 */
    return max_cbwl(line, num, 0x00, 0x03, 0, 1);
} /* max_cwl() */
