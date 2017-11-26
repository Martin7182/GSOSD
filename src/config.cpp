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
 *  File	: config.cpp
 *  Purpose	: Config functions handling.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/06/27
 *
 *========================================================================
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>
#include "max7456.h"
#include "globals.h"
#include "config.h"

//Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

/* X-macro generating config names array. */
#define X(key, name, var, type, stype, def) \
    const char s_ ## var[] PROGMEM = name;
    CONFIG_TABLE
#undef X
#define X(key, name, var, type, stype, def) s_ ## var,
const char * const CFG_NAMES[] PROGMEM = {
    CONFIG_TABLE
};
#undef X

/* X-macro generating config sizes array. */
#define X(key, name, var, type, stype, def) sizeof(stype),
const uint16_t CFG_SIZES[] PROGMEM = {
    CONFIG_TABLE
};
#undef X

str16_t 	VERSION = "GSOSD 1.2.0";	/* current version */
configdata_t 	configdata;			/* configurable parameters */

/* X-macro generating local prototypes. */
#define X(key, name, funcvar, type, stype, def)	\
static void store_ ## stype(void *, const void *);
CONFIG_TABLE
#undef X

/* X-macro generating setters. */
#define X(key, name, funcvar, type, stype, def)	\
bool cfg_set_ ## funcvar(type val)		\
{						\
    store_ ## stype(&configdata.funcvar, &val);	\
    return true;				\
}
CONFIG_TABLE
#undef X

/* X-macro generating getters. */
#define X(key, name, funcvar, type, stype, def)	\
type cfg_get_ ## funcvar(void)			\
{						\
    return configdata.funcvar;			\
}
CONFIG_TABLE
#undef X


/*------------------------------------------------------------------------
 *  Function	: store_regset_t
 *  Purpose	: Setter for regset_t-typed values.
 *  Method	: Typecast & dereference void pointers.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
store_regset_t(
    void 	*param,	/* parameter to store */
    const void 	*val)	/* value to store */
{
    *(regset_t *)param = *(regset_t *)val;
} /* store_regset_t() */


/*------------------------------------------------------------------------
 *  Function	: store_str16_t
 *  Purpose	: Setter for str16_t-typed values.
 *  Method	: Typecast & dereference void pointers.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
store_str16_t(
    void 	*param,	/* parameter to store */
    const void 	*val)	/* value to store */
{
   strncpy((char *)param, *(const char **)val, 15);
   ((char *)param)[15] = '\0';
} /* store_str16_t() */


/*------------------------------------------------------------------------
 *  Function	: store_str5_t
 *  Purpose	: Setter for str5_t-typed values.
 *  Method	: Typecast & dereference void pointers.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
store_str5_t(
    void 	*param,	/* parameter to store */
    const void 	*val)	/* value to store */
{
   strncpy((char *)param, *(const char **)val, 4);
   ((char *)param)[4] = '\0';
} /* store_str5t() */


/*------------------------------------------------------------------------
 *  Function	: store_uint8_t
 *  Purpose	: Setter for uint8_t-typed values.
 *  Method	: Typecast & dereference void pointers.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
store_uint8_t(
    void 	*param,	/* parameter to store */
    const void 	*val)	/* value to store */
{
   *(uint8_t *)param = *(uint8_t *)val;
} /* store_uint8_t() */


/*------------------------------------------------------------------------
 *  Function	: store_uint16_t
 *  Purpose	: Setter for uint16_t-typed values.
 *  Method	: Typecast & dereference void pointers.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
store_uint16_t(
    void 	*param,	/* parameter to store */
    const void 	*val)	/* value to store */
{
   *(uint16_t *)param = *(uint16_t *)val;
} /* store_uint16_t() */


/*------------------------------------------------------------------------
 *  Function	: store_uint32_t
 *  Purpose	: Setter for uint32_t-typed values.
 *  Method	: Typecast & dereference void pointers.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
store_uint32_t(
    void 	*param,	/* parameter to store */
    const void 	*val)	/* value to store */
{
   *(uint32_t *)param = *(uint32_t *)val;
} /* store_uint32_t() */


/*------------------------------------------------------------------------
 *  Function	: store_bool
 *  Purpose	: Setter for bool-typed values.
 *  Method	: Typecast & dereference void pointers.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
static void
store_bool(
    void 	*param,	/* parameter to store */
    const void 	*val)	/* value to store */
{
   *(bool *)param = *(bool *)val != 0;
} /* store_bool() */


/*------------------------------------------------------------------------
 *  Function	: cfg_load_defaults
 *  Purpose	: Load hardcoded default configuration.
 *  Method	: Use preprocessed X-macro.
 *
 *  Returns	: Nothing.
 *------------------------------------------------------------------------
 */
void
cfg_load_defaults(void)
{
#define X(key, name, funcvar, type, stype, def)	\
    cfg_set_ ## funcvar(def);
CONFIG_TABLE
#undef X
}


/*------------------------------------------------------------------------
 *  Function	: cfg_load
 *  Purpose	: Load configuration from eeprom.
 *  Method	: Read all configuration from eeprom.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cfg_load(
    bool 	init)	/* whether called by initial setup */
{
    /* if configdata contains pad bytes, these will be read too */
    for (uint16_t i = 0; i < sizeof(configdata); i++) {
        ((uint8_t *)&configdata)[i] = EEPROM.read(i);
    }
    if (init && (strcmp(configdata.version, VERSION) != 0)) {
	cfg_load_defaults();
    }
    return max_regsetput(&configdata.maxregs);
} /* cfg_load() */


/*------------------------------------------------------------------------
 *  Function	: cfg_save
 *  Purpose	: Save configuration to eeprom.
 *  Method	: Write all changed configuration to eeprom.
 *
 *  The eeprom has an expected lifespan of approx. 100.000 write cycles.
 *  We try to protect against abuse and bugs by a delay after each write.
 *  100ms will require several hours to get the chip wrecked. But instead
 *  we use an increasing delay that starts at zero:
 *
 *  First 15 writes: 0ms
 *  Next 16 writes: 1ms
 *  Next 16 writes: 2ms
 *  ...
 *  100.000-th write: 6250ms
 *
 *  This will wreck it only after hours if not days. A 'workaround' is to
 *  replug the power periodically. But this will wear out the power button
 *  faster than the eeprom :-)
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cfg_save(
    bool 	init)	/* whether called by initial setup */
{
    bool 		ok = true;	/* whether to proceed */
    uint8_t		val;		/* eeprom value */
    static uint32_t	count = 0;	/* increasing delay to prevent wear */

    (void)init;
    ok = ok && max_regsetget(&configdata.maxregs);
    /* if configdata contains pad bytes, these will be written too */
    for (uint16_t i = 0; ok && i < sizeof(configdata); i++) {
        val = ((uint8_t *)&configdata)[i];
        if (val != EEPROM.read(i)) {	/* prevent unnecessary wear */
            EEPROM.write(i, val);
	    delay(++count >> 4);	/* mitigate errors and abuse */
	}
    }
    return ok;
} /* cfg_save() */


/*------------------------------------------------------------------------
 *  Function	: cfg_dump
 *  Purpose	: Dump configuration to serial output.
 *  Method	: Walk all configurable parameters.
 *
 *  Returns	: Indication of success.
 *------------------------------------------------------------------------
 */
bool
cfg_dump(void)
{
    bool 	ok = true;	/* whether to proceed */
    uint8_t	val;		/* eeprom value */
    uint16_t	i;		/* loop counter */
    char 	buf[CFG_SIZE];	/* buffer for PROGMEM strings */
    bool 	stx = false; 	/* whether STX printed */

    if (!cfg_get_silent() && (cfg_get_control() & 0x01) != 0x00) {
        Serial.write((byte)CONTROL_STX);
	stx = true;
    }
    ok = ok && max_regsetget(&configdata.maxregs);
    if (ok) {
	Serial.print("Total size: ");
	Serial.println(sizeof(configdata));
    }
    for (i = 0; ok && i < CONFIG_COUNT; i++) {
	strlcpy_P(buf, (char *)pgm_read_word(&(CFG_NAMES[i])), sizeof(buf));
	Serial.print(buf);
	Serial.print(" ");
	Serial.print(pgm_read_word_near(CFG_SIZES + i));
	Serial.println();
    }
    for (i = 0; ok && i < sizeof(configdata); i++) {
	if (i > 0) {
	    if (i % 16 == 0) {
		Serial.println();
	    } else {
		Serial.print(" ");
	    }
	}
        val = ((uint8_t *)&configdata)[i];
	if (val < 0x10) Serial.write('0');
	Serial.print(val, HEX);
    }
    if (stx) {
        Serial.write((byte)CONTROL_ETX);
    }
    return ok;
} /* cfg_dump() */
