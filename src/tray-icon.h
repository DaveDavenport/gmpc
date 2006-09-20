#ifndef __TRAY_ICON_H__
#define __TRAY_ICON_H__

void tray_icon_connection_changed(MpdObj *mi, int connect);

int tray_availible(void);

void tray_notify_popup(void);

#endif
