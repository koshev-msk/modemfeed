/*
 * Freescale i.MX233/i.MX28 USB loader
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <libusb.h>

#define TRANSFER_TIMEOUT_MS		60000
#define HID_SET_REPORT			0x09
#define HID_REPORT_TYPE_OUTPUT		0x02

#define MX23_VID			0x066f
#define MX23_PID			0x3780
#define MX28_VID			0x15a2
#define MX28_PID			0x004f

#define BLTC_CMD_INQUIRY		0x01
#define BLTC_CMD_DOWNLOAD_FIRMWARE	0x02

#define BLTC_PAGE_INQUIRY_CHIP_INFO	0x01

struct page_inquiry_chip_info {
	uint16_t	chip_id;
	uint8_t		__pad;
	uint8_t		chip_rev;
	uint16_t	rom_ver;
	uint16_t	proto_ver;
};

static int get_mxs_dev(libusb_device_handle **h)
{
	int i, cnt, ret;
	struct libusb_device_descriptor desc;
	libusb_device **devs;
	libusb_device *rdev = NULL;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		return -EINVAL;

	for (i = 0; i < cnt; i++) {
		if (!devs[i])
			continue;

		ret = libusb_get_device_descriptor(devs[i], &desc);
		if (ret < 0)
			continue;

		if (desc.idVendor == MX23_VID && desc.idProduct == MX23_PID) {
			rdev = devs[i];
			break;
		}

		if (desc.idVendor == MX28_VID && desc.idProduct == MX28_PID) {
			rdev = devs[i];
			break;
		}
	}

	if (!rdev) {
		fprintf(stderr, "No compatible device found.\n");
		ret = -ENODEV;
		goto exit;
	}

	ret = libusb_open(rdev, h);
	if (ret)
		fprintf(stderr, "Could not open device, ret=%i\n", ret);
exit:
	libusb_free_device_list(devs, 1);
	return ret;
}

static int transfer(struct libusb_device_handle *h, int report,
			unsigned char *buf, unsigned cnt, int *offset)
{
	int ret;
	int last_trans = 0;
	const int control_transfer =
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS |
		LIBUSB_RECIPIENT_INTERFACE;
	const int interrupt_transfer =
		LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN;

	if (report < 3) {
		ret = libusb_control_transfer(h, control_transfer,
				HID_SET_REPORT,
				(HID_REPORT_TYPE_OUTPUT << 8) | report,
				0, buf, cnt, TRANSFER_TIMEOUT_MS);
		last_trans = (ret > 0) ? ret - 1 : 0;
		if (ret > 0)
			ret = 0;
	} else {
		if (cnt > 64)
			cnt = 64;
		ret = libusb_interrupt_transfer(h, interrupt_transfer,
				buf, cnt, &last_trans,
				TRANSFER_TIMEOUT_MS);
	}

	*offset += last_trans;

	return ret;
}

static int detect_mxs_cpu(libusb_device_handle *h)
{
	int ret;
	uint8_t buf[1025];
	uint8_t fin[14];
	int last_trans = 0;
	const int len = sizeof(buf);
	struct page_inquiry_chip_info *info;
	info = (struct page_inquiry_chip_info *)(buf + 1);

	const uint8_t inquiry_chip_info[] = {
		BLTC_CMD_INQUIRY, 'B', 'L', 'T', 'C', 0, 0, 0,
		0, 0x04, 0, 0,
		0, 0x80, 0, 0,

		BLTC_CMD_INQUIRY, BLTC_PAGE_INQUIRY_CHIP_INFO, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
	};

	ret = transfer(h, 1, (uint8_t *)inquiry_chip_info,
			sizeof(inquiry_chip_info), &last_trans);
	if (ret)
		return ret;

	memset(buf, 0, sizeof(buf));

	last_trans = 0;
	do {
		ret = transfer(h, 3, buf + last_trans, len, &last_trans);
		if (ret)
			return ret;
	} while (last_trans < len);

	last_trans = 0;
	ret = transfer(h, 4, fin, sizeof(fin), &last_trans);
	if (ret)
		return ret;

	if ((fin[0] != 0x04) || memcmp(fin + 1, "BLTS", 4))
		return -EINVAL;

	printf("Detected: i.MX%i\n"
		"Chip ID:          0x%04x\n"
		"Chip Revision:    0x%04x\n"
		"ROM Version:      0x%04x\n"
		"Protocol Version: 0x%04x\n",
		(info->chip_id == 0x2800) ? 28 : 23,
		info->chip_id, info->chip_rev,
		info->rom_ver, info->proto_ver);

	return 0;
}



static int upload_firmware(char *fn, libusb_device_handle *h)
{
	FILE *f;
	long len, tmplen;
	uint8_t buf[1025] = { BLTC_CMD_DOWNLOAD_FIRMWARE };
	int ret = 0;
	int last_trans = 0;

	uint8_t inquiry_download_fw[] = {
		BLTC_CMD_INQUIRY, 'B', 'L', 'T', 'C', 1, 0, 0,
		0, 0x60, 0x24, 0x4a,
		0, 0, 0, 0,

		BLTC_CMD_DOWNLOAD_FIRMWARE, 0, 0x4a, 0x24,
		0x60, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
	};

	f = fopen(fn, "rb");
	if (!f) {
		fprintf(stderr, "Failed to open firmware (%s)\n", fn);
		return -EINVAL;
	}

	ret = fseek(f, 0, SEEK_END);
	if (ret)
		goto exit;

	len = ftell(f);
	if (len <= 0)
		goto exit;
	tmplen = __builtin_bswap32(len);

	memcpy(inquiry_download_fw + 9, &len, 4);
	memcpy(inquiry_download_fw + 17, &tmplen, 4);

	ret = fseek(f, 0, SEEK_SET);
	if (ret)
		goto exit;

	ret = transfer(h, 1, (uint8_t *)inquiry_download_fw,
			sizeof(inquiry_download_fw), &last_trans);
	if (ret) {
		fprintf(stderr,
			"An Error occured while transfering the firmware through USB.\n");
		goto exit;
	}

	while ((len = fread(buf + 1, 1, sizeof(buf) - 1, f))) {
		buf[0] = BLTC_CMD_DOWNLOAD_FIRMWARE;
		ret = transfer(h, 2, buf, len + 1, &last_trans);
		if (ret)
			goto exit;
	}

exit:
	fclose(f);
	return ret;
}

void print_usage()
{
	printf(
		"Usage: mxsldr <bootstream>\n"
		"              (e.g: u-boot.sb)\n");
}

int main(int argc, char const *const argv[])
{
	int ret;
	libusb_device_handle *h = NULL;

	/* Detect and exit. */
	if (argc != 2) {
		print_usage();
		return 1;
	}

	ret = libusb_init(NULL);
	if (ret < 0)
		goto out;

	ret = get_mxs_dev(&h);
	if (ret)
		goto out;
	if (!h)
		goto out;

	if (libusb_kernel_driver_active(h, 0))
		libusb_detach_kernel_driver(h, 0);

	ret = libusb_claim_interface(h, 0);
	if (ret) {
		fprintf(stderr, "Failed to claim interface\n");
		goto out;
	}

	ret = detect_mxs_cpu(h);
	if (ret) {
		fprintf(stderr, "Failed to detect CPU\n");
		goto exit;
	}

	ret = upload_firmware((char *)argv[1], h);
	if (ret)
		goto exit;

exit:
	libusb_release_interface(h, 0);
out:
	if (h)
		libusb_close(h);

	libusb_exit(NULL);
	return ret;
}
