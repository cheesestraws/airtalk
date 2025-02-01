set(srcs
	 "buffer_pool.c"
	 "led.c"
	 "net_common.c"
	 "wifi.c"
	 "ethernet.c"
	 "ip.c"
	 "uart.c"
	 "tashtalk.c"
	 "crc.c"
	 "crc32.c"
	 "node_table.c"
	 "apps.c"
	 "scan_manager.c"
	 "packet_utils.c"
	 "pstrings.c"
	 "storage.c"
	 "recovery_listener.c"
	 "recovery.c"
	 "main.c"
)
        
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "")