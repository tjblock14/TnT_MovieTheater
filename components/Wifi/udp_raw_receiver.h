#ifndef UDP_RAW_RECEIVER_H
#define UDP_RAW_RECEIVER_H

#include "CoreDefines.h"

/* One RGB triplet per LED, sent by the Raspberry Pi. */
#define TV_BACKLIGHT_UDP_RX_SIZE (NUM_BACKLIGHT_LEDS * 3)

void udp_listen_task(void *pvParameters);

#endif // UDP_RAW_RECEIVER_H
