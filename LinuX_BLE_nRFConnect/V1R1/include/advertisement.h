#ifndef ADVERTISEMENT_H
#define ADVERTISEMENT_H

/* Registers a simple LE advertisement (advertising our service UUID +
 * a local name) via BlueZ's LEAdvertisingManager1, so the device is
 * visible and connectable to scanners like nRF Connect. */
void advertisement_init(void);

#endif
