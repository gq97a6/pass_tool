#include "hid.h"

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"

//TinyUSB descriptors ----------------------------------------------------------------------------------------------------

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

//In this example we implement Keyboard + Mouse HID device,
//so we must define both report descriptors
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD) ),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE) )
};

//String descriptor
const char* hid_string_descriptor[5] = {
    //array of pointer to string descriptors
    (char[]){0x09, 0x04},  //0: is supported language is English (0x0409)
    "MyCustomManufacturer",             //1: Manufacturer
    "MyCustomProduct",      //2: Product
    "692137420",              //3: Serials, should use chip ID
    "Example HID interface",  //4: HID
};

//This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
static const uint8_t hid_configuration_descriptor[] = {
    //Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    //Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

//TinyUSB HID callbacks ----------------------------------------------------------------------------------------------------

//Invoked when received GET_HID_REPORT_DESCRIPTOR control request
//Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    //We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

//Invoked when received GET_REPORT control request
//Application must fill buffer report's content and return its length.
//Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

//Invoked when received SET_REPORT control request or
//received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

//Application ----------------------------------------------------------------------------------------------------

void sendString(char* payload, int len) {
    static const uint8_t conv_table[128][2] = { HID_ASCII_TO_KEYCODE };
    uint8_t keymod = 0;
    
    for(int i = 0; i < len; i++) {
        //Check if shift is required
        if (conv_table[(int)payload[i]][0]) keymod = KEYBOARD_MODIFIER_LEFTSHIFT;
        else keymod = 0;

        //Set key
        uint8_t keycode[6] = {0};
        keycode[0] = conv_table[(int)payload[i]][1];

        if (tud_mounted()) tud_hid_keyboard_report(
                HID_ITF_PROTOCOL_KEYBOARD, 
                keymod,
                keycode
            );
        else return;

        vTaskDelay(pdMS_TO_TICKS(40));
    }

    //New line
    uint8_t keycode[6] = {HID_KEY_ENTER};
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
    vTaskDelay(pdMS_TO_TICKS(10));

    //Realese all keys
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void initializeHID() {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}