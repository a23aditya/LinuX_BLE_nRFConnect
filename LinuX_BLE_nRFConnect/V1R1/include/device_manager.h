#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "common.h"

#define MAX_BLE_DEVICES     128

typedef struct
{
    gboolean used;
    char object_path[128];
    char address[32];
    char name[64];
    char alias[64];
    char address_type[16];
    int16_t rssi;
    gboolean connected;
    gboolean paired;
    gboolean trusted;
    gboolean services_resolved;
} BLE_DEVICE;

void device_manager_init(void);
BLE_DEVICE * device_find_by_path(const char *path);
BLE_DEVICE *device_add(const char *path);
void device_print_all(void);

#endif