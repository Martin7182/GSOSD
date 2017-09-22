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
 *  File	: config.h
 *  Purpose	: Declarations of config functions.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/06/27
 *
 *========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdbool.h>
#include "max7456.h"

/*
 * The version string is only used to force an eeprom reset when it changes.
 * So if there is a mismatch between software and eeprom, defaults will be
 * loaded. See cfg_load() for details.
 */
typedef char str16_t[16];
typedef char str5_t[5];
typedef const char *str_t;
extern str16_t VERSION;

#define CFG_SIZE (16)	/* Maximum size for name in bytes */

/* A list of all configurable parameters */
/* key		name	   var/functype		stype		default	     */
#define CONFIG_TABLE0 							     \
X(CFG_MAXREGS, 	"MAXREGS", maxregs,  regset_t,	regset_t,	DEFREGVALS)  \

/* key		name	   var/func  type	stype		default	     */
#define CONFIG_TABLE1 							     \
X(CFG_VERSION,	"VERSION", version,  str_t,	str16_t,	VERSION)     \
X(CFG_BAUDRATE,	"BAUDRATE",baudrate, uint32_t, 	uint32_t,	9600)	     \
X(CFG_TIMEOUT, 	"TIMEOUT", timeout,  uint32_t,	uint32_t,       10000)	     \
X(CFG_VDETECT,	"VDETECT", vdetect,  uint32_t,	uint32_t,       1000)	     \
X(CFG_REFRESH,	"REFRESH", refresh,  uint32_t,	uint32_t,       100)	     \
X(CFG_LOGO,	"LOGO",    logo,     uint32_t,	uint32_t,       2000)	     \
X(CFG_ENABLE,	"ENABLE",  enable,   bool,	bool,           true)	     \
X(CFG_DEBUG, 	"DEBUG",   debug,    bool,	bool,   	false)	     \
X(CFG_ECHO, 	"ECHO",    echo,     bool,	bool,   	true)	     \
X(CFG_SILENT, 	"SILENT",  silent,   bool,	bool,   	false)	     \
X(CFG_CONTROL, 	"CONTROL", control,  uint8_t,	uint8_t,   	0x03)	     \
X(CFG_SENSADJ0,	"SENSADJ0",sensadj0, uint16_t,	uint16_t,       20000)	     \
X(CFG_SENSADJ1,	"SENSADJ1",sensadj1, uint16_t,	uint16_t,       20000)	     \
X(CFG_SENSADJ2,	"SENSADJ2",sensadj2, uint16_t,	uint16_t,       20000)	     \
X(CFG_SENSADJ3,	"SENSADJ3",sensadj3, uint16_t,	uint16_t,       20000)	     \

/* Following values are for standalone build only. */
#define CONFIG_TABLE2							     \
X(CFG_SENSV0, 	"SENSV0",  sensv0,   bool,	bool,	  	true)	     \
X(CFG_SENSX0,	"SENSX0",  sensx0,   uint8_t,	uint8_t,  	1)	     \
X(CFG_SENSY0,	"SENSY0",  sensy0,   uint8_t,	uint8_t,  	1)	     \
X(CFG_SENST0,	"SENST0",  senst0,   str_t,	str5_t,  	"V")	     \
X(CFG_SENSV1, 	"SENSV1",  sensv1,   bool,	bool,	  	true)	     \
X(CFG_SENSX1,	"SENSX1",  sensx1,   uint8_t,	uint8_t,  	1)	     \
X(CFG_SENSY1,	"SENSY1",  sensy1,   uint8_t,	uint8_t,  	2)	     \
X(CFG_SENST1,	"SENST1",  senst1,   str_t,	str5_t,  	"V")	     \
X(CFG_SENSV2, 	"SENSV2",  sensv2,   bool,	bool,	  	true)	     \
X(CFG_SENSX2,	"SENSX2",  sensx2,   uint8_t,	uint8_t,  	1)	     \
X(CFG_SENSY2,	"SENSY2",  sensy2,   uint8_t,	uint8_t,  	3)	     \
X(CFG_SENST2,	"SENST2",  senst2,   str_t,	str5_t,  	"V")	     \
X(CFG_SENSV3, 	"SENSV3",  sensv3,   bool,	bool,	  	true)	     \
X(CFG_SENSX3,	"SENSX3",  sensx3,   uint8_t,	uint8_t,  	1)	     \
X(CFG_SENSY3,	"SENSY3",  sensy3,   uint8_t,	uint8_t,  	4)	     \
X(CFG_SENST3,	"SENST3",  senst3,   str_t,	str5_t,		"V")	     \
X(CFG_TIMEV, 	"TIMEV",   timev,    bool,	bool,	  	true)	     \
X(CFG_TIMEX,	"TIMEX",   timex,    uint8_t,	uint8_t,  	1)	     \
X(CFG_TIMEY,	"TIMEY",   timey,    uint8_t,	uint8_t,  	5)	     \
X(CFG_TIMET,	"TIMET",   timet,    str_t,	str5_t,  	"")	     \

#define CONFIG_TABLE CONFIG_TABLE0 CONFIG_TABLE1 CONFIG_TABLE2

/* X-macro generating config enums. */
#define X(key, name, var, type, stype, def) key,
typedef enum config_t {
    CONFIG_TABLE
    CONFIG_COUNT
} config_t;
#undef X

/* X-macro generating config struct. */
#define X(key, name, var, type, stype, def) stype var;
typedef struct configdata_t {
    CONFIG_TABLE
} configdata_t;
#undef X

/* X-macros generating function prototypes. */
#define X(key, name, func, type, stype, def) bool cfg_set_ ## func(type);
CONFIG_TABLE
#undef X

#define X(key, name, func, type, stype, def) type cfg_get_ ## func(void);
CONFIG_TABLE
#undef X

extern const char * const CFG_NAMES[];
extern const uint16_t CFG_SIZES[];

void cfg_load_defaults(void);

bool cfg_load(bool);

bool cfg_save(bool);

bool cfg_dump(void);

#endif /* CONFIG_H */
