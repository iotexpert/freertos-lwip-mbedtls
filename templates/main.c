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

/* The primary WIFI driver interface */
static whd_interface_t iface ;

/* This enables RTOS aware debugging */
volatile int uxTopUsedPriority ;

static void donothing(void *arg)
{
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

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler() ;   

    /* Should never get here */
    CY_ASSERT(0) ;
}

static const char *request = "GET / HTTP/1.1\r\nHost: www.iotlibs.com\r\nConnection: close\r\n\r\n" ;
static int received = 0 ;
static int packets = 0 ;

static void getHTTPData(void *arg)
{
    TickType_t ticks = pdMS_TO_TICKS(1000) ;  
    const char *site = (const char *)arg ;
    struct netconn *conn ;
    struct ip_addr remote ;
    bool first = true ;
    struct netbuf *recv ;
    int len = strlen(request) ;
    err_t res ;
    void *ptr ;
    uint16_t plen ;

    /* Get a host IP address, given a name */     
    while (netconn_gethostbyname(site, &remote) != ERR_OK)
    {
        // Loop til it works
    }
        
    printf("Connecting to host '%s', address %s\n", site, ipaddr_ntoa(&remote)) ;
    printf("--------------------------------------------------------------------\n") ;
    while (true)
    {
        if (!first)
          vTaskDelay(ticks) ;

        /* Create a new connection, this does not actually connect yet, just allocates a connection */          
        conn = netconn_new(NETCONN_TCP) ;
        if (conn == NULL)
            continue ;
        
        /* Actually connet to the remote host */
        res = netconn_connect(conn, &remote, 80) ;
        if (res != ERR_OK)
        {
            printf("Error connecting data, code = %d\n", res) ;          
            netconn_delete(conn) ;
            continue ;
        }

        /* Write a HTTP GET request to the remote host */
        res = netconn_write(conn, request, len, NETCONN_NOFLAG) ;
        if (res != ERR_OK)
        {
            printf("Error sending data, code = %d\n", res) ;
            netconn_delete(conn) ;
            continue ;
        }

        /* Now read data until the complete response has been read */
        while (true)
        {
            /* Rceive data, break out of this loop if an error occurs */
            res = netconn_recv(conn, &recv) ;
            if (res != ERR_OK)
                break ;

            /* Extract the data an size from the received packet */        
            netbuf_data(recv, &ptr, &plen) ;
            received += plen ;
            packets++ ;
                    
            /* 
             * Print information on the screen about the number of packets and
             * amount of data received.
             */
            if (!first)
                printf("\x1b[4A") ;
            
            printf("\x1b[K") ;
            printf("Received %d packets\n", packets) ;
            
            printf("\x1b[K") ;
            printf("Received %d bytes total\n", received) ;
            
            printf("\x1b[K") ;
            printf("Total %d bytes free\n", xPortGetFreeHeapSize()) ;
            
            printf("\x1b[K") ;
            printf("Minimum %d bytes free\n", xPortGetMinimumEverFreeHeapSize()) ;
            
            netbuf_free(recv) ;
            netbuf_delete(recv) ;
            
            first = false ;
        }

        first = false ;        

        /* Delete the connection, which is disconnect from the other end */
        netconn_delete(conn) ;
    }
}

/*
 * This function waits until the DHCP assigns an IP address to the network interface.
 * Thiis a FreeRTOS task that the address is assigned, starts all of the other tasks 
 * that want to run based on a network connection.
 */
static void waitForDHCP(void *args)
{
    /* Convert 1 second to ticks */
    TickType_t ticks = pdMS_TO_TICKS(1000) ;
    struct netif *net = getLWIPInterface() ;    

    while (true)
    {
        if (net->ip_addr.u_addr.ip4.addr != 0)
        {
            printf("IP Address %s assigned\n", ip4addr_ntoa(&net->ip_addr.u_addr.ip4)) ;
            break ;
        }
        vTaskDelay(ticks) ;
    }

    xTaskCreate(getHTTPData, "HTTP", 1024, "www.iotlibs.com", 1, NULL) ;
    vTaskDelete(NULL) ;
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

/*
 * THis is called first after the initialization of FreeRTOS.  It basically connects to WiFi and then starts the
 * task that waits for DHCP.  No tasks will actually run until this function returns.
 */
void vApplicationDaemonTaskStartupHook()
{
    cy_rslt_t res ;
    whd_ssid_t ssiddata ;
    const char *ssid = WIFI_SSID ;
    const char *key = WIFI_KEY ;

    //
    // Step 1: bring up the LWiP TCP stack
    //
    // Note: the LWiP stack can create a thread that is an application thread.  I don't really
    //       want to use it that way.  I really want to bring up WiFi and get an IP address before
    //       launching additional tasks.  The donothing is basically an empty function that 
    //       is a NOP in the network bringup.
    //
    tcpip_init(donothing, NULL) ;
    printf("LWiP TCP/IP stack initialized sucessfully\n") ;

    //
    // Step 2: Initialize the WIFI driver
    //
    res = cybsp_wifi_init_primary(&iface) ;
    if (res != CY_RSLT_SUCCESS)
    {
        printf("wifi_sdio_init failed -  could not initialize WHD driver, error %lx\n", res) ;
        return ;
    }
    printf("WIFI driver initialized sucessfully\n") ;

    //
    // Step 3: Initialize MBEDTLS 
    //
    res = initMbedTls() ;
    if (res != CY_RSLT_SUCCESS)
    {
        printf("    initialization of mbedtls failed, error %lx\n", res) ;
        return ;
    }
    printf("MBEDTLS initialized sucessfully\n") ;

    //
    // Step 4: Join a wifi network
    //
    ssiddata.length = strlen(ssid) ;
    memcpy(ssiddata.value, ssid, ssiddata.length) ;
    res = whd_wifi_join(iface, &ssiddata, WHD_SECURITY_WPA2_AES_PSK, (const uint8_t *)key, strlen(key)) ;
    if (res != CY_RSLT_SUCCESS)
    {
        res = whd_wifi_join(iface, &ssiddata, WHD_SECURITY_WPA2_TKIP_PSK, (const uint8_t *)key, strlen(key)) ;
        if (res != CY_RSLT_SUCCESS)
        {
            res = whd_wifi_join(iface, &ssiddata, WHD_SECURITY_WPA2_MIXED_PSK, (const uint8_t *)key, strlen(key)) ;
            if (res != CY_RSLT_SUCCESS)
            {
                xTaskCreate(blinky, "BLINKY", 128, NULL, 1, NULL) ;                
                printf("    could not join WIFI network '%s'\n", ssid) ;
                return ;
            }
        }
    }
    printf("Sucessfully joined wifi network '%s'\n", ssid) ;

    //
    // Step 5: Add the new WIFI interface to the TCP/IP stack
    //
    res = addInterfaceToLwip(iface, NULL) ;
    if (res != CY_RSLT_SUCCESS)
    {
        printf("    network stack bring up failed, error %lx\n", res) ;
        return ;
    }

    printf("WiFi interface added to TCP/IP stack\n") ;
    printf("Network stack is ready\n") ;

    /* Create our original tasks */    
    xTaskCreate(blinky, "BLINKY", 128, NULL, 1, NULL) ;
    xTaskCreate(waitForDHCP, "DHCP", 256, NULL, 1, NULL) ;
}
