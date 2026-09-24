#include "dbus_helper.h"
#include "log.h"

static gchar *dbus_variant_to_string(GVariant *value)
{
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_VARIANT))
    {
        GVariant *inner = g_variant_get_variant(value);
        gchar *result = dbus_variant_to_string(inner);
        g_variant_unref(inner);
        return result;
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING) ||
        g_variant_is_of_type(value, G_VARIANT_TYPE_OBJECT_PATH) ||
        g_variant_is_of_type(value, G_VARIANT_TYPE_SIGNATURE))
    {
        return g_strdup(g_variant_get_string(value, NULL));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN))
    {
        return g_strdup(g_variant_get_boolean(value) ? "true" : "false");
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_BYTE))
    {
        return g_strdup_printf("0x%02x", g_variant_get_byte(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT16))
    {
        return g_strdup_printf("%" G_GINT16_FORMAT, g_variant_get_int16(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT16))
    {
        return g_strdup_printf("%" G_GUINT16_FORMAT, g_variant_get_uint16(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT32))
    {
        return g_strdup_printf("%" G_GINT32_FORMAT, g_variant_get_int32(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32))
    {
        return g_strdup_printf("%" G_GUINT32_FORMAT, g_variant_get_uint32(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT64))
    {
        return g_strdup_printf("%" G_GINT64_FORMAT, g_variant_get_int64(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT64))
    {
        return g_strdup_printf("%" G_GUINT64_FORMAT, g_variant_get_uint64(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE))
    {
        return g_strdup_printf("%g", g_variant_get_double(value));
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_BYTESTRING))
    {
        gsize length = 0;
        const guint8 *bytes = g_variant_get_fixed_array(value, &length, sizeof(guint8));
        GString *builder = g_string_new(NULL);

        for (gsize i = 0; i < length; ++i)
            g_string_append_printf(builder, "%02x", bytes[i]);

        gchar *output = g_string_free(builder, FALSE);
        return output;
    }

    if (g_variant_classify(value) == G_VARIANT_CLASS_ARRAY)
    {
        GVariantIter iter;
        GVariant *child;
        GString *builder = g_string_new("[");
        gboolean first = TRUE;

        g_variant_iter_init(&iter, value);
 

        while ((child = g_variant_iter_next_value(&iter)) != NULL)
        {
            gchar *child_text = dbus_variant_to_string(child);
            if (!first)
                g_string_append(builder, ", ");

            g_string_append(builder, child_text);
            g_free(child_text);
            g_variant_unref(child);
            first = FALSE;
        }

        g_string_append(builder, "]");
        return g_string_free(builder, FALSE);
    }

    return g_strdup(g_variant_print(value, TRUE));
}

void dbus_print_properties(GVariant *properties)
{
    if (properties == NULL)
        return;

    GVariantIter iter;
    const gchar *key;
    GVariant *value;

    g_variant_iter_init(&iter, properties);

    while (g_variant_iter_next(&iter, "{&sv}", &key, &value))
    {
        gchar *value_text = dbus_variant_to_string(value);
        log_info("  %s = %s", key, value_text);
        g_free(value_text);
        g_variant_unref(value);
    }
}

GDBusConnection *dbus_get_system_connection(void)
{
    GError *error = NULL;

    GDBusConnection *conn;

    conn = g_bus_get_sync(
                G_BUS_TYPE_SYSTEM,
                NULL,
                &error);

    if(error)
    {
        log_error("%s", error->message);
        g_error_free(error);
        return NULL;
    }

    log_info("Connected to System D-Bus");

    return conn;
}