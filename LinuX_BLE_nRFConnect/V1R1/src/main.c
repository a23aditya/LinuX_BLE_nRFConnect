#include "common.h"

#include "adapter.h"
#include "device_manager.h"
#include "signal_handler.h"
#include "gatt_server.h"
#include "advertisement.h"

int main()
{
    GMainLoop *loop;

    device_manager_init();

    adapter_print_name();  // Prints the name of the Bluetooth adapter

    signal_handler_init();  // Initializes the signal handler to listen for D-Bus signals

    gatt_server_init();    // Registers our GATT service (read/write + notify) with BlueZ
    advertisement_init();  // Starts advertising so nRF Connect (or any scanner) can find us

    adapter_start_discovery(); // Starts the Bluetooth discovery process

    loop = g_main_loop_new(NULL,FALSE);  // Creates a new GMainLoop instance to handle events

    g_main_loop_run(loop);  // Runs the main loop, which will block and process events until g_main_loop_quit() is called

    g_main_loop_unref(loop);  // Cleans up and frees the GMainLoop instance

    return 0;
}