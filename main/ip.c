#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "packet_types.h"
#include "buffer_pool.h"
#include "wifi.h"
#include "led.h"
#include "crc.h"
#include "uart.h"
#include "node_table.h"

static const char* TAG = "ip";

int udp_sock = -1;
buffer_pool_t* packet_pool;
TaskHandle_t udp_rx_task = NULL;
TaskHandle_t udp_tx_task = NULL;

QueueHandle_t udp_tx_queue = NULL;
QueueHandle_t udp_rx_queue = NULL;

node_table_t forward_for_nodes = { 0 };

void udp_error() {
	flash_led_once(UDP_RED_LED);
}

void init_udp(void) {
	struct sockaddr_in saddr = { 0 };
	int sock = -1;
	int err = 0;
	struct in_addr outgoing_addr = { 0 };
	esp_netif_ip_info_t ip_info = { 0 };
	
	/* bail out early if we don't have a wifi network interface yet */
	
	if (get_wifi_intf() == NULL) {
		ESP_LOGE(TAG, "wireless network interface does not exist.");
		udp_error();
		return;
	}
	
	/* or if doesn't think it's ready */
	if (!is_wifi_ready()) {
		ESP_LOGE(TAG, "wireless network interface is not ready.");
		udp_error();
		return;
	};
	
	/* or if we can't get its IP */
	err = esp_netif_get_ip_info(get_wifi_intf(), &ip_info);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "wireless network interface wouldn't tell us its IP.");
		udp_error();
		return;
	}
	
	inet_addr_from_ip4addr(&outgoing_addr, &ip_info.ip);
	
	/* create the multicast socket and twiddle its socket options */

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "socket() failed.  error: %d", errno);
		udp_error();
		return;
	}
	
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(1954);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (err < 0) {
		ESP_LOGE(TAG, "bind() failed, error: %d", errno);
		udp_error();
		goto err_cleanup;
	}
		
	uint8_t ttl = 1;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(uint8_t));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...IP_MULTICAST_TTL...) failed, error: %d", errno);
		udp_error();
		goto err_cleanup;
	}
		
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &outgoing_addr, sizeof(struct in_addr));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...IP_MULTICAST_IF...) failed, error: %d", errno);
		udp_error();
		goto err_cleanup;
	}


	struct ip_mreq imreq = { 0 };
	imreq.imr_interface.s_addr = IPADDR_ANY;
	err = inet_aton("239.192.76.84", &imreq.imr_multiaddr.s_addr);
	if (err != 1) {
		ESP_LOGE(TAG, "inet_aton failed, error: %d", errno);
		udp_error();
		goto err_cleanup;
	}

	err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
						 &imreq, sizeof(struct ip_mreq));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...IP_ADD_MEMBERSHIP...) failed, error: %d", errno);
		udp_error();
		goto err_cleanup;
	}
	 
	udp_sock = sock;
	
	ESP_LOGI(TAG, "multicast socket now ready.");
						
	return;

	
err_cleanup:
	close(sock);
	udp_error();
	return;

}

void udp_rx_runloop(void *pvParameters) {
	ESP_LOGI(TAG, "starting LToUDP listener");
	init_udp();
	
	while(1) {
		/* retry if we need to */
		while(udp_sock == -1) {
			ESP_LOGE(TAG, "setting up LToUDP listener failed.  Retrying...");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			init_udp();
		}
	
		while(1) {
			unsigned char buffer[603+4]; // 603 for LLAP (per Inside Appletalk) + 
										 // 4 bytes LToUDP header
			int len = recv(udp_sock, buffer, sizeof(buffer), 0);
			if (len < 0) {
				flash_led_once(UDP_RED_LED);
				break;
			}
			if (len > 603) {
				flash_led_once(UDP_RED_LED);
				ESP_LOGE(TAG, "packet too long");
				continue;
			}
			if (len > 7) {
				flash_led_once(UDP_RX_GREEN);
			
				// Have we seen any traffic from this node?  Is it one
				// we actually want to send traffic to?  If not, go around
				// again.
				if (!nt_fresh(&forward_for_nodes, buffer[4], 3600) &&
				    (buffer[4] != 255)) {
					continue;
				}
			
				// fetch an empty buffer from the pool and fill it with
				// packet info
				llap_packet* lp = bp_fetch(packet_pool);
				if (lp != NULL) {
					crc_state_t crc;
				
					// copy the LLAP packet into the packet buffer
					lp->length = len-2; // -4 for LToUDP tag, +2 for the FCS
					memcpy(lp->packet, buffer+4, len-4);
				
					// now calculate the CRC
					crc_state_init(&crc);
					crc_state_append_all(&crc, buffer+4, len-4);
					lp->packet[lp->length - 2] = crc_state_byte_1(&crc);
					lp->packet[lp->length - 1] = crc_state_byte_2(&crc);
				
					xQueueSendToBack(udp_rx_queue, &lp, portMAX_DELAY);
				}
			}			
		}
	
		close(udp_sock);
		udp_sock = -1;
	}
}

void udp_tx_runloop(void *pvParameters) {
	llap_packet* packet = NULL;
	unsigned char outgoing_buffer[605 + 4] = { 0 }; // LToUDP has 4 byte header
	struct sockaddr_in dest_addr = {0};
	int err = 0;
	
	static int consecutive_handshakes = 0;
	
	dest_addr.sin_addr.s_addr = inet_addr("239.192.76.84");
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(1954);
	
	ESP_LOGI(TAG, "starting LToUDP sender");
	
	while(1) {
		xQueueReceive(udp_tx_queue, &packet, portMAX_DELAY);
				
		if (packet == NULL) {
			continue;
		}
		
		// Add this to the "known senders" list we're forwarding traffic for
		if (packet->length > 3) {
			nt_touch(&forward_for_nodes, packet->packet[1]);
		}
		
		// Should we pass this on?
		// If it's an RTS or a CTS packet, then no
		if (packet->length == 5) {
			if (packet->packet[2] == 0x84 || packet->packet[2] == 0x85) {
				consecutive_handshakes++;
				
				if (consecutive_handshakes > 10) {
					flash_led_once(LT_RED_LED);
					ESP_LOGE(TAG, "%d consecutive handshakes! reinitialisng tashtalk", consecutive_handshakes);
					uart_init_tashtalk();
					consecutive_handshakes = 0;
					
				}
				goto skip_processing;
			}
		}
		
		consecutive_handshakes = 0;
		
		/* TODO: if this is an ENQ for a node ID that is not in our recently
		   seen table, pass it on, otherwise block all ENQs */
		   
		if (udp_sock != -1) {
			memcpy(outgoing_buffer+4, packet->packet, packet->length);
			
			err = sendto(udp_sock, outgoing_buffer, packet->length+2, 0, 
				(struct sockaddr *)&dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "error: sendto: errno %d", errno);
				flash_led_once(UDP_RED_LED);
			} else {
				flash_led_once(UDP_TX_GREEN);
			}
			err = 0;
		}
				
skip_processing:
		bp_relinquish(packet_pool, (void**)&packet);
	}
}


void start_udp(buffer_pool_t* pool, QueueHandle_t txQueue, QueueHandle_t rxQueue) {
	packet_pool = pool;
	udp_tx_queue = txQueue;
	udp_rx_queue = rxQueue;
	xTaskCreate(&udp_rx_runloop, "LToUDP-rx", 4096, NULL, 5, &udp_rx_task);
	xTaskCreate(&udp_tx_runloop, "LToUDP-tx", 4096, NULL, 5, &udp_tx_task);

}