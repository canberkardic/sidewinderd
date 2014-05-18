/*
 * Sidewinder daemon - used for supporting the special keys of Microsoft
 * Sidewinder X4 / X6 gaming keyboards.
 *
 * Copyright (c) 2014 Tolga Cakir <tolga@cevel.net>
 * 
 * Special Thanks to Filip Wieladek, Andreas Bader and Alan Ott. Without
 * these guys, I wouldn't be able to do anything.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <libudev.h>
#include <locale.h>

#include <linux/hidraw.h>
#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

/* VIDs & PIDs*/
#define VENDOR_ID_MICROSOFT			0x045e
#define PRODUCT_ID_SIDEWINDER_X6	0x074b
#define PRODUCT_ID_SIDEWINDER_X4	0x0768

/* device list */
#define DEVICE_SIDEWINDER_X6	0x00
#define DEVICE_SIDEWINDER_X4	0x01

/* constants */
#define MIN_PROFILE	0
#define MAX_PROFILE	2

/* special keys */
#define SKEY_S01	0x01
#define SKEY_S02	0x02
#define SKEY_S03	0x04
#define SKEY_S04	0x08
#define SKEY_S05	0x10
#define SKEY_S06	0x20
#define SKEY_S07	0x40
#define SKEY_S08	0x80
#define SKEY_S09	0x01 << 8
#define SKEY_S10	0x02 << 8
#define SKEY_S11	0x04 << 8
#define SKEY_S12	0x08 << 8
#define SKEY_S13	0x10 << 8
#define SKEY_S14	0x20 << 8
#define SKEY_S15	0x40 << 8
#define SKEY_S16	0x80 << 8
#define SKEY_S17	0x01 << 16
#define SKEY_S18	0x02 << 16
#define SKEY_S19	0x04 << 16
#define SKEY_S20	0x08 << 16
#define SKEY_S21	0x10 << 16
#define SKEY_S22	0x20 << 16
#define SKEY_S23	0x40 << 16
#define SKEY_S24	0x80 << 16
#define SKEY_S25	0x01 << 24
#define SKEY_S26	0x02 << 24
#define SKEY_S27	0x04 << 24
#define SKEY_S28	0x08 << 24
#define SKEY_S29	0x10 << 24
#define SKEY_S30	0x20 << 24

/* global variables */
volatile uint8_t active = 1;

/* sidewinder device status */
struct sidewinder_data {
	uint8_t device_id;
	uint8_t profile;
	uint8_t status;
	int32_t file_desc;
	const char *device_node;
};

void switch_profile(struct sidewinder_data *sw) {
	sw->profile++;

	if (sw->profile > MAX_PROFILE) {
		sw->profile = MIN_PROFILE;
	}
}

void cleanup(struct sidewinder_data *sw) {
	close(sw->file_desc);
	free(sw);
}

void handler() {
	active = 0;
}

void setup_udev(struct sidewinder_data *sw) {
	/* udev */
	struct udev *udev;
	struct udev_device *dev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

	char vid_microsoft[4], pid_sidewinder_x6[4], pid_sidewinder_x4[4];

	/* converting integers to strings */
	snprintf(vid_microsoft, sizeof(vid_microsoft) + 1, "%04x", VENDOR_ID_MICROSOFT);
	snprintf(pid_sidewinder_x6, sizeof(pid_sidewinder_x6) + 1, "%04x", PRODUCT_ID_SIDEWINDER_X6);
	snprintf(pid_sidewinder_x4, sizeof(pid_sidewinder_x4) + 1, "%04x", PRODUCT_ID_SIDEWINDER_X4);

	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		exit(1);
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path, *temp_path;

		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		temp_path = udev_device_get_devnode(dev);
		dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");

		if (strcmp(udev_device_get_sysattr_value(dev, "bInterfaceNumber"), "01") == 0) {
			dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

			if (strcmp(udev_device_get_sysattr_value(dev, "idVendor"), vid_microsoft) == 0) {
				if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x6) == 0) {
					sw->device_node = temp_path;
					sw->device_id = DEVICE_SIDEWINDER_X6;
				} else if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x4) == 0) {
					sw->device_node = temp_path;
					sw->device_id = DEVICE_SIDEWINDER_X4;
				}
			}
		}

		udev_device_unref(dev);
	}

	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
}

void setup_hidraw(struct sidewinder_data *sw) {
	sw->file_desc = open(sw->device_node, O_RDWR|O_NONBLOCK);

	if (sw->file_desc < 0) {
		printf("Can't open hidraw interface");
		exit(1);
	}
}

void process_input(uint8_t nbytes, unsigned char *buffer) {
	if (nbytes == 5 && buffer[0] == 8) {
		int i;

		for (i = 0; i < nbytes; i++) {
			printf("%2x ", buffer[i]);
		}
		puts("\n");
	} else if (nbytes = 8) {
		
	}
}

void listen_device(struct sidewinder_data *sw) {
	uint8_t nbytes;
	unsigned char buffer[8];
	int i;

	nbytes = read(sw->file_desc, buffer, 8);

	if (nbytes > 0) {
		process_input(nbytes, buffer);
	}
}

int main(int argc, char **argv) {
	struct sidewinder_data *sw;
	sw = calloc(4, sizeof(struct sidewinder_data));

	signal(SIGINT, handler);
	signal(SIGHUP, handler);
	signal(SIGTERM, handler);
	signal(SIGKILL, handler);

	setup_udev(sw);
	setup_hidraw(sw);

	while (active) {
		listen_device(sw);
		usleep(1);
	}

	cleanup(sw);

	exit(0);
}
