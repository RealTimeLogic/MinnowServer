# FreeRTOS/lwIP ESP32 and ESP8266 porting code

This directory contains porting code for ESP8266. The code should be very similar for ESP32. 
You may use this code "as is" with [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) or with the [SharkSSL ESP8266 IDE](https://realtimelogic.com/downloads/sharkssl/ESP8266/). The server runs in non secure mode when used with the esp-open-rtos package and in secure (TLS) mode when used with the SharkSSL ESP8266 IDE.

### Note:

The much more sophisticated Barracuda App Server, which includes WebSockets, is also available for ESP32 (WROVER). See the [Barracuda App Server for ESP32](https://realtimelogic.com/downloads/bas/ESP32/) download page for details.


### Files:

* EspMain.c - FreeRTOS/lwIP esp-open-rtos startup code and initialization, including some APIs used by the example code
* LedHwIntf.c - LED interface code
* MinnowHwIntf.c - Temperature, credentials, and upload  firmware interface code
* Makefile - SharkSSL ESP8266 IDE makefile

The following instructions are for building the Minnow Server example using the [SharkSSL ESP8266 IDE](https://realtimelogic.com/downloads/sharkssl/ESP8266/).

1. When the VM is running, navigate to: http://IP-ADDRESS-OF-VM/
2. Configure (set) your wi-fi credentials as instructed by the web page.
3. Navigate to the web based Linux shell: http://IP-ADDRESS-OF-VM/webshell/
4. Login with the credentials sharkssl/SharkSSL
5. Run the following Linux commands in the WebShell (the sudo password is 'SharkSSL'):

```bash
# Navigate to examples directory
cd ESP/esp-open-rtos/examples/
#Put the MinnowServer, the example, and JSON lib in the ms directory.
mkdir ms
cd ms
# Note that we do not need JSON and SMQ since these two componets are included in the SharkSSL IDE.
git clone https://github.com/RealTimeLogic/MinnowServer.git
# Navigate to the architecture directory
cd MinnowServer/doc/arch/
# Copy the two required files. All other files are included in the ESP8266 SharkSSL delivery
cp Makefile MinnowHwIntf.c ../../../
# The following enables compiling from withing the web IDE
chmod +x ms.sh; mv ms.sh ~/ESP/
# Navigate to the 'host build' make directory
cd ../../example/make/
# Amalgamate and compress www -> creates index.c
make packwww
# cd to the root of the example
cd ~/ESP/esp-open-rtos/examples/ms
# Build example and upload to the connected ESP8266
make test FLASH_MODE=dio ESPPORT=/dev/ttyUSB0
```

If the build and upload process are successful, you should see the ESP8266 connecting to your WiFi and then you should see the text:

WebSocket server listening on 443

You may then use a browser and navigate to https://IP-ADDRESS-OF-VM. Note that you will get a certificate error, which you may bypass. Remove the certificate error as follows:

1. Add the following entry in your computer's hosts file: IP-ADDRESS-OF-VM	device
2. Install [RTL's root certificate](https://realtimelogic.com/downloads/root-certificate/).
3. Navigate to https://device

See the embedded.com article [How to install a secure embedded Web server on ESP8266](https://www.embedded.com/design/prototyping-and-development/4461577/How-to-install-a-secure-embedded-web-server-on-a--3-WiFi-device) for detailed instructions.
