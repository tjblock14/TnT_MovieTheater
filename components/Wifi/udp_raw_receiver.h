#ifndef UDP_RAW_RECEIVER_H
#define UDP_RAW_RECEIVER_H

/* We expect three bytes for each LED from the raspberry pi, a reb, green, and blue byte */
#define TV_BACKLIGHT_UDP_RX_SIZE      (NUM_BACKLIGHT_LEDS * 3)

extern void udp_listen_task(void *pvParameters);

#endif // UDP_RAW_RECEIVER_H