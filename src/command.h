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
 *  File	: command.h
 *  Purpose	: Declarations of API commands.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/05/26
 *
 *========================================================================
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "config.h"

#define CMD_SIZE (16)	/* Maximum size for name in bytes */

/* A list of all commands */
/* key			name		function	nargs	data	*/
#define COMMAND_TABLE 							\
X(CMD_ABOUT, 		"ABOUT",	cmd_about, 	0, 	false)	\
X(CMD_LIST, 		"LIST",		cmd_list, 	0, 	false)	\
X(CMD_RESET, 		"RESET",	cmd_reset, 	0, 	false)	\
X(CMD_CLEARPART, 	"CLEARPART",	cmd_clearpart,	4, 	false)	\
X(CMD_CLEAR, 		"CLEAR",	cmd_clear,	0, 	false)	\
X(CMD_DUMP, 		"DUMP",		cmd_dump,	0, 	false)	\
X(CMD_LOAD, 		"LOAD",		cmd_load,	0, 	false)	\
X(CMD_DEFAULTS,		"DEFAULTS",	cmd_defaults,	0, 	false)	\
X(CMD_SAVE, 		"SAVE",		cmd_save,	0, 	false)	\
X(CMD_HOS,		"HOS",		cmd_hos,	1, 	false)	\
X(CMD_VOS,		"VOS",		cmd_vos,	1, 	false)	\
X(CMD_INSMUX1,		"INSMUX1",	cmd_insmux1,	1, 	false)	\
X(CMD_INSMUX2,		"INSMUX2",	cmd_insmux2,	1, 	false)	\
X(CMD_CBL,		"CBL",		cmd_cbl,	2, 	false)	\
X(CMD_CWL,		"CWL",		cmd_cwl,	2, 	false)	\
X(CMD_P_RAW, 		"P_RAW",	cmd_p_raw,	2, 	true) 	\
X(CMD_P_WINDOW,		"P_WINDOW",	cmd_p_window,	4, 	true) 	\
X(CMD_P_BANNER,		"P_BANNER",	cmd_p_banner,	4, 	true) 	\
X(CMD_SET_FONT, 	"SET_FONT",	cmd_set_font,	1, 	true)	\
X(CMD_GET_FONT, 	"GET_FONT",	cmd_get_font,	1, 	false)	\
X(CMD_GET_SENSOR, 	"GET_SENSOR",	cmd_get_sensor,	1, 	false)	\
X(CMD_GET_WIDTH, 	"GET_WIDTH",	cmd_get_width,	0, 	false)	\
X(CMD_GET_HEIGHT, 	"GET_HEIGHT",	cmd_get_height,	0, 	false)	\

/* X-macros generating function prototypes. */
#define X(key, name, func, nargs, data)	\
bool func(int32_t *, ...);
COMMAND_TABLE
#undef X

#define X(key, name, func, type, def, set)	\
bool cmd_cfg_set_ ## func(int32_t *, ...);
CONFIG_TABLE1
#undef X

#define X(key, name, func, type, def, set)	\
bool cmd_cfg_get_ ## func(int32_t *, ...);
CONFIG_TABLE1
#undef X


/* X-macro generating command enums. */
typedef enum command_t {
#define X(key, name, func, nargs, data) key,
    COMMAND_TABLE
#undef X
#define X(key, name, func, type, def, set) CMD_SET_ ## key, CMD_GET_ ## key,
    CONFIG_TABLE1
#undef X
    COMMAND_COUNT
} command_t;

typedef struct cmdprops_t {		/* command properties */
    bool 	(*command)(		/* command pointer */
			int32_t *,	/* integer arguments */
			...);		/* data arguments */
    uint8_t	argc;			/* fixed argument count */
    bool	use_data;		/* whether to use data */
} cmdprops_t, *cmdprops_p;

extern cmdprops_t CMD_PROPS[];
extern const char * const CMD_NAMES[];

#endif /* COMMAND_H */
