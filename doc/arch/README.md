# FreeRTOS/lwIP ESP32 and ESP8266 porting code

This directory contains porting code for ESP8266. The code should be very similar for ESP32. 
You may use this code "as is" with [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) or with the [SharkSSL ESP8266 IDE](https://realtimelogic.com/downloads/sharkssl/ESP8266/). The server runs in non secure mode when used with the esp-open-rtos package and in secure (TLS) mode when used with the SharkSSL ESP8266 IDE.

### Files:

* EspMain.c - FreeRTOS/lwIP esp-open-rtos startup code and initialization, including some APIs used by the example code
* LedHwIntf.c - LED interface code
* MinnowHwIntf.c - Temperature, credentials, and upload  firmware interface code
* Makefile - SharkSSL ESP8266 IDE makefile

The following instructions are for building the Minnow Server example using the [SharkSSL ESP8266 IDE](https://realtimelogic.com/downloads/sharkssl/ESP8266/).

1. Reconfigure the VM settings so the Linux VM has access to the Internet and restart the VM if running. When using VmWare, click Ctrl-D -> Network Adapter -> select "NAT".
2. When the VM is running, navigate to the web based Linux shell: http://IP-ADDRESS-OF-VM/webshell/
3. Login with the credentials sharkssl/SharkSSL
4. Run the following Linux commands in the WebShell (the sudo password is 'SharkSSL'):

```bash
sudo apt update  
# We need git and curl  
sudo apt install git curl   
# Navigate to examples directory  
cd ESP/esp-open-rtos/examples/  
#Put the MinnowServer, the example, and JSON lib in the ms directory.  
mkdir ms  
cd ms  
# Note that we do not need SMQ since the socket library and the secure SMQ are included in SharkSSL.  
git clone https://github.com/RealTimeLogic/MinnowServer.git  
git clone https://github.com/RealTimeLogic/JSON.git  
# Navigate to the architecture directory  
cd MinnowServer/doc/arch/  
# Copy the two required files. All other files are included in the ESP8266 SharkSSL delivery  
cp Makefile MinnowHwIntf.c ../../../  
# Navigate to the 'host build' make directory  
cd ../../example/make/  
# Amalgamate and compress www -> creates index.c  
make packwww  
# cd to the root of the example  
cd ../../../  
# Build example and upload to the connected ESP8266  
make test -j4 FLASH_MODE=dio ESPPORT=/dev/ttyUSB0  
```

If the build and upload process are successful, you should see the ESP8266 connecting to your WiFi and then you should see the text:

WebSocket server listening on 443

You may then use a browser and navigate to https://IP-ADDRESS-OF-VM. Note that you will get a certificate error, which you may bypass. Remove the certificate error as follows:

1. Add the following entry in your computer's hosts file: IP-ADDRESS-OF-VM	device
2. Install RTL's root certificate.
3. Navigate to https://device
