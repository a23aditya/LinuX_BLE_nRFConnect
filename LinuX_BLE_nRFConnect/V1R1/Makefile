CC = gcc

TARGET = ble_framework

SRC = \
src/main.c \
src/log.c \
src/dbus_helper.c \
src/adapter.c \
src/device_manager.c \
src/signal_handler.c \
src/gatt_server.c \
src/advertisement.c

CFLAGS = -Wall -Wextra -Iinclude

LIBS = $(shell pkg-config --cflags --libs gio-2.0)

all:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)