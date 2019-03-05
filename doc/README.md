# Minnow Server Reference Platform Specification

The following details the communication API, the security concept used by the reference platform for storing credentials on a device, and the authentication mechanism used.

## Asynchronous JSON Communication

A message is an array of two elements, the message-name and the message body. The message body may be a primitive type, an object or array.

For an introduction to the JSON Communication concept, see the [Minnow Server tutorial: How to Communicate Between the Browser and Device using WebSockets](https://realtimelogic.com/articles/Creating-SinglePage-Apps-with-the-Minnow-Server#communicate)

* **Message devname** sent from device to browser after connection establishment: ["devname",["the-device-name"]]
* **Message nonce** is sent from device to browser just after sending **devname** or each time the user provides wrong credentials: ["nonce"," b64-encoded-12-byte-nonce "]. See authentication and password management below for more information on how the nonce is used.
* **Message auth** is sent from browser to client: ["auth", {name:"string",hash:"string"}], where the hash is a 40 byte long password hash in hex form (a 20 byte SHA-1 hash)
* **Message ledinfo** is sent from device to browser if the authentication was successful: ["ledinfo", {"leds" : [...]}], where ... is one or several objects of type {"name":"ledname","id":number, "color":"string", "on":boolean}
* **Message setled** is sent from both browser and device. The browser sends this to the device when a user clicks a button. The device sends **setled** as an ack when receiving **setled** or when a button is clicked directly in the device. The UI state should be updated when the browser receives **setled**. Message structure: ["setled", {"id":number,"on":boolean}], where id is a number from the **ledinfo** message.
* **Message settemp** is sent from device to browser when temperature changes: ["settemp", number]
* **Message uploadack** is sent from the device to the client as a response to the two binary frames **Upload** and **UploadEOF** sent from the client to the server: ["uploadack", number], where number is how many binary chunks the server has received so far. One **uploadack** is sent for each 10 binary **Upload** messages received. The message is used for flow control in the client. See binary frames for details.
* **Message AJAX** an AJAX encapsulated message. The data sent from the client to the server includes: ["AJAX", [service, rpcID, args]], where args is an array with a copy of the arguments passed into the AJAX function. The response data sent from the server to the client includes: ["AJAX", [rpcID, response]], where response is an object, array, or primitive type and is the data sent as a response message by the inquired AJAX service.

## AJAX over WebSockets

The example supports AJAX in addition to providing asynchronous JSON communication. AJAX is an encapsulated message sent over the asynchronous JSON WebSocket connection bus. AJAX is a type of remote procedure call that includes a request and response. The included AJAX over WebSockets functionality is virtually identical to the example code provided in the [AJAX over WebSockets tutorial](https://makoserver.net/articles/AJAX-over-WebSockets) .

**AJAX messages:**
* **math/add** - identical to math/add in the AJAX over WebSockets tutorial
* **math/subtract** - identical to math/subtract in the AJAX over WebSockets tutorial
* **math/mul** - multiply
* **math/div** - divide
* **auth/setcredentials** is sent when the user sets a new username/password. The AJAX function is called with four arguments ajax(curUname, curPwd, newUname, newPwd). The response is boolean true or an AJAX exception.

## Binary Messages

In addition to the JSON text frames, binary frames are used when sending binary data. The binary data is sent from the client to the server when the user uploads new firmware. The Web Interface supports drag and drop firmware upload. The upload is split into chunks by the JavaScript client and sent over the WebSocket connection as binary frames. A binary frame includes the message type as the first byte and is one of **Upload** or **UploadEOF**, where **UploadEOF** signals the end of the binary firmware upload. WebSockets and TCP/IP support flow control, but the API provided in browsers for sending WebSockets does not. The browser's JavaScript WebSocket API does not specify what happens to large amounts of queued data inside the browser and some browsers fail when too much data is queued. For this reason, the firmware upgrade logic includes flow control. The device sends the JSON message **uploadack** for each tenth binary message received. The JavaScript client code is designed such that a maximum of 20 messages is queued at any time.

## Authentication and Password Management

The example includes logic that makes it more difficult for an attacker to eavesdrop and extract the credentials from the device. We use [SHA-1](https://en.wikipedia.org/wiki/SHA-1) hashing for making it difficult to both extract the credentials from the device and eavesdrop on the credentials.

For an introduction to the secure authentication concept, see the [Minnow Server tutorial: How to Authenticate the User](https://realtimelogic.com/articles/Creating-SinglePage-Apps-with-the-Minnow-Server#authenticate)

There are several security aspects one must consider when dealing with credentials, such as:

* Storing credentials on the device in cleartext or in an encrypted form
* Preventing an eavesdropper from recording credentials when authenticating
* Preventing an eavesdropper from recording credentials when setting new credentials

**Note:** the default username and password embedded in the server code is "root" and "password". These credentials are disabled as soon as the user sets his own personal credentials.

**Credentials stored on the device:**

An attacker may attempt to directly extract the stored credentials from a device that has been physically compromised. Username and passwords stored in clear text are easy to find when an attacker has physical access to a device. To make it much more difficult to find the stored credentials, the credentials are not stored as strings since strings are easy for an attacker to find. Instead, the username and password are each stored as a 20 byte SHA-1 hash. You may find ways to store the 40 byte data array together with other binary data, making it very difficult for an attacker to find the two SHA-1 hashes. Note that using SHA-1 or any other hashing algorithm should not be considered encryption since the two SHA-1 hashes, if compromised, can effectively be used as credentials when authenticating with the server.

**When user sends login credentials to the device:**

The password is not sent in cleartext when the user authenticates with the server. Instead, the password is sent as a SHA-1 hash. However, a simple SHA-1 on the password does not prevent relay attacks since an eavesdropper could simply record the SHA-1 hash and use this to login to the device. To prevent relay attacks, the device sends a nonce (one time key) to the client for each login attempt. The client creates a SHA-1 hash by combining the password with the nonce, thus preventing relay attacks.

The client must perform two SHA-1 hash calculations since the device stores the credentials as a SHA-1 hash. The device would not be able to verify the password if the client did not perform a double hash calculation.

The SHA-1 password hash sent by the client to the server is calculated as follows:

SHA-1(SHA-1(password) + nonce)

**When user sends new credentials to the device:**

The credentials are sent in cleartext from the client to the server when the user sets new credentials. Performing a SHA-1 hash on the password would not help since the hashed SHA-1 password would effectively be a password an eavesdropper could use.

This means that there is a security problem when a user sets new credentials and when the WebSocket connection is not protected by TLS. Setting new credentials will in most cases only be performed one time when the device is used for the first time, making it less likely to be compromised. However, if security is important, consider using TLS to protect the WebSocket connection.

