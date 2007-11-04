/*
    CAN data visualizer for the Toyota Prius
    User interface function prototypes

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

#define	WIDTH	640
#define	HEIGHT	480

int ui_create_window();
void ui_main_loop();
void UIAdjustBacklight(unsigned char dimmed);
void UICopyDisplayBufferToScreen(int x, int y, int w, int h);
void UICleanUp(int vis);
