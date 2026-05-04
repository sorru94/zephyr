/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

int main(void)
{
	printk("Address of sample %p\n", (void *)__rom_region_start);
	printk("Hello sysbuild with mcuboot! %s\n", CONFIG_BOARD);
	printk("Waiting for PHY auto-negotiation to trigger in the background...\n");

	/* Sleep to keep the main thread alive while the network stack logs the bug */
	k_sleep(K_SECONDS(10));

	return 0;
}
