/*
 * Linux ADK - hid.c
 *
 * Copyright (C) 2014 - Gary Bisson <bisson.gary@gmail.com>
 *
 * Based on usbAccReadWrite.c by Jeremy Rosen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <libusb.h>

#include "linux-adk.h"
#include "hid.h"

/**
 * Mouse descriptor from the specification:
 * <https://www.usb.org/sites/default/files/hid1_11.pdf>
 *
 * Appendix E (p71): §E.10 Report Descriptor (Mouse)
 *
 * The usage tags (like Wheel) are listed in "HID Usage Tables":
 * <https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf>
 * §4 Generic Desktop Page (0x01) (p26)
 */
static const unsigned char mouse_report_desc[]  = {
    // Usage Page (Generic Desktop)
    0x05, 0x01,
    // Usage (Mouse)
    0x09, 0x02,

    // Collection (Application)
    0xA1, 0x01,

    // Usage (Pointer)
    0x09, 0x01,

    // Collection (Physical)
    0xA1, 0x00,

    // Usage Page (Buttons)
    0x05, 0x09,

    // Usage Minimum (1)
    0x19, 0x01,
     // Usage Maximum (5)
    0x29, 0x05,
    // Logical Minimum (0)
    0x15, 0x00,
    // Logical Maximum (1)
    0x25, 0x01,
    // Report Count (5)
    0x95, 0x05,
    // Report Size (1)
    0x75, 0x01,
    // Input (Data, Variable, Absolute): 5 buttons bits
    0x81, 0x02,

    // Report Count (1)
    0x95, 0x01,
    // Report Size (3)
    0x75, 0x03,
    // Input (Constant): 3 bits padding
    0x81, 0x01,

    // Usage Page (Generic Desktop)
    0x05, 0x01,
    // Usage (X)
    0x09, 0x30,
    // Usage (Y)
    0x09, 0x31,
    // Usage (Wheel)
    0x09, 0x38,
    // Local Minimum (-127)
    0x15, 0x81,
    // Local Maximum (127)
    0x25, 0x7F,
    // Report Size (8)
    0x75, 0x08,
    // Report Count (3)
    0x95, 0x03,
    // Input (Data, Variable, Relative): 3 position bytes (X, Y, Wheel)
    0x81, 0x06,

    // End Collection
    0xC0,

    // End Collection
    0xC0,
};

int send_hid_descriptor(accessory_t * acc)
{
	int ret;

	ret = libusb_control_transfer(acc->handle, LIBUSB_ENDPOINT_OUT |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_REGISTER_HID, 1, ARRAY_LEN(mouse_report_desc),
				      NULL, 0, 0);
	if (ret < 0) {
		printf("couldn't register HID device on the android device : %s\n",
		       libusb_error_name(ret));
		return -1;
	}

	ret = libusb_control_transfer(acc->handle, LIBUSB_ENDPOINT_OUT |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_SET_HID_REPORT_DESC, 1, 0,
				      (unsigned char *) mouse_report_desc, ARRAY_LEN(mouse_report_desc), 0);
	if (ret < 0) {
		printf("couldn't send HID descriptor to the android device\n");
		return -1;
	}


	return 0;
}

int send_hid_inputs(accessory_t *acc)
{
    unsigned char *buffer = calloc(1, 4);
    if (buffer) {
        buffer[0] = 0;
        buffer[1] = 10;
        buffer[2] = 10;
        buffer[3] = 0;

        int ret;
        for (uint8_t i = 0; i < 16; i++) {
            ret = libusb_control_transfer(acc->handle, LIBUSB_ENDPOINT_OUT |
                              LIBUSB_REQUEST_TYPE_VENDOR,
                              AOA_SEND_HID_EVENT, 1, 0,
                              buffer, 4, 0);
            if (ret < 0) {
                printf("couldn't send HID event %d\n", i);
                break;
            }
            sleep(1);
        }
    } else {
        printf("failed to allocate HID event buffer\n");
        return -1;
    }

    free(buffer);
    return 0;
}
