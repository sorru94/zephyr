/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>

/* Semaphore to block until we get an IP address */
static struct k_sem ipv4_sem;
static struct net_mgmt_event_callback mgmt_cb;

/* Note: mgmt_event is now a uint64_t to match Zephyr 4.x requirements */
static void event_handler(struct net_mgmt_event_callback *cb,
			  uint64_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND) {
		for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			/* Address variables are now wrapped inside the embedded 'ipv4' struct */
			if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type == NET_ADDR_DHCP) {
				char buf[NET_IPV4_ADDR_LEN];

				printk("Acquired IPv4 address: %s\n",
				       net_addr_ntop(AF_INET,
						     &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
						     buf, sizeof(buf)));

				/* Unblock the main thread */
				k_sem_give(&ipv4_sem);
				break;
			}
		}
	}
}

int main(void)
{
	printk("Address of sample %p\n", (void *)__rom_region_start);
	printk("Hello sysbuild with mcuboot! %s\n", CONFIG_BOARD);

	/* Fetch the first Ethernet interface directly */
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (!iface) {
		printk("No Ethernet interface found. Ensure networking is configured properly.\n");
		return -1;
	}

	printk("Ethernet interface found.\n");

	/* Initialize semaphore to wait for the DHCP assignment */
	k_sem_init(&ipv4_sem, 0, 1);

	/* Register the event callback for DHCP bound */
	net_mgmt_init_event_callback(&mgmt_cb, event_handler, NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&mgmt_cb);

	/* Start the DHCPv4 client */
	printk("Starting DHCPv4...\n");
	net_dhcpv4_start(iface);

	printk("Waiting for IP address...\n");

	/* Wait up to 20 seconds for the IP address to be bound */
	if (k_sem_take(&ipv4_sem, K_SECONDS(20)) == 0) {
		printk("Successfully acquired IP address and ready for traffic!\n");
	} else {
		printk("Timeout waiting for IP address. Check your network/router.\n");
	}

	return 0;
}
