#include "common.h"
#include "gatt_server.h"
#include "dbus_helper.h"
#include "log.h"

#define ADAPTER_PATH        "/org/bluez/hci0"

#define APP_PATH            "/org/bluez/example"
#define SERVICE_PATH        APP_PATH "/service0"
#define CHAR_RW_PATH        SERVICE_PATH "/char0"
#define CHAR_NOTIFY_PATH    SERVICE_PATH "/char1"

static GDBusConnection *gatt_conn = NULL;

/* Backing storage for char0 (read/write) */
static guint8 rw_value[64] = "hello";
static gsize  rw_value_len = 5;

/* char1 = LED status (read + notify) */
uint8_t gLed = 0;                 /* 0 = off, 1 = on - toggled every 3s */
static gboolean notifying = FALSE; /* has a client subscribed via StartNotify? */

/* ---------------------------------------------------------------------
 * org.freedesktop.DBus.ObjectManager  (exported at APP_PATH)
 *
 * BlueZ calls GetManagedObjects() right after RegisterApplication() to
 * discover the service/characteristic tree - this is how it learns what
 * we're exporting.
 * ------------------------------------------------------------------- */

static const gchar *object_manager_xml =
    "<node>"
    "  <interface name='org.freedesktop.DBus.ObjectManager'>"
    "    <method name='GetManagedObjects'>"
    "      <arg type='a{oa{sa{sv}}}' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static GVariant *build_managed_objects(void)
{
    GVariantBuilder objects;
    g_variant_builder_init(&objects, G_VARIANT_TYPE("a{oa{sa{sv}}}"));

    /* ---- service0 : org.bluez.GattService1 ---- */
    {
        GVariantBuilder props;
        g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(GATT_SERVICE_UUID));
        g_variant_builder_add(&props, "{sv}", "Primary", g_variant_new_boolean(TRUE));

        GVariantBuilder ifaces;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        g_variant_builder_add(&ifaces, "{sa{sv}}", "org.bluez.GattService1", &props);

        g_variant_builder_add(&objects, "{oa{sa{sv}}}", SERVICE_PATH, &ifaces);
    }

    /* ---- char0 : org.bluez.GattCharacteristic1 (read/write) ---- */
    {
        const gchar *flags_rw[] = { "read", "write", NULL };

        GVariantBuilder props;
        g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(GATT_CHAR_RW_UUID));
        g_variant_builder_add(&props, "{sv}", "Service", g_variant_new_object_path(SERVICE_PATH));
        g_variant_builder_add(&props, "{sv}", "Flags", g_variant_new_strv(flags_rw, -1));

        GVariantBuilder ifaces;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        g_variant_builder_add(&ifaces, "{sa{sv}}", "org.bluez.GattCharacteristic1", &props);

        g_variant_builder_add(&objects, "{oa{sa{sv}}}", CHAR_RW_PATH, &ifaces);
    }

    /* ---- char1 : org.bluez.GattCharacteristic1 (LED status: read + notify) ---- */
    {
        const gchar *flags_notify[] = { "read", "notify", NULL };

        GVariantBuilder props;
        g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(GATT_CHAR_NOTIFY_UUID));
        g_variant_builder_add(&props, "{sv}", "Service", g_variant_new_object_path(SERVICE_PATH));
        g_variant_builder_add(&props, "{sv}", "Flags", g_variant_new_strv(flags_notify, -1));
        g_variant_builder_add(&props, "{sv}", "Notifying", g_variant_new_boolean(notifying));

        GVariantBuilder ifaces;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        g_variant_builder_add(&ifaces, "{sa{sv}}", "org.bluez.GattCharacteristic1", &props);

        g_variant_builder_add(&objects, "{oa{sa{sv}}}", CHAR_NOTIFY_PATH, &ifaces);
    }

    return g_variant_new("(a{oa{sa{sv}}})", &objects);
}

static void object_manager_method_call(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)parameters; (void)user_data;

    if (g_strcmp0(method_name, "GetManagedObjects") == 0)
    {
        log_info("BlueZ requested GetManagedObjects()");
        g_dbus_method_invocation_return_value(invocation, build_managed_objects());
    }
    else
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
            G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method %s", method_name);
    }
}

static const GDBusInterfaceVTable object_manager_vtable = {
    .method_call = object_manager_method_call,
    .get_property = NULL,
    .set_property = NULL
};

/* ---------------------------------------------------------------------
 * org.bluez.GattService1  (exported at SERVICE_PATH)
 * ------------------------------------------------------------------- */

static const gchar *gatt_service_xml =
    "<node>"
    "  <interface name='org.bluez.GattService1'>"
    "    <property type='s' name='UUID' access='read'/>"
    "    <property type='b' name='Primary' access='read'/>"
    "  </interface>"
    "</node>";

static GVariant *service_get_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)user_data;

    if (g_strcmp0(property_name, "UUID") == 0)
        return g_variant_new_string(GATT_SERVICE_UUID);
    if (g_strcmp0(property_name, "Primary") == 0)
        return g_variant_new_boolean(TRUE);

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Unknown property %s", property_name);
    return NULL;
}

static const GDBusInterfaceVTable gatt_service_vtable = {
    .method_call = NULL,
    .get_property = service_get_property,
    .set_property = NULL
};

/* ---------------------------------------------------------------------
 * org.bluez.GattCharacteristic1  -  char0 (read / write)
 * ------------------------------------------------------------------- */

static const gchar *char_rw_xml =
    "<node>"
    "  <interface name='org.bluez.GattCharacteristic1'>"
    "    <property type='s' name='UUID' access='read'/>"
    "    <property type='o' name='Service' access='read'/>"
    "    <property type='as' name='Flags' access='read'/>"
    "    <method name='ReadValue'>"
    "      <arg type='a{sv}' name='options' direction='in'/>"
    "      <arg type='ay' direction='out'/>"
    "    </method>"
    "    <method name='WriteValue'>"
    "      <arg type='ay' name='value' direction='in'/>"
    "      <arg type='a{sv}' name='options' direction='in'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static void char_rw_method_call(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)user_data;

    if (g_strcmp0(method_name, "ReadValue") == 0)
    {
        GVariantBuilder array;
        g_variant_builder_init(&array, G_VARIANT_TYPE("ay"));
        for (gsize i = 0; i < rw_value_len; i++)
            g_variant_builder_add(&array, "y", rw_value[i]);

        log_info("GATT: ReadValue on char0 (%" G_GSIZE_FORMAT " bytes)", rw_value_len);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(ay)", &array));
    }
    else if (g_strcmp0(method_name, "WriteValue") == 0)
    {
        GVariantIter *iter = NULL;
        GVariant *options = NULL;
        guint8 byte;
        gsize i = 0;

        g_variant_get(parameters, "(ay@a{sv})", &iter, &options);

        while (i < sizeof(rw_value) - 1 && g_variant_iter_next(iter, "y", &byte))
            rw_value[i++] = byte;
        rw_value_len = i;
        rw_value[i] = '\0'; /* handy when the peer writes plain ASCII text */

        g_variant_iter_free(iter);
        g_variant_unref(options);

        log_info("GATT: WriteValue on char0 (%" G_GSIZE_FORMAT " bytes): \"%s\"",
                  rw_value_len, (const char *)rw_value);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
            G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method %s", method_name);
    }
}

static GVariant *char_rw_get_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)user_data;

    if (g_strcmp0(property_name, "UUID") == 0)
        return g_variant_new_string(GATT_CHAR_RW_UUID);
    if (g_strcmp0(property_name, "Service") == 0)
        return g_variant_new_object_path(SERVICE_PATH);
    if (g_strcmp0(property_name, "Flags") == 0)
    {
        const gchar *flags[] = { "read", "write", NULL };
        return g_variant_new_strv(flags, -1);
    }

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Unknown property %s", property_name);
    return NULL;
}

static const GDBusInterfaceVTable char_rw_vtable = {
    .method_call = char_rw_method_call,
    .get_property = char_rw_get_property,
    .set_property = NULL
};

/* ---------------------------------------------------------------------
 * org.bluez.GattCharacteristic1  -  char1 (notify)
 * ------------------------------------------------------------------- */

static const gchar *char_notify_xml =
    "<node>"
    "  <interface name='org.bluez.GattCharacteristic1'>"
    "    <property type='s' name='UUID' access='read'/>"
    "    <property type='o' name='Service' access='read'/>"
    "    <property type='as' name='Flags' access='read'/>"
    "    <property type='b' name='Notifying' access='read'/>"
    "    <method name='ReadValue'>"
    "      <arg type='a{sv}' name='options' direction='in'/>"
    "      <arg type='ay' direction='out'/>"
    "    </method>"
    "    <method name='StartNotify'/>"
    "    <method name='StopNotify'/>"
    "  </interface>"
    "</node>";

/* Pushes the current gLed value out as a PropertiesChanged signal on
 * "Value" - this is the mechanism BlueZ watches to turn into an actual
 * ATT notification to any subscribed client. */
static void emit_led_status(void)
{
    GVariantBuilder array;
    g_variant_builder_init(&array, G_VARIANT_TYPE("ay"));
    g_variant_builder_add(&array, "y", gLed);

    GVariantBuilder changed;
    g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&changed, "{sv}", "Value", g_variant_new("ay", &array));

    GVariantBuilder invalidated;
    g_variant_builder_init(&invalidated, G_VARIANT_TYPE("as"));

    g_dbus_connection_emit_signal(
        gatt_conn,
        NULL,
        CHAR_NOTIFY_PATH,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        g_variant_new("(sa{sv}as)", "org.bluez.GattCharacteristic1", &changed, &invalidated),
        NULL);
}

/* Toggles the LED every 3 seconds - runs continuously regardless of
 * whether a BLE client is connected, so the LED behaves the same way
 * a real embedded blink loop would. Only pushes a BLE notification if
 * someone has actually subscribed (StartNotify). */
static gboolean led_toggle_timeout_cb(gpointer user_data)
{
    (void)user_data;

    gLed = !gLed;
    log_info("LED status: %s", gLed ? "ON" : "OFF");

    /* On real hardware (e.g. Luckfox Pico Plus), replace this with the
     * actual GPIO write, e.g.: gpio_set_value(LED_GPIO_PIN, gLed); */

    if (notifying)
        emit_led_status();

    return G_SOURCE_CONTINUE;
}

static void char_notify_method_call(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)parameters; (void)user_data;

    if (g_strcmp0(method_name, "ReadValue") == 0)
    {
        GVariantBuilder array;
        g_variant_builder_init(&array, G_VARIANT_TYPE("ay"));
        g_variant_builder_add(&array, "y", gLed);

        log_info("GATT: ReadValue on char1 (LED = %s)", gLed ? "ON" : "OFF");
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(ay)", &array));
    }
    else if (g_strcmp0(method_name, "StartNotify") == 0)
    {
        if (!notifying)
        {
            notifying = TRUE;
            log_info("GATT: notifications started on char1 (LED status)");
            emit_led_status(); /* push the current value right away */
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "StopNotify") == 0)
    {
        notifying = FALSE;
        log_info("GATT: notifications stopped on char1");
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
            G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method %s", method_name);
    }
}

static GVariant *char_notify_get_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)user_data;

    if (g_strcmp0(property_name, "UUID") == 0)
        return g_variant_new_string(GATT_CHAR_NOTIFY_UUID);
    if (g_strcmp0(property_name, "Service") == 0)
        return g_variant_new_object_path(SERVICE_PATH);
    if (g_strcmp0(property_name, "Flags") == 0)
    {
        const gchar *flags[] = { "read", "notify", NULL };
        return g_variant_new_strv(flags, -1);
    }
    if (g_strcmp0(property_name, "Notifying") == 0)
        return g_variant_new_boolean(notifying);

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Unknown property %s", property_name);
    return NULL;
}

static const GDBusInterfaceVTable char_notify_vtable = {
    .method_call = char_notify_method_call,
    .get_property = char_notify_get_property,
    .set_property = NULL
};

/* ---------------------------------------------------------------------
 * Registration
 * ------------------------------------------------------------------- */

static guint register_iface(GDBusConnection *conn, const gchar *path,
                             const gchar *xml, const GDBusInterfaceVTable *vtable)
{
    GError *error = NULL;
    GDBusNodeInfo *node = g_dbus_node_info_new_for_xml(xml, &error);

    if (!node)
    {
        log_error("Introspection parse failed for %s: %s", path, error->message);
        g_error_free(error);
        return 0;
    }

    guint id = g_dbus_connection_register_object(
        conn, path, node->interfaces[0], vtable, NULL, NULL, &error);

    if (id == 0)
    {
        log_error("Failed to register %s: %s", path, error->message);
        g_error_free(error);
    }

    /* node is intentionally kept alive (not freed) for the life of the
     * process - register_object needs interface_info to stay valid. */
    return id;
}

static void register_application_cb(GObject *source, GAsyncResult *res, gpointer user_data)
{
    (void)user_data;
    GError *error = NULL;

    GVariant *result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &error);

    if (error)
    {
        log_error("RegisterApplication failed: %s", error->message);
        log_error("(Is the adapter powered on? Try: bluetoothctl power on)");
        g_error_free(error);
        return;
    }

    log_info("GATT application registered with BlueZ");
    g_variant_unref(result);
}

void gatt_server_init(void)
{
    gatt_conn = dbus_get_system_connection();
    if (!gatt_conn)
    {
        log_error("GATT server: no D-Bus connection");
        return;
    }

    register_iface(gatt_conn, APP_PATH, object_manager_xml, &object_manager_vtable);
    register_iface(gatt_conn, SERVICE_PATH, gatt_service_xml, &gatt_service_vtable);
    register_iface(gatt_conn, CHAR_RW_PATH, char_rw_xml, &char_rw_vtable);
    register_iface(gatt_conn, CHAR_NOTIFY_PATH, char_notify_xml, &char_notify_vtable);

    GVariantBuilder empty_options;
    g_variant_builder_init(&empty_options, G_VARIANT_TYPE("a{sv}"));

    g_dbus_connection_call(
        gatt_conn,
        "org.bluez",
        ADAPTER_PATH,
        "org.bluez.GattManager1",
        "RegisterApplication",
        g_variant_new("(oa{sv})", APP_PATH, &empty_options),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        register_application_cb,
        NULL);

    /* LED blinks 3s on / 3s off for the whole lifetime of the process -
     * notifications to a subscribed BLE client happen as a side effect
     * inside led_toggle_timeout_cb(), not the other way around. */
    g_timeout_add_seconds(3, led_toggle_timeout_cb, NULL);
}
