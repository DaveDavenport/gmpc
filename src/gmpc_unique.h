#ifndef __GMPC_UNIQUE_H__
#define __GMPC_UNIQUE_H__

typedef enum _GmpcCommands{
    COMMAND_QUIT                = 1,
    COMMAND_PRESENT             = 2,
    COMMAND_PLAYER_PLAY         = 3,
    COMMAND_PLAYER_PAUSE        = 4,
    COMMAND_PLAYER_STOP         = 5,
    COMMAND_PLAYER_NEXT         = 6,
    COMMAND_PLAYER_PREV         = 7,
    COMMAND_VIEW_TOGGLE         = 8,
    COMMAND_VIEW_HIDE           = 9,
    COMMAND_VIEW_SHOW           = 10,
    COMMAND_PLAYLIST_ADD_STREAM = 11

} GmpcCommands;
#endif
