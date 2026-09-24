#include "../include/common.h"
#include "../include/adapter.h"
#include "../include/dbus_helper.h"
#include "../include/log.h"

void adapter_print_name(void)
{
    GDBusConnection *conn;

    GError *error = NULL;

    GVariant *result;
    GVariant *value;

    const gchar *name;

    conn = dbus_get_system_connection();

    if(conn == NULL)
        return;

    result =
        g_dbus_connection_call_sync(
            conn,
            "org.bluez",
            "/org/bluez/hci0",
            "org.freedesktop.DBus.Properties",
            "Get",
            g_variant_new("(ss)",
                          "org.bluez.Adapter1",
                          "Name"),
            G_VARIANT_TYPE("(v)"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

    if(error)
    {
        log_error("%s", error->message);
        g_error_free(error);
        return;
    }

    g_variant_get(result, "(v)", &value);
    g_variant_get(value, "s", &name);
    log_info("Bluetooth Adapter : %s", name);
    g_variant_unref(value);
    g_variant_unref(result);
}

int adapter_start_discovery(void)
{
    GDBusConnection *conn;

    GError *error = NULL;

    conn = dbus_get_system_connection();

    if(conn == NULL)
        return -1;

    g_dbus_connection_call_sync(
            conn,
            "org.bluez",
            "/org/bluez/hci0",
            "org.bluez.Adapter1",
            "StartDiscovery",
            NULL,
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

    if(error)
    {
        log_error("%s", error->message);
        g_error_free(error);
        return -1;
    }

    log_info("Discovery Started");

    return 0;
}

int adapter_stop_discovery(void)
{
    GDBusConnection *conn;

    GError *error = NULL;

    conn = dbus_get_system_connection();

    if(conn == NULL)
        return -1;

    g_dbus_connection_call_sync(
            conn,
            "org.bluez",
            "/org/bluez/hci0",
            "org.bluez.Adapter1",
            "StopDiscovery",
            NULL,
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

    if(error)
    {
        log_error("%s", error->message);
        g_error_free(error);
        return -1;
    }

    log_info("Discovery Stopped");

    return 0;
}