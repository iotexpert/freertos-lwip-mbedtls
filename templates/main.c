#include <cyhal.h>
#include <cybsp.h>
#include <lwip/tcpip.h>
#include <lwip/api.h>
#include <cy_retarget_io.h>
#include <cybsp_wifi.h>
#include <initlwip.h>
#include <mbedtlsinit.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

#define WIFI_SSID   "CYFI_IOT_EXT"
#define WIFI_KEY    "cypresswicedwifi101"

static const char *request = "GET /get HTTP/1.1\r\nHost: httpbin.org\r\nConnection: close\r\n\r\n" ;
//static const char *request = "GET / HTTP/1.1\r\n\r\n" ;


/* The primary WIFI driver interface */
static whd_interface_t iface ;

/* This enables RTOS aware debugging */
volatile int uxTopUsedPriority ;

static void donothing(void *arg)
{
}

/*
 * This is the familiar blinky.  It is useful because when the LED stops blinking we know there has
 * been a hard crash of some type.
 */
static void blinky(void *args)
{
    /* Convert 500 ms to ticks */
    TickType_t ticks = pdMS_TO_TICKS(500) ;


    /* Initialize the User LED */
    cyhal_gpio_init((cyhal_gpio_t) CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    while (true)
    {
        cyhal_gpio_toggle((cyhal_gpio_t) CYBSP_USER_LED) ;
        vTaskDelay(ticks) ;
    }
}




static void networkTask(void *arg)
{
    cy_rslt_t res ;
    whd_ssid_t ssiddata ;
    int received = 0 ;
    int packets = 0 ;

    const char *ssid = WIFI_SSID ;
    const char *key = WIFI_KEY ;

    tcpip_init(donothing, NULL) ;
    printf("LWiP TCP/IP stack initialized\n") ;

    cybsp_wifi_init_primary(&iface) ;
    printf("WIFI driver initialized \n") ;

    initMbedTls() ;
    printf("MBEDTLS initialized\n") ;

    ssiddata.length = strlen(ssid) ;
    memcpy(ssiddata.value, ssid, ssiddata.length) ;
    res = whd_wifi_join(iface, &ssiddata, WHD_SECURITY_WPA2_AES_PSK, (const uint8_t *)key, strlen(key)) ;
 
    printf("Sucessfully joined wifi network '%s'\n", ssid) ;

    addInterfaceToLwip(iface, NULL) ;
   
    printf("WiFi interface added to TCP/IP stack\nRunning dns lookup\n") ;
    
    struct netif *net = getLWIPInterface() ;    

    while (true)
    {
        if (net->ip_addr.u_addr.ip4.addr != 0)
        {
            printf("IP Address %s assigned\n", ip4addr_ntoa(&net->ip_addr.u_addr.ip4)) ;
            break ;
        }
        vTaskDelay(100) ;
    }

    const char *site = (const char *)arg ;
    struct netconn *conn ;
    struct ip_addr remote ;

    struct netbuf *recv ;
    int len = strlen(request) ;
  
    char *ptr ;
    uint16_t plen ;

    printf("DNS Loopup %s\n",site);
    while (netconn_gethostbyname(site, &remote) != ERR_OK);

    printf("Connecting to host '%s', address %s\n", site, ipaddr_ntoa(&remote)) ;
    
    conn = netconn_new(NETCONN_TCP) ;
    netconn_connect(conn, &remote, 80) ;
    netconn_write(conn, request, len, NETCONN_NOFLAG) ;
    
    while (true)
    {
        res = netconn_recv(conn, &recv) ;
        if (res != ERR_OK)
            break ;
        netbuf_data(recv, (void *)&ptr, &plen) ;
        received += plen ;
        packets++ ;

        for(int i=0;i<plen;i++)
            printf("%c",ptr[i]);

    }
    netbuf_free(recv) ;
    netbuf_delete(recv) ;
            
    netconn_delete(conn) ;

    printf("\n\n\nReceived %d packets\n", packets) ;        
    printf("Received %d bytes total\n", received) ;
 }

int main()
{
    cy_rslt_t result ;

    /* This enables RTOS aware debugging in OpenOCD */
    uxTopUsedPriority = configMAX_PRIORITIES - 1 ;
   
    /* Initialize the board support package */
    result = cybsp_init() ;
    CY_ASSERT(result == CY_RSLT_SUCCESS) ;

    /* Enable global interrupts */
    __enable_irq();
    
    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* clear the screen */
    printf("\x1b[2J\x1b[;H");
    printf("FreeRTOS LWiP MBEDTLS demo project\n") ;

    //return 0;

    /* Create our original tasks */    
    xTaskCreate(blinky, "BLINKY", 128, NULL, 1, NULL) ;
    xTaskCreate(networkTask, "Network", 256, "httpbin.org", 1, NULL) ;
    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler() ;   

    /* Should never get here */
    CY_ASSERT(0) ;
}