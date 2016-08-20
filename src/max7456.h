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
 *  File	: max7456.h
 *  Purpose	: Declarations for Max7456 functionality.
 *  Author(s)	: Martin7182
 *  Creation	: 2016/06/03
 *
 *========================================================================
 */

#ifndef MAX7456_H
#define MAX7456_H

#include <stdbool.h>

#define FONTBUF_SIZE 	(54)		/* font buffer size in bytes */

typedef char fontbuf_t[FONTBUF_SIZE];	/* buffer to hold one font character */

/* A list of all Max7456 registers. */
/* Read/write registers. */
/* addr	key	name				default	restore_eeprom	*/
#define REG_MAP_RW							\
X(0x80, VM0,   "Video Mode 0",			0x08,	true)		\
X(0x81, VM1,   "Video Mode 1",			0x47,	true)		\
X(0x82, HOS,   "Horizontal Offset",		0x20,	true)		\
X(0x83, VOS,   "Vertical Offset",		0x10,	true)		\
X(0x84, DMM,   "Display Memory Mode",		0x00,	false)		\
X(0x85, DMAH,  "Display Memory Address High",	0x00,	false)		\
X(0x86, DMAL,  "Display Memory Address Low",	0x00,	false)		\
X(0x87, DMDI,  "Display Memory Data In",	0x00,	false)		\
X(0x88, CMM,   "Character Memory Mode",		0x00,	false)		\
X(0x89, CMAH,  "Character Memory Address High",	0x00,	false)		\
X(0x8A, CMAL,  "Character Memory Address Low",	0x00,	false)		\
X(0x8B, CMDI,  "Character Memory Data In",	0x00,	false)		\
X(0x8C, OSDM,  "OSD Insertion Mux",		0x1B,	true)		\
X(0x90, RB0,   "Row 0 Brightness",		0x01,	true)		\
X(0x91, RB1,   "Row 1 Brightness",		0x01,	true)		\
X(0x92, RB2,   "Row 2 Brightness",		0x01,	true)		\
X(0x93, RB3,   "Row 3 Brightness",		0x01,	true)		\
X(0x94, RB4,   "Row 4 Brightness",		0x01,	true)		\
X(0x95, RB5,   "Row 5 Brightness",		0x01,	true)		\
X(0x96, RB6,   "Row 6 Brightness",		0x01,	true)		\
X(0x97, RB7,   "Row 7 Brightness",		0x01,	true)		\
X(0x98, RB8,   "Row 8 Brightness",		0x01,	true)		\
X(0x99, RB9,   "Row 9 Brightness",		0x01,	true)		\
X(0x9A, RB10,  "Row 10 Brightness",		0x01,	true)		\
X(0x9B, RB11,  "Row 11 Brightness",		0x01,	true)		\
X(0x9C, RB12,  "Row 12 Brightness",		0x01,	true)		\
X(0x9D, RB13,  "Row 13 Brightness",		0x01,	true)		\
X(0x9E, RB14,  "Row 14 Brightness",		0x01,	true)		\
X(0x9F, RB15,  "Row 15 Brightness",		0x01,	true)		\
X(0xEC, OSDBL, "OSD Black Level",		0x10,	false)		\

/* Readonly registers. */
/* addr	key	name							*/
#define REG_MAP_R							\
X(0xAF, STAT,  "Status")						\
X(0xBF, DMDO,  "Display Memory Data Out")				\
X(0xCF, CMDO,  "Character Memory Data Out")				\

/* X-macro generating the number of writeable registers */
#define X(addr, key, name, def, save_eeprom) key,
typedef enum regw_count_t {
    REG_MAP_RW
    REGW_COUNT
} regw_count_t;
#undef X

typedef struct regset_t {		/* used as single (return) value */
    uint8_t values[REGW_COUNT];		/* register values */
} regset_t;

extern regset_t DEFREGVALS;		/* default register values */

#define MAXROWS (16)			/* maximum number of screen rows */
#define MAXCOLS (30)			/* maximum number of screen columns */
#define MAXSCRSIZE (MAXROWS * MAXCOLS)  /* maximum screen buffer size */
#define MAXDIRTSIZE (MAXSCRSIZE/8 + 1)	/* maximum dirt size */

#define SCREENDIRTY(screenbuf, pos) 				\
    ((screenbuf).dirt[(pos) / 8] & (0x01 << ((pos) % 8)))

#define SCREENSETDIRTY(screenbuf, pos) 				\
{								\
    (screenbuf).dirt[(pos) / 8] |= (0x01 << ((pos) % 8));	\
}

#define SCREENUNSETDIRTY(screenbuf, pos) 			\
{								\
    (screenbuf).dirt[(pos) / 8] &= ~(0x01 << ((pos) % 8));	\
}

#define SCREENSET(screenbuf, pos, val) 				\
{								\
    (screenbuf).buf[(pos)] = (val);				\
    SCREENSETDIRTY((screenbuf), (pos));				\
    (screenbuf).dirty = true;					\
}

typedef struct screenbuf_t {		/* shadow screenbuffer */
    char 	buf[MAXSCRSIZE];	/* buffer contents */
    uint8_t 	dirt[MAXDIRTSIZE];	/* dirty flag for each char in buf */
    int		rows;			/* number of rows */
    int 	cols;			/* number of columns */
    bool 	dirty;			/* whether screen needs redrawing */
} screenbuf_t, *screenbuf_p;

/*
 * A functional interface costs a lot (stack space & handling) but would be
 * clean. however, the interrupt handler that calls a function that uses a
 * screen buffer can't take any parameter so we have to store something
 * globally anyhow. See max_vsync() for details.
 */
extern screenbuf_t screenbuf;

bool max_setup(bool);

bool max_reset(bool);

bool max_enable(bool);

bool max_regsetget(regset_t *);

bool max_regsetput(regset_t *);

bool max_fontcharget(uint8_t, fontbuf_t *);

bool max_fontcharput(uint8_t, fontbuf_t *);

void max_videodetect(void);

int8_t max_hos(int16_t);

int8_t max_vos(int16_t);

int8_t max_insmux1(int16_t);

int8_t max_insmux2(int16_t);

int8_t max_cbl(int8_t, int16_t);

int8_t max_cwl(int8_t, int16_t);

bool max_refreshscreen();

#endif /* MAX7456_H */
