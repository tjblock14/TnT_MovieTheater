#include "udp_raw_receiver.h"
#include "CoreDefines.h"
#include "CoreVariables.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <errno.h>
#include <sys/time.h>

#ifdef DEBUG_CFG
    static const char *TAG = "UDP";
#endif

#ifndef UDP_RX_STACK_SIZE
    #define UDP_RX_STACK_SIZE    (4096)
#endif

#define UDP_LISTEN_PORT          (7777)
#define UDP_BUF_SIZE             (256)

static void udp_listen_task(void *pvParameters)
{
    uint8_t udp_buff[UDP_BUF_SIZE];

    /* First, create the socket */
    int UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    /* Set a timeout for how long we wait to receive a packet */
    struct timeval timeout = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(UdpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


    /* If the socket was not successfully created, delete the task */
    if(UdpSocket < 0)
    {
        #ifdef DEBUG_CFG
            ESP_LOGE(TAG, "Unable to create the socket");
        #endif

        vTaskDelete(NULL);
        return;
    }

    /* Now, bind it? */

    /* Create an empty sock address struct */
    struct sockaddr_in st_UdpListen = {0};

    st_UdpListen.sin_family      = AF_INET;
    st_UdpListen.sin_port        = htons(UDP_LISTEN_PORT);
    st_UdpListen.sin_addr.s_addr = htonl(INADDR_ANY);

    int err = bind(UdpSocket, (struct sockaddr *)&st_UdpListen, sizeof(st_UdpListen));

    if(err < 0)
    {
        #ifdef DEBUG_CFG
            ESP_LOGE(TAG, "Socket did not bind correctly - Err: %d", err);
        #endif

        close(UdpSocket);
        vTaskDelete(NULL);

        return;
    }

    #ifdef DEBUG_CFG
        ESP_LOGI(TAG, "UDP Server listening on port %d", UDP_LISTEN_PORT);
    #endif

    /* UDP task infinite loop */
    while(1)
    {
        struct sockaddr_in SourceAddr = {0};
        socklen_t SocketLength = sizeof(SourceAddr);

        /* Get how many bytes were received into the UDP buffer (size of UDP packet payload) */
        int UdpBytesReceived = recvfrom(UdpSocket, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&SourceAddr, &SocketLength);

        /* There was an error. Error gets stored in errno */
        if(UdpBytesReceived < 0)
        {   
            /* errno is a LwIP global error integer that stores the error from LwIP functions */
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue; /* Timed out before any data was available */
            }

            #ifdef DEBUG_CFG 
                ESP_LOGE(TAG, "recfrom() failed, error: %d", errno);
            #endif

            continue;
        }

        #ifdef DEBUG_CFG
             /* Convert the Sender's IP from binary to string format and log it */
            char SenderIp[32];
            inet_ntoa_r(SourceAddr.sin_addr, SenderIp, sizeof(SenderIp));
            ESP_LOGI(TAG, "RX %d bytes from %s:%d", UdpBytesReceived, SenderIp, ntohs(SourceAddr.sin_port));
        #endif

        /*
         * Payload format (FIRST TEST):
         *   4 bytes: R, G, B, W
         * This sets ALL LEDs to one color. Once this works, you can upgrade
         * to full frames (LED_COUNT * 4 bytes).
         */
        if (UdpBytesReceived == 4) 
        {
            uint8_t red = udp_buff[0];
            uint8_t green = udp_buff[1];
            uint8_t blue = udp_buff[2];
            uint8_t white = udp_buff[3];

            /* FIXME: Set the actual function to set all LEDs*/
            tv_backlight_set_all(red, green, blue, white);
        } 
        else 
        {
            #ifdef DEBUG_CFG
                ESP_LOGW(TAG, "Unexpected payload length %d (expected 4)", UdpBytesReceived);
            #endif
        }
    }
}