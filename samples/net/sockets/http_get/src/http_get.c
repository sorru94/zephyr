/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#if !defined(__ZEPHYR__)

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#else

#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <zephyr/net/tls_credentials.h>
#include "ca_certificate.h"
#endif

#include "net_sample_common.h"

#endif

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(http_get, CONFIG_APP_LOG_LEVEL);

static struct net_mgmt_event_callback ipv4_cb;
static K_SEM_DEFINE(ipv4_address_obtained, 0, 1);

/* HTTP server to connect to */
#define HTTP_HOST "google.com"
/* Port to connect to, as string */
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define HTTP_PORT "443"
#else
#define HTTP_PORT "80"
#endif
/* HTTP path to request */
#define HTTP_PATH "/"

#define SSTRLEN(s) (sizeof(s) - 1)
#define FLUSH_LOGS { while (LOG_PROCESS()) {}}
#define CHECK(r) { if (r < 0) { LOG_ERR("Error: %d", (int)r); FLUSH_LOGS; exit(1); } }

#define REQUEST "GET " HTTP_PATH " HTTP/1.1\r\nHost: " HTTP_HOST "\r\n\r\n"

static char response[1024];

void dump_addrinfo(const struct addrinfo *ai)
{
	LOG_INF("addrinfo @%p: ai_family=%d, ai_socktype=%d, ai_protocol=%d, "
	       "sa_family=%d, sin_port=%x",
	       ai, ai->ai_family, ai->ai_socktype, ai->ai_protocol, ai->ai_addr->sa_family,
	       ntohs(((struct sockaddr_in *)ai->ai_addr)->sin_port));
}


static void ipv4_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event) {
        case NET_EVENT_IPV4_ADDR_ADD:
            k_sem_give(&ipv4_address_obtained);
            break;
        case NET_EVENT_IPV4_ADDR_DEL:
            k_sem_take(&ipv4_address_obtained, K_NO_WAIT);
            break;
    }
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st, sock;

	wait_for_network();

#ifdef CONFIG_NET_DHCPV4
    net_mgmt_init_event_callback(&ipv4_cb, ipv4_mgmt_event_handler,
        NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
    net_mgmt_add_event_callback(&ipv4_cb);

    struct net_if *iface = net_if_get_default();
    while (net_if_oper_state(iface) != NET_IF_OPER_UP) {
        k_sleep(K_MSEC(200));
    }

    net_dhcpv4_start(iface);
    while (k_sem_count_get(&ipv4_address_obtained) == 0) {
        k_sleep(K_MSEC(200));
    }
#endif

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
			   ca_certificate, sizeof(ca_certificate));
#endif

	LOG_INF("Preparing HTTP GET request for http://" HTTP_HOST
	       ":" HTTP_PORT HTTP_PATH "");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
	LOG_INF("getaddrinfo status: %d", st);

	if (st != 0) {
		LOG_ERR("Unable to resolve address, quitting");
		return 0;
	}

#if 0
	for (; res; res = res->ai_next) {
		dump_addrinfo(res);
	}
#endif

	dump_addrinfo(res);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sock = socket(res->ai_family, res->ai_socktype, IPPROTO_TLS_1_2);
#else
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#endif
	CHECK(sock);
	LOG_INF("sock = %d", sock);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_opt[] = {
		CA_CERTIFICATE_TAG,
	};
	CHECK(setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
			 sec_tag_opt, sizeof(sec_tag_opt)));

	CHECK(setsockopt(sock, SOL_TLS, TLS_HOSTNAME,
			 HTTP_HOST, sizeof(HTTP_HOST)))
#endif

	LOG_INF("Connecting to server...");
	CHECK(connect(sock, res->ai_addr, res->ai_addrlen));
	LOG_INF("Connected");
	LOG_INF("Sending request...");
	CHECK(send(sock, REQUEST, SSTRLEN(REQUEST), 0));

	LOG_INF("Response:");

	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			LOG_ERR("Error reading response");
			return 0;
		}

		if (len == 0) {
			break;
		}

		response[len] = 0;
		LOG_INF("%s", response);
	}

	LOG_INF("Close socket");

	(void)close(sock);
	return 0;
}
