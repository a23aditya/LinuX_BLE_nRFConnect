#include "common.h"
#include "advertisement.h"
#include "gatt_server.h"
#include "dbus_helper.h"
#include "log.h"

#define ADAPTER_PATH   "/org/bluez/hci0"
#define ADV_PATH       "/org/bluez/example/advertisement0"

static const gchar *advertisement_xml =
    "<node>"
    "  <interface name='org.bluez.LEAdvertisement1'>"
    "    <property type='s' name='Type' access='read'/>"
    "    <property type='as' name='ServiceUUIDs' access='read'/>"
    "    <property type='s' name='LocalName' access='read'/>"
    "    <property type='b' name='IncludeTxPower' access='read'/>"
    "    <method name='Release'/>"
    "  </interface>"
    "</node>";

static void advertisement_method_call(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)parameters; (void)user_data;

    if (g_strcmp0(method_name, "Release") == 0)
    {
        log_info("Advertisement released by BlueZ");
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
            G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method %s", method_name);
    }
}

static GVariant *advertisement_get_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)user_data;

    if (g_strcmp0(property_name, "Type") == 0)
        return g_variant_new_string("peripheral");
    if (g_strcmp0(property_name, "ServiceUUIDs") == 0)
    {
        const gchar *uuids[] = { GATT_SERVICE_UUID, NULL };
        return g_variant_new_strv(uuids, -1);
    }
    if (g_strcmp0(property_name, "LocalName") == 0)
        return g_variant_new_string("BLE-Framework");
    if (g_strcmp0(property_name, "IncludeTxPower") == 0)
        return g_variant_new_boolean(TRUE);

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Unknown property %s", property_name);
    return NULL;
}

static const GDBusInterfaceVTable advertisement_vtable = {
    .method_call = advertisement_method_call,
    .get_property = advertisement_get_property,
    .set_property = NULL
};

static void register_advertisement_cb(GObject *source, GAsyncResult *res, gpointer user_data)
{
    (void)user_data;
    GError *error = NULL;

    GVariant *result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &error);

    if (error)
    {
        log_error("RegisterAdvertisement failed: %s", error->message);
        log_error("(Is the adapter powered on? Try: bluetoothctl power on)");
        g_error_free(error);
        return;
    }

    log_info("Advertisement registered with BlueZ");
    g_variant_unref(result);
}

void advertisement_init(void)
{
    GDBusConnection *conn = dbus_get_system_connection();
    if (!conn)
    {
        log_error("Advertisement: no D-Bus connection");
        return;
    }

    GError *error = NULL;
    GDBusNodeInfo *node = g_dbus_node_info_new_for_xml(advertisement_xml, &error);

    if (!node)
    {
        log_error("Advertisement introspection parse failed: %s", error->message);
        g_error_free(error);
        return;
    }

    guint reg_id = g_dbus_connection_register_object(
        conn, ADV_PATH, node->interfaces[0], &advertisement_vtable, NULL, NULL, &error);

    if (reg_id == 0)
    {
        log_error("Failed to register advertisement object: %s", error->message);
        g_error_free(error);
        return;
    }

    GVariantBuilder empty_options;
    g_variant_builder_init(&empty_options, G_VARIANT_TYPE("a{sv}"));

    g_dbus_connection_call(
        conn,
        "org.bluez",
        ADAPTER_PATH,
        "org.bluez.LEAdvertisingManager1",
        "RegisterAdvertisement",
        g_variant_new("(oa{sv})", ADV_PATH, &empty_options),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        register_advertisement_cb,
        NULL);
}
