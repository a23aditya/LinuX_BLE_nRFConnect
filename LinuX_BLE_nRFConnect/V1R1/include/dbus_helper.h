#ifndef DBUS_HELPER_H
#define DBUS_HELPER_H

#include "common.h"

GDBusConnection *dbus_get_system_connection(void);
void dbus_print_properties(GVariant *properties);

#endif