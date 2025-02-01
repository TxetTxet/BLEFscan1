#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/widget.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gap.h>

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Widget* widget;
    Bluetooth* bt;
    FuriMessageQueue* event_queue;
    FuriMutex* mutex;
    bool scanning;
    uint32_t devices_found;
} BluetoothScannerApp;

typedef enum {
    BluetoothScannerEventTypeTick,
    BluetoothScannerEventTypeScan,
    BluetoothScannerEventTypeStop
} BluetoothScannerEventType;

typedef struct {
    BluetoothScannerEventType type;
} BluetoothScannerEvent;

void bluetooth_scanner_app_update_list(BluetoothScannerApp* app);
