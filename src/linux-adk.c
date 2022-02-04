/*
 * Linux ADK - linux-adk.c
 *
 * Copyright (C) 2013 - Gary Bisson <bisson.gary@gmail.com>
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
#include <signal.h>

#include <libusb.h>

#include "linux-adk.h"

extern void accessory_main(accessory_t * acc);

volatile int stop_acc = 0;
int verbose = 0;

static const accessory_t acc_default = {
	.device = "18d1:4ee7",
	.manufacturer = "Google, Inc.",
	.model = "AccessoryChat",
	.description = "Sample Program",
	.version = "1.0",
	.url = "https://github.com/gibsson",
	.serial = "0000000012345678",
};

static int is_accessory_present(accessory_t * acc);
static int init_accessory(accessory_t * acc, int aoa_max_version);
static void fini_accessory(accessory_t * acc);
static int iden_accessory(accessory_t * acc);
static void print_descriptor(struct libusb_device_handle *handle); 


static void show_help(char *name)
{
	printf
	    ("Linux Accessory Development Kit\n\nusage: %s [OPTIONS]\nOPTIONS:\n"
	     "\t-a, --aoa-max-version\n\t\tAOA maximum version to be used. "
	     "Default is no maximum version.\n"
	     "\t-d, --device\n\t\tUSB device product and vendor IDs. "
	     "Default is \"%s\".\n"
	     "\t-D, --description\n\t\taccessory description. "
	     "Default is \"%s\".\n"
	     "\t-m, --manufacturer\n\t\tmanufacturer's name. "
	     "Default is \"%s\".\n"
	     "\t-M, --model\n\t\tmodel's name. "
	     "Default is \"%s\".\n"
	     "\t-n, --vernumber\n\t\taccessory version number. "
	     "Default is \"%s\".\n"
	     "\t-N, --no_app\n\t\toption that allows to connect without an "
	     "Android App (AOA v2.0 only, for Audio and HID).\n"
	     "\t-s, --serial\n\t\tserial numder. "
	     "Default is \"%s\".\n"
	     "\t-u, --url\n\t\taccessory url. "
	     "Default is \"%s\".\n"
	     "\t-v, --version\n\t\tShow program version and exit.\n"
	     "\t-V, --verbose\n\t\tSets libusb verbose mode.\n"
	     "\t-h, --help\n\t\tShow this help and exit.\n", name,
	     acc_default.device, acc_default.description,
	     acc_default.manufacturer, acc_default.model, acc_default.version,
	     acc_default.serial, acc_default.url);
	return;
}

static void show_version(char *name)
{
	printf("%s v%s\nreport bugs to %s\n", name, PACKAGE_VERSION,
	       PACKAGE_BUGREPORT);
	return;
}

static void signal_handler(int signo)
{
	printf("SIGINT: Closing accessory\n");
	stop_acc = 1;
}

int main(int argc, char *argv[])
{
	int arg_count = 1;
	int no_app = 0;
	int aoa_max_version = -1;
	accessory_t acc = { NULL, NULL, 0, 0, 0, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL
	};

	if (signal(SIGINT, signal_handler) == SIG_ERR)
		printf("Cannot setup a signal handler...\n");

	/* Disable buffering on stdout */
	setbuf(stdout, NULL);

	/* Parse all parameters */
	while (arg_count < argc) {
		if ((strcmp(argv[arg_count], "-a") == 0)
		    || (strcmp(argv[arg_count], "--aoa-max-version") == 0)) {
			aoa_max_version= atoi(argv[++arg_count]);
		} else if ((strcmp(argv[arg_count], "-d") == 0)
			   || (strcmp(argv[arg_count], "--device") == 0)) {
			acc.device = argv[++arg_count];
		} else if ((strcmp(argv[arg_count], "-D") == 0)
			   || (strcmp(argv[arg_count], "--description")
			       == 0)) {
			acc.description = argv[++arg_count];
		} else if ((strcmp(argv[arg_count], "-m") == 0)
			   || (strcmp(argv[arg_count], "--manufacturer")
			       == 0)) {
			acc.manufacturer = argv[++arg_count];
		} else if ((strcmp(argv[arg_count], "-M") == 0)
			   || (strcmp(argv[arg_count], "--model") == 0)) {
			acc.model = argv[++arg_count];
		} else if ((strcmp(argv[arg_count], "-n") == 0)
			   || (strcmp(argv[arg_count], "--versionnumber")
			       == 0)) {
			acc.version = argv[++arg_count];
		} else if ((strcmp(argv[arg_count], "-N") == 0)
			   || (strcmp(argv[arg_count], "--no_app") == 0)) {
			no_app = 1;
		} else if ((strcmp(argv[arg_count], "-s") == 0)
			   || (strcmp(argv[arg_count], "--serial") == 0)) {
			acc.serial = argv[++arg_count];
		} else if ((strcmp(argv[arg_count], "-u") == 0)
			   || (strcmp(argv[arg_count], "--url") == 0)) {
			acc.url = argv[++arg_count];
		} else if ((strcmp(argv[arg_count], "-v") == 0)
			   || (strcmp(argv[arg_count], "--version") == 0)) {
			show_version(argv[0]);
			exit(1);
		} else if ((strcmp(argv[arg_count], "-V") == 0)
			   || (strcmp(argv[arg_count], "--verbose") == 0)) {
			verbose = 1;
		} else {
			show_help(argv[0]);
			exit(1);
		}
		arg_count++;
	}

	/* Use of default values if not set */
	if (!acc.device)
		acc.device = acc_default.device;
	if (!acc.description)
		acc.description = acc_default.description;
	if (!acc.manufacturer && !no_app)
		acc.manufacturer = acc_default.manufacturer;
	if (!acc.model && !no_app)
		acc.model = acc_default.model;
	if (!acc.version)
		acc.version = acc_default.version;
	if (!acc.serial)
		acc.serial = acc_default.serial;
	if (!acc.url)
		acc.url = acc_default.url;

	if (init_accessory(&acc, aoa_max_version) != 0)
		goto end;

	accessory_main(&acc);

end:
	fini_accessory(&acc);
	return 0;
}

static int init_accessory(accessory_t * acc, int aoa_max_version)
{
	int ret;
	uint16_t pid, vid;
	char *tmp;
	uint8_t buffer[2];
	int tries = 10;

	/* Initializing libusb */
	ret = libusb_init(NULL);
	if (ret != 0) {
		printf("libusb init failed: %d\n", ret);
		return ret;
	}
	if (verbose)
		libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

	/* Check if device is not already in accessory mode */
	if (is_accessory_present(acc))
		return 0;

	/* Getting product and vendor IDs */
	vid = (uint16_t) strtol(acc->device, &tmp, 16);
	pid = (uint16_t) strtol(tmp + 1, &tmp, 16);
	printf("Looking for device %4.4x:%4.4x\n", vid, pid);

	/* Trying to open it */
	acc->handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (acc->handle == NULL) {
		printf("Unable to open device...\n");
		return -1;
	}

    if (verbose)
        print_descriptor(acc->handle);

	/* Now asking if device supports Android Open Accessory protocol */
	ret = libusb_control_transfer(acc->handle,
				      LIBUSB_ENDPOINT_IN |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_GET_PROTOCOL, 0, 0, buffer,
				      sizeof(buffer), 0);
	if (ret < 0) {
		printf("Error getting protocol...\n");
		return ret;
	} else {
		acc->aoa_version = ((buffer[1] << 8) | buffer[0]);
		printf("Device supports AOA %d.0!\n", acc->aoa_version);
	}
	if ((aoa_max_version > 0) && ((int)acc->aoa_version > aoa_max_version)) {
		acc->aoa_version = aoa_max_version;
		printf("Limiting AOA to version %d.0!\n", acc->aoa_version);
	}

	/* Some Android devices require a waiting period between transfer calls */
	usleep(10000);

	/* In case of a no_app accessory, the version must be >= 2 */
	if ((acc->aoa_version < 2) && !acc->manufacturer) {
		printf("Connecting without an Android App only for AOA 2.0\n");
		return -1;
	}

    if (iden_accessory(acc) < 0) {
        printf("Error identifying accessory\n");
        goto error;
    }

	printf("Turning the device in Accessory mode\n");
	ret = libusb_control_transfer(acc->handle,
				      LIBUSB_ENDPOINT_OUT |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_START_ACCESSORY, 0, 0, NULL, 0, 0);
	if (ret < 0)
		goto error;

    libusb_close(acc->handle);

	/* Let some time for the new enumeration to happen */
	usleep(10000);

	/* Connect to the Accessory */
	while (tries--) {
		if (is_accessory_present(acc))
			break;
		else if (!tries)
			goto error;
		else
			sleep(1);
	}

	return 0;

error:
	printf("Accessory init failed: %d\n", ret);
	return -1;
}

static int is_accessory_present(accessory_t * acc)
{
	struct libusb_device_handle *handle;
	uint16_t vid = AOA_ACCESSORY_VID;
	uint16_t pid = 0;

	/* Trying to open all the AOA IDs possible */
	pid = AOA_ACCESSORY_PID;
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle != NULL)
		goto claim;

	pid = AOA_ACCESSORY_ADB_PID;
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle != NULL)
		goto claim;

	pid = AOA_AUDIO_PID;
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle != NULL)
		goto claim;

	pid = AOA_AUDIO_ADB_PID;
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle != NULL)
		goto claim;

	pid = AOA_ACCESSORY_AUDIO_PID;
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle != NULL)
		goto claim;

	pid = AOA_ACCESSORY_AUDIO_ADB_PID;
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle != NULL)
		goto claim;

	return 0;

claim:
	printf("Found accessory %4.4x:%4.4x\n", vid, pid);
	acc->handle = handle;
	acc->vid = vid;
	acc->pid = pid;
    if (verbose)
        print_descriptor(acc->handle);
    if (iden_accessory(acc) < 0)
        return 0;
	return 1;
}

static void fini_accessory(accessory_t * acc)
{
	printf("Closing USB device\n");

	if (acc->handle != NULL) {
		libusb_release_interface(acc->handle, 0);
		libusb_close(acc->handle);
	}

	libusb_exit(NULL);

	return;
}

static int iden_accessory(accessory_t * acc)
{
	printf("Sending identification to the device\n");

    int ret;

	if (acc->manufacturer) {
        if (verbose)
            printf(" sending manufacturer: %s\n", acc->manufacturer);
		ret = libusb_control_transfer(acc->handle,
					      LIBUSB_ENDPOINT_OUT
					      | LIBUSB_REQUEST_TYPE_VENDOR,
					      AOA_SEND_IDENT, 0,
					      AOA_STRING_MAN_ID,
					      (uint8_t *) acc->manufacturer,
					      strlen(acc->manufacturer) + 1, 0);
		if (ret < 0)
			return ret;
	}

	if (acc->model) {
        if (verbose)
            printf(" sending model: %s\n", acc->model);
		ret = libusb_control_transfer(acc->handle,
					      LIBUSB_ENDPOINT_OUT
					      | LIBUSB_REQUEST_TYPE_VENDOR,
					      AOA_SEND_IDENT, 0,
					      AOA_STRING_MOD_ID,
					      (uint8_t *) acc->model,
					      strlen(acc->model) + 1, 0);
		if (ret < 0)
			return ret;
	}

    if (verbose)
        printf(" sending description: %s\n", acc->description);
	ret = libusb_control_transfer(acc->handle,
				      LIBUSB_ENDPOINT_OUT |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_SEND_IDENT, 0, AOA_STRING_DSC_ID,
				      (uint8_t *) acc->description,
				      strlen(acc->description) + 1, 0);
	if (ret < 0)
		return ret;

    if (verbose)
        printf(" sending version: %s\n", acc->version);
	ret = libusb_control_transfer(acc->handle,
				      LIBUSB_ENDPOINT_OUT |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_SEND_IDENT, 0, AOA_STRING_VER_ID,
				      (uint8_t *) acc->version,
				      strlen(acc->version) + 1, 0);
	if (ret < 0)
		return ret;

    if (verbose)
        printf(" sending url: %s\n", acc->url);
	ret = libusb_control_transfer(acc->handle,
				      LIBUSB_ENDPOINT_OUT |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_SEND_IDENT, 0, AOA_STRING_URL_ID,
				      (uint8_t *) acc->url,
				      strlen(acc->url) + 1, 0);
	if (ret < 0)
		return ret;

    if (verbose)
        printf(" sending serial number: %s\n", acc->serial);
	ret = libusb_control_transfer(acc->handle,
				      LIBUSB_ENDPOINT_OUT |
				      LIBUSB_REQUEST_TYPE_VENDOR,
				      AOA_SEND_IDENT, 0, AOA_STRING_SER_ID,
				      (uint8_t *) acc->serial,
				      strlen(acc->serial) + 1, 0);
	if (ret < 0)
		return ret;

	if (acc->aoa_version >= 2) {
        if (verbose)
            printf(" asking for audio support\n");
		ret = libusb_control_transfer(acc->handle,
					      LIBUSB_ENDPOINT_OUT
					      | LIBUSB_REQUEST_TYPE_VENDOR,
					      AOA_AUDIO_SUPPORT, 1, 0, 0, 0, 0);
		if (ret < 0)
			return ret;
	}

    return 0;
}

void print_descriptor(libusb_device_handle *handle)
{
    libusb_device *device = libusb_get_device(handle);

    struct libusb_device_descriptor desc;
    int result = libusb_get_device_descriptor(device, &desc);
    if (result < 0) {
        printf("Could not get descriptor...\n");
    }

    char buf[128];
    result = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, (unsigned char *) buf, sizeof(buf));
    if (result >= 0 && (size_t) result <= sizeof(buf)) {
        char *dev_ser = malloc(result + 1);
        memcpy(dev_ser, buf, result);
        dev_ser[result] = '\0';
        printf("Device serial: %s\n", dev_ser);
        free(dev_ser);
    }

    for (uint8_t i = 0; i < desc.bNumConfigurations; i++) {
        struct libusb_config_descriptor *config;
        result = libusb_get_config_descriptor(device, i, &config);
        if (LIBUSB_SUCCESS != result) {
            printf("Error getting config descriptor\n");
            continue;
        }

        printf("  Configuration:\n");
        printf("    wTotalLength:        %u\n", config->wTotalLength);
        printf("    bNumInterfaces:      %u\n", config->bNumInterfaces);
        printf("    bConfigurationValue: %u\n", config->bConfigurationValue);
        printf("    iConfiguration:      %u\n", config->iConfiguration);
        printf("    bmAttributes:        %02xh\n", config->bmAttributes);
        printf("    MaxPower:            %u\n", config->MaxPower);

        for (uint8_t j = 0; j < config->bNumInterfaces; j++) {
            struct libusb_interface iface = config->interface[j];
            for (int k = 0; k < iface.num_altsetting; k++) {
                struct libusb_interface_descriptor as = iface.altsetting[k];
                printf("    Interface:\n");
                printf("      bInterfaceNumber:   %u\n", as.bInterfaceNumber);
                printf("      bAlternateSetting:  %u\n", as.bAlternateSetting);
                printf("      bNumEndpoints:      %u\n", as.bNumEndpoints);
                printf("      bInterfaceClass:    %u\n", as.bInterfaceClass);
                printf("      bInterfaceSubClass: %u\n", as.bInterfaceSubClass);
                printf("      bInterfaceProtocol: %u\n", as.bInterfaceProtocol);
                printf("      iInterface:         %u\n", as.iInterface);

                for (uint8_t l = 0; l < config->interface[j].altsetting[k].bNumEndpoints; l++) {
                    struct libusb_endpoint_descriptor ep = as.endpoint[l];
                    printf("      Endpoint:\n");
                    printf("        bEndpointAddress: %02xh\n", ep.bEndpointAddress);
                    printf("        bmAttributes:     %02xh\n", ep.bmAttributes);
                    printf("        wMaxPacketSize:   %u\n", ep.wMaxPacketSize);
                    printf("        bInterval:        %u\n", ep.bInterval);
                    printf("        bRefresh:         %u\n", ep.bRefresh);
                    printf("        bSynchAddress:    %u\n", ep.bSynchAddress);

                    for (int m = 0; m < ep.extra_length;) {
                        if (LIBUSB_DT_SS_ENDPOINT_COMPANION == ep.extra[m + 1]) {
                            struct libusb_ss_endpoint_companion_descriptor *ep_comp;

                            result = libusb_get_ss_endpoint_companion_descriptor(NULL, &ep, &ep_comp);
                            if (LIBUSB_SUCCESS != result) {
                                continue;
                            }

                            printf("        USB 3.0 Endpoint Companion:\n");
                            printf("          bMaxBurst:         %u\n", ep_comp->bMaxBurst);
                            printf("          bmAttributes:      %02xh\n", ep_comp->bmAttributes);
                            printf("          wBytesPerInterval: %u\n", ep_comp->wBytesPerInterval);
                            
                            libusb_free_ss_endpoint_companion_descriptor(ep_comp);
                        }
                        m += ep.extra[i];
                    }
                }
            }
        }

        libusb_free_config_descriptor(config);
    }
}
