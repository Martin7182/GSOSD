/*========================================================================
 *  Copyright (c) 2017 Martin7182
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
 *  File	: font.h
 *  Purpose	: Declarations for font handling.
 *  Author(s)	: Martin7182
 *  Creation	: 2017/10/16
 *
 *========================================================================
 */

#ifndef FONT_H
#define FONT_H

typedef enum {		/* possible font effects on currently loaded font */
    FE_BORDER = 0,	/* black border */
    FE_SHADOW,		/* shadow */
    FE_TRANSWHITE,	/* don't use black pixels */
    FE_BLACKWHITE,	/* don't use transparent pixels */
    FE_INVERT,		/* invert black/white pixels */
    FE_NONE,		/* none */
} fonteffect_t, *fonteffect_p;

bool font_apply(fonteffect_p);

#endif /* FONT_H */

