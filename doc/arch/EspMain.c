
#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/api.h"

#include "ssid_config.h"

#include <selib.h>
#include <ledctrl.h>

extern void initLED(void);
extern int _setLed(int ledId, int on);

static int errorCounter;

static void restart()
{
    int i;
    printf("Restarting....\n");
    for(i = 0 ; i < 10; i++)
    {
        _setLed(1, TRUE);
        vTaskDelay(150 / portTICK_PERIOD_MS);
        _setLed(1, FALSE);
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
    sdk_system_restart();
}

int pollkb(void)
{
   int c=uart_getc_nowait(0);
   if(c < 0) return 0;
   //printf("---->>> %d\n", c);
   return c=='\r' ? '\n' : c;
}

U32 getMilliSec(void)
{
   return xTaskGetTickCount() * portTICK_PERIOD_MS;
}



static U32 baseUnixTime = 1517846288;

U32 baGetUnixTime(void)
{
   return baseUnixTime +
      (U32)(((U64)xTaskGetTickCount() * (U64)portTICK_PERIOD_MS) / 1000);
}



static void setTime(void)
{
   int chk=0;
   ip_addr_t addr;
   struct netconn* ncon = netconn_new(NETCONN_TCP);
   if(ncon)
   {
      chk++;
      if(!netconn_gethostbyname("time.nist.gov", &addr))
      {
         chk++;
         if(!netconn_connect(ncon, &addr, 37))
         {
            struct netbuf *buf;
            chk++;
            if(!netconn_recv(ncon, &buf))
            {
               U32 now;
               chk++;
               if(4 == netbuf_copy(buf, (void*)&now, 4))
               {
                  chk++;
                  now = lwip_htonl(now);
                  /* Convert from 1900 to 1970 format */
                  now = now - 2208988800U;
                  printf("The current Unix epoch time is %u\n",now);
                  baseUnixTime =
                     now - (xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
               }
               netbuf_delete(buf);
            }
         }
      }
      netconn_delete(ncon);
   }
   if( chk != 5 )
      printf("Connecting to time.nist.gov failed! %d\n",chk);
}


void _xprintf(const char* fmt, ...)
{
   va_list varg;
   va_start(varg, fmt);
   vprintf(fmt, varg);
   va_end(varg);
} 


/*
  Returns the name of this device. Used by some examples
*/
const char* getDevName(void)
{
   return "ESP8266 FreeRTOS/lwIP";
}

/* Return MAC addr. Used by some examples
 */
int getUniqueId(const char** id)
{
   struct netif *netif = sdk_system_get_netif(STATION_IF);
   if(!netif)
   {
       printf("Cannot get MAC addr!\n");
       restart();
   }
   *id = (char*)netif->hwaddr;
   return (int)netif->hwaddr_len;
}


/* Used by some examples */
void setProgramStatus(ProgramStatus s)
{
    switch(s)
   {
      case ProgramStatus_DnsError:
      case ProgramStatus_ConnectionError:
         if(++errorCounter < 10)
         {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            break;
         }
         /* fall through */
      case ProgramStatus_SocketError:
      case ProgramStatus_PongResponseError:
      case ProgramStatus_InvalidCommandError:
      case ProgramStatus_MemoryError:
      case ProgramStatus_Restarting:
          restart();
          break;
              
      case ProgramStatus_CloseCommandReceived:
          for(;;)
          {
              vTaskDelay(1000 / portTICK_PERIOD_MS);
              printf("CloseCommandReceived\n");
          }

      case ProgramStatus_Starting:
      case ProgramStatus_CertificateNotTrustedError:
      case ProgramStatus_SslHandshakeError:
      case ProgramStatus_WebServiceNotAvailError:
          break;

      case ProgramStatus_Connecting:
          _setLed(2, TRUE);
          break;

   case ProgramStatus_SslHandshake:
          _setLed(3, TRUE);
          break;

   case ProgramStatus_DeviceReady:
          _setLed(1, FALSE);
          _setLed(2, FALSE);
          _setLed(3, FALSE);
          break;
   }
}



static void setupWiFi(void)
{
   errorCounter=0;
   vTaskDelay(1500 / portTICK_PERIOD_MS);
    printf("%sSDK version:%s\n", 
           "\n\nSharkSSL Example Task starting.\n",
           sdk_system_get_sdk_version());
    for(;;)
    {
        uint8_t status = sdk_wifi_station_get_connect_status();
        printf("WiFi connection status %u\n",(unsigned int)status);
        if(status == STATION_GOT_IP)
            break;
        if (status == STATION_WRONG_PASSWORD)
            printf("WiFi: wrong password\n\r");
        else if (status == STATION_NO_AP_FOUND)
            printf("WiFi: AP not found\n\r");
        else if (status == STATION_CONNECT_FAIL)
            printf("WiFi: connection failed\r\n");
        else
        {
            if(++errorCounter < 25)
            {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                continue;
            }
            printf("Giving up\n");
        }
        restart();
    }
}


static void mainTaskWrapper(void *pvParameters)
{
   initLED();
   setupWiFi();
   setTime();
   mainTask(0); /* shark example */
   restart();
}


void user_init(void)
{
    uart_set_baud(0, 115200);

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* Must call_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(&mainTaskWrapper, "SharkSSL Example", 600, NULL, 2, NULL);
}
