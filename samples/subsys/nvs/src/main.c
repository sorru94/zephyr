/*
 * NVS Sample for Zephyr using high level API, the sample illustrates the usage
 * of NVS for storing data of different kind (strings, binary blobs, unsigned
 * 32 bit integer) and also how to read them back from flash. The reading of
 * data is illustrated for both a basic read (latest added value) as well as
 * reading back the history of data (previously added values). Next to reading
 * and writing data it also shows how data can be deleted from flash.
 *
 * The sample stores the following items:
 * 1. A string representing an IP-address: stored at id=1, data="192.168.1.1"
 * 2. A binary blob representing a key: stored at id=2, data=FF FE FD FC FB FA
 *    F9 F8
 * 3. A reboot counter (32bit): stored at id=3, data=reboot_counter
 * 4. A string: stored at id=4, data="DATA" (used to illustrate deletion of
 * items)
 *
 * At first boot the sample checks if the data is available in flash and adds
 * the items if they are not in flash.
 *
 * Every reboot increases the values of the reboot_counter and updates it in
 * flash.
 *
 * At the 10th reboot the string item with id=4 is deleted (or marked for
 * deletion).
 *
 * At the 11th reboot the string item with id=4 can no longer be read with the
 * basic nvs_read() function as it has been deleted. It is possible to read the
 * value with nvs_read_hist()
 *
 * At the 78th reboot the first sector is full and a new sector is taken into
 * use. The data with id=1, id=2 and id=3 is copied to the new sector. As a
 * result of this the history of the reboot_counter will be removed but the
 * latest values of address, key and reboot_counter is kept.
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */


 #include <zephyr/kernel.h>
 #include <zephyr/sys/reboot.h>
 #include <zephyr/device.h>
 #include <string.h>
 #include <zephyr/drivers/flash.h>
 #include <zephyr/storage/flash_map.h>
 #include <zephyr/fs/nvs.h>

 #define NVS_PARTITION			storage_partition
 #define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
 #define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)
 #define NVS_PARTITION_SIZE		FIXED_PARTITION_SIZE(NVS_PARTITION)

 int main(void)
 {
	 int flash_rc = flash_erase(NVS_PARTITION_DEVICE, NVS_PARTITION_OFFSET, NVS_PARTITION_SIZE);
	 if (flash_rc) {
		 printk("Flash erase failure: %s (%d).\n", strerror(-flash_rc), flash_rc);
		 return -1;
	 }

	 const struct device *flash_device = NVS_PARTITION_DEVICE;
	 if (!device_is_ready(flash_device)) {
		 printk("Flash device %s not ready.\n", flash_device->name);
		 return -1;
	 }

	 struct flash_pages_info fp_info = { 0 };
	 off_t flash_offset = NVS_PARTITION_OFFSET;
	 flash_rc = flash_get_page_info_by_offs(flash_device, flash_offset, &fp_info);
	 if (flash_rc) {
		 printk("Unable to get page info: %d.\n", flash_rc);
		 return -1;
	 }

	 struct nvs_fs nvs_fs = { 0 };
	 nvs_fs.flash_device = flash_device;
	 nvs_fs.offset = flash_offset;
	 nvs_fs.sector_size = fp_info.size;
	 nvs_fs.sector_count = NVS_PARTITION_SIZE / fp_info.size;
	 ssize_t nvs_rc = nvs_mount(&nvs_fs);
	 if (nvs_rc) {
		 printk("Mounting NVS failed: %d.\n", nvs_rc);
		 return -1;
	 }

	 const char data[] = "some data";
	 nvs_rc = nvs_write(&nvs_fs, 0U, data, sizeof(data));
	 if (nvs_rc < 0) {
		 printk("NVS write error: %s (%d).\n", strerror(-nvs_rc), nvs_rc);
		 return -1;
	 }

	 // int foo = 0;
	 // nvs_rc = nvs_read(&nvs_fs, 0U, &foo, sizeof(foo)); // This would work
	 nvs_rc = nvs_read(&nvs_fs, 0U, NULL, 0); // This returns -22
	 if ((nvs_rc < 0) && (nvs_rc != -ENOENT)) {
		 printk("NVS read error: %s (%d).\n", strerror(-nvs_rc), nvs_rc);
		 return -1;
	 }
	 printk("NVS read returned: %s (%d).\n", strerror(-nvs_rc), nvs_rc);

	 printk("Sample completed with no issue.\n");
	 return 0;
 }
