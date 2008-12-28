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
    KB_ACTION_TOGGLE_MUTE
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
    KB_QUIT,
    KB_CLOSE,
    KB_FULLSCREEN,
    KB_REPEAT,
    KB_RANDOM,
    KB_TOGGLE_MUTE,
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
        "Mute"
        };
int KeybindingDefault[KB_NUM][3] = {
        {GDK_Up,            GDK_CONTROL_MASK,                   KB_ACTION_PLAY},
        {GDK_Right,         GDK_CONTROL_MASK,                   KB_ACTION_NEXT},
        {GDK_Left,          GDK_CONTROL_MASK,                   KB_ACTION_PREV},
        {GDK_Down,          GDK_CONTROL_MASK,                   KB_ACTION_STOP},
        {GDK_Delete,        GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_CLEAR_PLAYLIST},
        {GDK_Insert,        GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_FULL_ADD_PLAYLIST},
        {GDK_KP_Subtract,   0,                                  KB_ACTION_INTERFACE_COLLAPSE},
        {GDK_KP_Add,        0 ,                                 KB_ACTION_INTERFACE_EXPAND},
        {GDK_minus,         GDK_CONTROL_MASK,                   KB_ACTION_INTERFACE_COLLAPSE},
        {GDK_plus,          GDK_CONTROL_MASK|GDK_SHIFT_MASK,    KB_ACTION_INTERFACE_EXPAND},
        {GDK_w,             GDK_CONTROL_MASK,                   KB_ACTION_CLOSE},
        {GDK_q,             GDK_CONTROL_MASK,                   KB_ACTION_QUIT},
        {GDK_F12,           0,                                  KB_ACTION_FULLSCREEN},
        {GDK_r,             GDK_CONTROL_MASK,                   KB_ACTION_REPEAT},
        {GDK_s,             GDK_CONTROL_MASK,                   KB_ACTION_RANDOM},
        {GDK_m,             GDK_CONTROL_MASK,                   KB_ACTION_TOGGLE_MUTE}
};
#endif
