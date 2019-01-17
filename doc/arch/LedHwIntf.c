#include "espressif/esp_common.h"
#include <selib.h>
#include <ledctrl.h>

#define D0 16 // Bultin LED on Geekcreit ESP8266
#define D6 12 // Wire LED between D6 and ground
#define D7 13 // Wire LED between D7 and ground
#define D8 15 // Wire LED between D8 and ground

static const LedInfo ledInfo[] = {
   {
      "Bultin LED",
      LedColor_blue,
      1
   },
   {
      "LED D6",
      LedColor_red,
      2
   },
   {
      "LED D7",
      LedColor_red,
      3
   },
   {
      "LED D8",
      LedColor_red,
      4
   }
};

#define LEDSLEN (sizeof(ledInfo) / sizeof(ledInfo[0]))

static int leds[LEDSLEN];

/* Returns the LED on/off state for led with ID 'ledId'.
 */
int getLedState(int ledId)
{
   if(ledId >= 1 && ledId <= LEDSLEN)
      return leds[ledId-1];
   return 0;
}

/*
  Return an array of LedInfo (struct). Each element in the array
  provides information for one LED. The 'len' argument must be set by
  function getLedInfo.  The out argument 'en' specifies the length of
  the returned array, that is, number of LEDs in the device.  Each LED
  has a name, color, and ID. The ID, which provides information about
  which LED to turn on/off, is used by control messages sent between
  device code and UI clients. The IDs for a four LED device can for
  example be 1,2,3,4.
*/
const LedInfo* getLedInfo(int* len)
{
   *len = LEDSLEN;
   return ledInfo;
}


/* Command sent by UI client to turn LED with ID on or off. This
   function must set the LED to on if 'on' is TRUE and off if 'on' is FALSE.
   The parameter 'LedId' is the value of the field 'id' in the
   corresponding 'LedInfo' structure.  Code might be required to match
   the 'id' value to the physical LED and entry in the LED array.  In
   this example, 'id' field is numbered from 1 through 4, with 'id' 1
   referring to the first entry in the array of LedInfo
   structures. The expression (LedId - 1), results in the correct
   index.
*/
int _setLed(int ledId, int on)
{
   if(ledId >= 1 && ledId <= LEDSLEN)
   {
      leds[ledId-1] = on;
      switch(ledId)
      {
         case 1: gpio_write(D0, on ? 0 : 1); break;
         case 2: gpio_write(D6, on ? 1 : 0); break;
         case 3: gpio_write(D7, on ? 1 : 0); break;
         case 4: gpio_write(D8, on ? 1 : 0); break;
      }
      return 0;
   }
   return -1;
}

int setLed(int ledId, int on)
{
   printf("Set led %d %s\n", ledId, on ? "on" : "off");
   return _setLed(ledId, on);
}


/*
  An optional function that enables LEDs to be set directly by the
  device. This function is typically used by devices that include one
  or more buttons. A button click may for example turn on a specific
  LED. The function is called at intervals (polled) by the LED device
  code. The function may for example detect a button click and return
  the information to the caller. Arguments 'ledId' and 'on' are out
  arguments, where 'ledId' is set to the LED ID and 'on' is set to
  TRUE for on and FALSE for off. The function must return TRUE (a non
  zero value) if the LED is to be set on/off and zero on no
  change. Create an empty function returning zero if you do not plan
  on implementing this feature.
*/
int setLedFromDevice(int* ledId, int* on)
{
   return FALSE;
}


void initLED(void)
{
    gpio_enable(D0, GPIO_OUTPUT);
    gpio_enable(D6, GPIO_OUTPUT);
    gpio_enable(D7, GPIO_OUTPUT);
    gpio_enable(D8, GPIO_OUTPUT);
}
