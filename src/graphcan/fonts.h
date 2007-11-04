/*
    CAN data visualizer for the Toyota Prius
    Fonts

    Copyright 2004-2005 Attila Vass
    Copyright 2007 Peter Åstrand <astrand@lysator.liu.se>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define	FONTMAP_WIDTH	176
#define	FONTMAP_HEIGHT	31
#define	FONTMAP_COLORS	10

typedef struct _fontmetrics
{
    char letter;
    int x;
    int y;
    int width;
    int height;
} fontmetrics;
