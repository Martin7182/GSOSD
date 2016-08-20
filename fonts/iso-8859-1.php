<?php
#=========================================================================
#  Copyright (c) 2016 Martin7182
#
#  This file is part of GSOSD.
#
#  GSOSD is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  GSOSD is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GSOSD.  If not, see <http://www.gnu.org/licenses/>.
#
#  File		: iso-8859-1.php
#  Purpose	: Print ascii charset, control characters as spaces.
#  Author(s)	: Martin7182
#  Creation	: 2016/05/20
#
#=========================================================================

#
# Characters are printed in a 16x16 matrix between start and stop headers.
#
printf("=== START ===\n");
for ($c=0; $c < 256; $c++) {
    if ($c < 32) {
        printf("%c", " ");
    } else if ($c > 126 && $c < 160) {
        printf("%c", " ");
    } else {
        printf("%c", $c);
    }
    if (($c + 1) % 16 == 0) {
        printf("\n");
    }
}
printf("=== STOP ===\n");
?>
