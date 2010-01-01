/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __PLAYLIST3_KEYBINDINGS_H__
#define __PLAYLIST3_KEYBINDINGS_H__
/**
 * Keybindings
 */

#define KB_GLOBAL "keybindings-keycode-global"
#define MK_GLOBAL "keybindings-mask-global"
#define AC_GLOBAL "keybindings-action"

typedef enum _KeybindAction{
    KB_ACTION_PLAY,
    KB_ACTION_NEXT,
    KB_ACTION_PREV,
    KB_ACTION_STOP,
    KB_ACTION_CLEAR_PLAYLIST,
    KB_ACTION_FULL_ADD_PLAYLIST,
    KB_ACTION_INTERFACE_COLLAPSE,
    KB_ACTION_INTERFACE_EXPAND,
    KB_ACTION_CLOSE,
    KB_ACTION_QUIT,
    KB_ACTION_FULLSCREEN,
    KB_ACTION_REPEAT,
    KB_ACTION_RANDOM,
    KB_ACTION_TOGGLE_MUTE,
    KB_ACTION_SINGLE_MODE,
    KB_ACTION_CONSUME,
    KB_ACTION_FF,
    KB_ACTION_REW
}KeybindAction;
/** Some default keybindings */
typedef enum _Keybind{
    KB_PLAY,
    KB_NEXT,
    KB_PREV,
    KB_STOP,
    KB_CLEAR_PLAYLIST,
    KB_FULL_ADD_PLAYLIST,
    KB_INTERFACE_COLLAPSE_KP,
    KB_INTERFACE_EXPAND_KP,
    KB_INTERFACE_COLLAPSE,
    KB_INTERFACE_EXPAND,
    KB_CLOSE,
    KB_QUIT,
    KB_FULLSCREEN,
    KB_REPEAT,
    KB_RANDOM,
    KB_TOGGLE_MUTE,
    KB_SINGLE_MODE,
    KB_CONSUME,
    KB_FF,
    KB_REW,
    KB_NUM
}Keybind;


const char *Keybindname[KB_NUM] = {
        "Play",
        "Next",
        "Previous",
        "Stop",
        "Clear Playlist",
        "Full Add Playlist",
        "Interface Collapse Keypad",
        "Interface Expand Keypad",
        "Interface Collapse",
        "Interface Expand",
        "Close",
        "Quit",
        "Fullscreen",
        "Repeat",
        "Random",
        "Mute",
        "SingleMode",
        "Consume",
        "Fast Forward",
        "Rewind"
        };
int KeybindingDefault[KB_NUM][3] = {
        {GDK_Up,            GDK_CONTROL_MASK,                   KB_ACTION_PLAY},                    // KB_PLAY
        {GDK_Right,         GDK_CONTROL_MASK,                   KB_ACTION_NEXT},                    // KB_NEXT
        {GDK_Left,          GDK_CONTROL_MASK,                   KB_ACTION_PREV},                    // KB_PREV
        {GDK_Down,          GDK_CONTROL_MASK,                   KB_ACTION_STOP},                    // KB_STOP
        {GDK_Delete,        GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_CLEAR_PLAYLIST},          // KB_CLEAR_PLAYLIST
        {GDK_Insert,        GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_FULL_ADD_PLAYLIST},       // KB_ADD PLAYLIST
        {GDK_KP_Subtract,   0,                                  KB_ACTION_INTERFACE_COLLAPSE},      // KB_INTERFACE_COLLAPSE_KP
        {GDK_KP_Add,        0 ,                                 KB_ACTION_INTERFACE_EXPAND},        // KB_INTERFACE_EXPAND_KP
        {GDK_minus,         GDK_CONTROL_MASK,                   KB_ACTION_INTERFACE_COLLAPSE},      // KB_INTERFACE_COLLAPSE
        {GDK_plus,          GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_INTERFACE_EXPAND},        // KB_INTERFACE_EXPAND
        {GDK_w,             GDK_CONTROL_MASK,                   KB_ACTION_CLOSE},                   // KB_CLOSE
        {GDK_q,             GDK_CONTROL_MASK,                   KB_ACTION_QUIT},                    // KB_QUIT
        {GDK_F12,           0,                                  KB_ACTION_FULLSCREEN},              // KB_FULLSCREEN
        {GDK_r,             GDK_CONTROL_MASK,                   KB_ACTION_REPEAT},                  // KB_REPEAT
        {GDK_s,             GDK_CONTROL_MASK,                   KB_ACTION_RANDOM},                  // KB_RANDOM
        {GDK_m,             GDK_CONTROL_MASK,                   KB_ACTION_TOGGLE_MUTE},             // KB_TOGGLE_MUTE
        {GDK_k,             GDK_CONTROL_MASK,                   KB_ACTION_SINGLE_MODE},             // KB_SINGLE_MODE
        {GDK_l,             GDK_CONTROL_MASK,                   KB_ACTION_CONSUME},                 // KB_CONSUME
        {GDK_Right,         GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_FF},                      // KB_FF
        {GDK_Left,          GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_REW}                      // KB_REW
};
#endif
