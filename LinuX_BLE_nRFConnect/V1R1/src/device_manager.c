#include "device_manager.h"
#include "log.h"

static BLE_DEVICE device_table[MAX_BLE_DEVICES];

void device_manager_init(void)
{
    memset(device_table,0,sizeof(device_table));
}

BLE_DEVICE *device_find_by_path(const char *path)
{
    for(int i=0;i<MAX_BLE_DEVICES;i++)
    {
        if(device_table[i].used && strcmp(device_table[i].object_path,path) == 0)
        {
            return &device_table[i];
        }
    }

    return NULL;
}

BLE_DEVICE *device_add(const char *path)
{
    BLE_DEVICE *device;
    device=device_find_by_path(path);

    if(device)
        return device;

    for(int i=0;i<MAX_BLE_DEVICES;i++)
    {
        if(device_table[i].used==FALSE)
        {
            device_table[i].used=TRUE;

            strncpy(device_table[i].object_path,path,sizeof(device_table[i].object_path)-1);
            return &device_table[i];
        }
    }
    return NULL;
}

void device_print_all(void)
{
    log_info("======================================");
    log_info("Known Devices");
    log_info("======================================");

    for(int i=0;i<MAX_BLE_DEVICES;i++)
    {
        if(!device_table[i].used)
            continue;

        log_info("%2d  %s",i,device_table[i].address);
        log_info("    Name : %s",device_table[i].name);
        log_info("    RSSI : %d",device_table[i].rssi);
        log_info("    Connected : %d",device_table[i].connected);
        log_info("");
    }
}