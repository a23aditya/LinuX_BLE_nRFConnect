#include "dbus_helper.h"
#include "signal_handler.h"
#include "device_manager.h"
#include "log.h"


void device_update_properties(BLE_DEVICE *device,GVariant *properties)
{
    if (device == NULL || properties == NULL)
        return;

    GVariantIter iter;
    const gchar *key;
    GVariant *value;

    g_variant_iter_init(&iter, properties);

    while (g_variant_iter_next(&iter, "{&sv}", &key, &value))
    {
        /* ---------------- Address ---------------- */
        if (g_strcmp0(key, "Address") == 0)
        {
            strncpy(device->address,
                    g_variant_get_string(value, NULL),
                    sizeof(device->address) - 1);
        }

        /* ---------------- Name ---------------- */
        else if (g_strcmp0(key, "Name") == 0)
        {
            strncpy(device->name,
                    g_variant_get_string(value, NULL),
                    sizeof(device->name) - 1);
        }

        /* ---------------- Alias ---------------- */
        else if (g_strcmp0(key, "Alias") == 0)
        {
            strncpy(device->alias,
                    g_variant_get_string(value, NULL),
                    sizeof(device->alias) - 1);
        }

        /* ---------------- Address Type ---------------- */
        else if (g_strcmp0(key, "AddressType") == 0)
        {
            strncpy(device->address_type,
                    g_variant_get_string(value, NULL),
                    sizeof(device->address_type) - 1);
        }

        /* ---------------- RSSI ---------------- */
        else if (g_strcmp0(key, "RSSI") == 0)
        {
            device->rssi = g_variant_get_int16(value);
        }

        /* ---------------- Connected ---------------- */
        else if (g_strcmp0(key, "Connected") == 0)
        {
            device->connected = g_variant_get_boolean(value);
        }

        /* ---------------- Paired ---------------- */
        else if (g_strcmp0(key, "Paired") == 0)
        {
            device->paired = g_variant_get_boolean(value);
        }

        /* ---------------- Trusted ---------------- */
        else if (g_strcmp0(key, "Trusted") == 0)
        {
            device->trusted = g_variant_get_boolean(value);
        }

        /* ---------------- ServicesResolved ---------------- */
        else if (g_strcmp0(key, "ServicesResolved") == 0)
        {
            device->services_resolved = g_variant_get_boolean(value);
        }

        g_variant_unref(value);
    }

    log_info("========== Device Updated ==========");
    log_info("Path              : %s", device->object_path);
    log_info("Address           : %s", device->address);
    log_info("Name              : %s", device->name);
    log_info("Alias             : %s", device->alias);
    log_info("Address Type      : %s", device->address_type);
    log_info("RSSI              : %d", device->rssi);
    log_info("Connected         : %d", device->connected);
    log_info("Paired            : %d", device->paired);
    log_info("Trusted           : %d", device->trusted);
    log_info("ServicesResolved  : %d", device->services_resolved);
}

static void interfaces_added_callback(GDBusConnection *connection,const gchar *sender_name,const gchar *object_path,const gchar *interface_name,const gchar *signal_name,GVariant *parameters,gpointer user_data)
{
    //unused parameters
    (void)connection;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;
    (void)user_data;

    const gchar *device_path;
    GVariant *interfaces;

    // Extract the device path and interfaces from the parameters
    g_variant_get(parameters,"(o@a{sa{sv}})",&device_path,&interfaces);

    log_info("--------------------------------");
   // log_info("Device : %s", device_path);

    BLE_DEVICE *device;
    device = device_add(device_path);

    GVariantIter iter;
    const gchar *iface_name;
    GVariant *properties;

    g_variant_iter_init(&iter, interfaces);

    // Iterate through the interfaces and print their properties
    while (g_variant_iter_next(&iter,"{&s@a{sv}}",&iface_name,&properties))
    {
        log_info("Interface : %s", iface_name);
       // dbus_print_properties(properties);
       device_update_properties(device, properties);
        g_variant_unref(properties);
    }

    g_variant_unref(interfaces);
}

static void properties_changed_callback(GDBusConnection *connection,const gchar *sender_name,const gchar *object_path,const gchar *interface_name,const gchar *signal_name,GVariant *parameters,gpointer user_data)
{
    (void)connection;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;
    (void)user_data;

    const gchar *iface_name;
    GVariant *changed_properties;
    GVariant *invalidated_properties;

    g_variant_get(
            parameters,
            "(&s@a{sv}@as)",
            &iface_name,
            &changed_properties,
            &invalidated_properties);

    if (g_strcmp0(iface_name, "org.bluez.Device1") != 0)
    {
        g_variant_unref(changed_properties);
        g_variant_unref(invalidated_properties);
        return;
    }

    log_info("--------------------------------");
    log_info("Device Properties Changed : %s", object_path);
    dbus_print_properties(changed_properties);

    g_variant_unref(changed_properties);
    g_variant_unref(invalidated_properties);
}

void signal_handler_init(void)
{
    GDBusConnection *conn = dbus_get_system_connection();
    if (!conn)
    {
        log_error("NULL connection");
        return;
    }
    g_dbus_connection_signal_subscribe(
        conn,
        "org.bluez",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesAdded",
        NULL,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        interfaces_added_callback,
        NULL,
        NULL);

    g_dbus_connection_signal_subscribe(
        conn,
        "org.bluez",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        NULL,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        properties_changed_callback,
        NULL,
        NULL);
}