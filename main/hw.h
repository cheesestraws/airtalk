#define REVA

#ifdef REV0
// REV0 => through-hole prototype
#define UART_TX GPIO_NUM_14
#define UART_RX GPIO_NUM_27
#define UART_RTS GPIO_NUM_26
#define UART_CTS GPIO_NUM_12

#define WIFI_RED_LED_PIN 23
#define WIFI_GREEN_LED_PIN 22
#define UDP_RED_LED_PIN 19
#define UDP_TX_GREEN_PIN 18
#define UDP_RX_GREEN_PIN 5
#define LT_RED_LED_PIN 17
#define LT_TX_GREEN_PIN 16
#define LT_RX_GREEN_PIN 4
#define OH_NO_LED_PIN 0
#endif

#ifdef REVA
// REVA => surface-mount prototype
#define UART_TX GPIO_NUM_15
#define UART_RX GPIO_NUM_13
#define UART_RTS GPIO_NUM_5
#define UART_CTS GPIO_NUM_2

#define WIFI_RED_LED_PIN 17
#define WIFI_GREEN_LED_PIN 32
#define UDP_RED_LED_PIN 27
#define UDP_TX_GREEN_PIN 12
#define UDP_RX_GREEN_PIN 14
#define LT_RED_LED_PIN 33
#define LT_TX_GREEN_PIN 25
#define LT_RX_GREEN_PIN 26
#define OH_NO_LED_PIN 18
#endif