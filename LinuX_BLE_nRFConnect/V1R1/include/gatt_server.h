#ifndef GATT_SERVER_H
#define GATT_SERVER_H

#include <stdint.h>

/* Custom 128-bit test service. Change these if you want your own UUIDs -
 * any random v4 UUID works, nRF Connect will just show them as "Unknown". */
#define GATT_SERVICE_UUID      "12345678-1234-5678-1234-56789abcdef0"
#define GATT_CHAR_RW_UUID      "12345678-1234-5678-1234-56789abcdef1"
#define GATT_CHAR_NOTIFY_UUID  "12345678-1234-5678-1234-56789abcdef2"

/* Current LED state (0 = off, 1 = on). Toggled every 3s by the timer
 * started in gatt_server_init(). On real hardware, swap the placeholder
 * comment inside led_toggle_timeout_cb() for a real GPIO write. */
extern uint8_t gLed;

/* Registers one GATT service with two characteristics:
 *   - char0 : read + write   (echoes back whatever was last written)
 *   - char1 : read + notify  (current LED status - toggles 3s on/3s off,
 *                             notifies subscribed clients on every change)
 * with BlueZ over D-Bus, so it shows up under GattServices when a phone
 * (e.g. nRF Connect) connects to this device. */
void gatt_server_init(void);

#endif
