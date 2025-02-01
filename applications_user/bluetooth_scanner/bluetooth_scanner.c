#include "bluetooth_scanner.h"
#include <notification/notification_messages.h>

#define TAG "BtScanner"

static void bluetooth_scanner_draw_callback(Canvas* canvas, void* ctx) {
    BluetoothScannerApp* app = ctx;
    furi_mutex_acquire(app->mutex, FuriWaitForever);
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "BT Devices Found:");
    
    if(app->scanning) {
        canvas_draw_str(canvas, 80, 10, "Scanning...");
    } else {
        canvas_draw_str(canvas, 80, 10, "Stopped");
    }
    
    canvas_set_font(canvas, FontSecondary);
    int y = 25;
    for(uint32_t i = 0; i < app->devices_found; i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Device %ld", i+1);
        canvas_draw_str(canvas, 5, y, buffer);
        y += 10;
        if(y >= 64) break;
    }
    
    furi_mutex_release(app->mutex);
}

static void bluetooth_device_found_callback(BluetoothDevice* device, void* context) {
    BluetoothScannerApp* app = context;
    if(device->name != NULL && strlen(device->name) > 0) {
        FURI_LOG_I(TAG, "Found device: %s RSSI: %d", device->name, device->rssi);
        furi_mutex_acquire(app->mutex, FuriWaitForever);
        app->devices_found++;
        furi_mutex_release(app->mutex);
    }
}

static bool bluetooth_scanner_input_callback(InputEvent* event, void* ctx) {
    BluetoothScannerApp* app = ctx;
    if(event->type == InputTypePress) {
        if(event->key == InputKeyBack) {
            return false;
        } else if(event->key == InputKeyOk) {
            BluetoothScannerEvent event = {.type = BluetoothScannerEventTypeScan};
            furi_message_queue_put(app->event_queue, &event, FuriWaitForever);
        }
    }
    return true;
}

void bluetooth_scanner_app_update_list(BluetoothScannerApp* app) {
    widget_reset(app->widget);
    widget_add_string_element(
        app->widget, 0, 0, AlignLeft, AlignTop, FontPrimary, "Bluetooth Devices");
    
    // Aquí se actualizaría la lista con los dispositivos encontrados
}

int32_t bluetooth_scanner_app(void* p) {
    BluetoothScannerApp* app = malloc(sizeof(BluetoothScannerApp));
    
    app->gui = furi_record_open(RECORD_GUI);
    app->bt = furi_record_open(RECORD_BLUETOOTH);
    app->event_queue = furi_message_queue_alloc(8, sizeof(BluetoothScannerEvent));
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    app->view_dispatcher = view_dispatcher_alloc();
    app->widget = widget_alloc();
    
    widget_set_draw_callback(app->widget, bluetooth_scanner_draw_callback, app);
    widget_set_input_callback(app->widget, bluetooth_scanner_input_callback, app);
    
    view_dispatcher_add_view(app->view_dispatcher, 0, widget_get_view(app->widget));
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    
    app->scanning = false;
    app->devices_found = 0;
    
    BluetoothScannerEvent event;
    while(1) {
        FuriStatus status = furi_message_queue_get(app->event_queue, &event, 100);
        
        if(status == FuriStatusOk) {
            if(event.type == BluetoothScannerEventTypeScan) {
                if(!app->scanning) {
                    app->scanning = true;
                    app->devices_found = 0;
                    furi_hal_bt_start_discovery(
                        BLE_SCAN_FAST, bluetooth_device_found_callback, app);
                }
            }
        }
        
        if(app->scanning) {
            notification_message(furi_record_open(RECORD_NOTIFICATION), &sequence_blink_cyan_100);
        }
        
        view_dispatcher_update(app->view_dispatcher);
    }
    
    furi_hal_bt_stop_discovery();
    view_dispatcher_remove_view(app->view_dispatcher, 0);
    widget_free(app->widget);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_BLUETOOTH);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->mutex);
    free(app);
    
    return 0;
}
