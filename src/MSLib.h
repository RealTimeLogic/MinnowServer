/*
 *     ____             _________                __                _
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/
 *                                                       /____/
 *
 *                 SharkSSL Embedded SSL/TLS Stack
 ****************************************************************************
 *			      HEADER
 *
 *   $Id: MSLib.h 4827 2021-08-23 20:44:04Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2013 - 2020
 *
 *   This software is copyrighted by and is the sole property of Real
 *   Time Logic LLC.  All rights, title, ownership, or other interests in
 *   the software remain the property of Real Time Logic LLC.  This
 *   software may only be used in accordance with the terms and
 *   conditions stipulated in the corresponding license agreement under
 *   which the software has been supplied.  Any unauthorized use,
 *   duplication, transmission, distribution, or disclosure of this
 *   software is expressly forbidden.
 *
 *   This Copyright notice may not be removed or modified without prior
 *   written consent of Real Time Logic LLC.
 *
 *   Real Time Logic LLC. reserves the right to modify this software
 *   without notice.
 *
 *               http://sharkssl.com
 ****************************************************************************
 *
 *  
 *  Minnow Server: SharkSSL WebSocket Server
 */

#ifndef _MSLib_h
#define _MSLib_h

#include <selib.h>

struct MST;

/** @addtogroup MSLib
@{
*/

/* RFC6455 Page 29: Opcode:  4 bits.
 * WS Opcodes with FIN=1. We do not manage WS fragments (FIN=0/1)
 * since it's not really used and the complexity is not something you
 * want in a tiny device.
 */
#define WSOP_Text   0x81
#define WSOP_Binary 0x82
/* RFC6455 5.5.  Control Frames */
#define WSOP_Close  0x88
#define WSOP_Ping   0x89
#define WSOP_Pong   0x8A


/** @defgroup MSLibErrCodes Minnow Server Error Codes
    @ingroup MSLib

    \brief Error codes returned by functions in the Minnow Server Lib.

@{
*/

/** Allocation error means the SharkSSL buffers are too small when
 * operating in secure mode, or the buffers provided to #MSTBuf is too
 * small when operating in non secure mode.
*/
#define MS_ERR_ALLOC                 -10

/** This error code is returned if WssProtocolHandshake#b64Credent is
    set and an HTTP client provides incorrect credentials.
 */
#define MS_ERR_AUTHENTICATION        -11

/** The HTTP request header may be stored in the SharkSSL send
    buffer. This error indicates that the send buffer must be
    increased.
 */
#define MS_ERR_HTTP_HEADER_OVERFLOW  -12

/** Received invalid HTTP header from client */
#define MS_ERR_INVALID_HTTP          -13

/** The request was successful, but it is not a WebSocket connection. */
#define MS_ERR_NOT_WEBSOCKET         -14

/** Socket read error */
#define MS_ERR_READ                  -15

/** Socket read timeout */
#define MS_ERR_READ_TMO              -16

/** The initial SSL handshake failed */
#define MS_ERR_SSL_HANDSHAKE         -17

/** Socket write error */
#define MS_ERR_WRITE                 -18

/** Encrypted ZIP file not supported by ZipFileSystem */
#define MS_ERR_ENCRYPTED_ZIP         -30

/** Error reported by #MSFetchPage */
#define MS_ERR_FILE_IO               -31

/** Incorrect parameter use when calling #MS_prepSend and then calling
    one of #MS_sendBin or #MS_sendText.
 */
#define MS_ERR_BUF_OVERFLOW          -40

/** Incorrect parameter use when calling #MS_prepSend and then calling
    one of #MS_sendBin or #MS_sendText.
 */
#define MS_ERR_BUF_UNDERFLOW         -41

/** @} */ /* end group MSLibErrCodes */ 


#define MAX_HTTP_H_SIZE 20

/** A page fetch callback function used by function MS_ebServer is
    typically installed when the web application is stored on the
    device.
    \sa ZipFileSystem
 */
typedef int (*MSFetchPage)(void* hndl, struct MST* mst,U8* path);

/** The WssProtocolHandshake structure keeps state information for the
    web server function MS_ebServer. An instance of this structure
    must be initialized as follows:
    \code
    WssProtocolHandshake wph={0};
    \endcode
 */
typedef struct {
   /** In param: Enable HTTP basic authentication by setting
       'b64Credent' to a B64 encoded string of 'username:password'

       All browsers support HTTP authentication using standard
       requests, but few browsers support HTTP authentication on
       WebSocket connection upgrades. You may therefore consider
       creating your own authentication on top of a newly established
       WebSocket connection. You can create your server side logic
       such that no commands are accepted until the client
       authenticates. Another option is to require that clients
       authenticate by using SSL certificates.

       \sa #MS_ERR_AUTHENTICATION
   */
   const U8* b64Credent;

   /** In param: Set the HTTP basic authentication's 'realm' name.
    */
   const U8* realm;

   /** In param: set the page fetch callback function if you plan on
       storing the web application in the device.

      <b>Example code:</b>
      \code
      WssProtocolHandshake wph={0};
      ZipFileSystem zfs;
      wph.fetchPage = msInitZipFileSystem(&zfs, getLedZipReader());
      wph.fetchPageHndl=&zfs;
      \endcode

      \sa msInitZipFileSystem
   */
   MSFetchPage fetchPage;

   /** In param: The MSFetchPage handle, if any */
   void* fetchPageHndl;

   /** Out param: Set to the initial HTTP header request line.
       Example:
       \code
       GET /path/to/file/index.html HTTP/1.0
       \endcode
   */
   U8* request;

   /** Out param: Set to the HTTP WebSocket header 'origin', if sent
       by the client.
   */
   U8* origin;

   /** HTTP header keys sent by the client. NULL signals end of array.
    */
   U8* hKeys[MAX_HTTP_H_SIZE];

   /** All HTTP header values sent by the client. NULL signals end of array.
    */
   U8* hVals[MAX_HTTP_H_SIZE];
} WssProtocolHandshake;


/** The WS protocol is frame based, and struct WssReadState keeps state
    information for the Minnow Server's #MS_read function.
 */
typedef struct
{
   /* Private members */
   U8* overflowPtr; /* Set if: consumed more data from stream than frame len */ 
   int overflowLen; /* overflowPtr len is used internally in wsRawRead */
   int frameHeaderIx; /* Cursor used when reading frameHeader from socket */
   U8* maskPtr;
   U8 frameHeader[8]; /*[0] FIN+opcode, [1] Payload len, [2-3] Ext payload len*/

   /** The WebSocket frame length.
    */
   int frameLen;

   /**  Read frame data until: frameLen - bytesRead = 0
    */
   int bytesRead;

   /** Set when function msRead returns due to a timeout.
   */
   U8 isTimeout;
} WssReadState;


/* Minnow Server Transmission Interface
 */

/** The Minnow Server Transmission Buffer is used by the Minnow Server
 * Transmission (MST) class when used in non secure mode. The buffer
 * emulates the SharkSSL send/receive buffer mechanism.
 * The send/receive buffers are set with #MS_setSocket.
 */
typedef struct {
   U8* recBuf;
   U8* sendBuf;
   U16 recBufSize;
   U16 sendBufSize;
} MSTBuf;


/** The Minnow Server Transmission Class defines a set of functions
 * for reading and writing socket data either in secure mode using
 * SharkSSL or non secure mode.
 */
typedef struct MST
{
   SOCKET* sock;
#ifdef MS_SEC
   union {
      SharkSslCon* sc;
      MSTBuf b;
   } u;
#else
   MSTBuf b;
#endif
   BaBool isSecure;
} MST;

/** Get the send buffer pointer.
 */
U8* MST_getSendBufPtr(MST* o);

/** Get the send buffer size.
 */
U16 MST_getSendBufSize(MST* o);

/** Write data to a secure layer or non secure (standard socket) layer.
    \param o MST instance
    \param buf create a pointer (U8* ptr) and pass in the pointer's
    address when calling this function. The pointer is set to the
    receive buffer when the functiion returns.
    \param timeout in milliseconds
*/ 
int MST_read(MST* o,U8 **buf,U32 timeout);

/** Write data to a secure layer or non secure (standard socket) layer.
    \param o MST instance
    \param buf the data to send or NULL if data to send was saved to the
    return value from MST_getSendBufPtr.
    \param len 'buf' length
*/ 
int MST_write(MST* o,U8* buf, int len);


/** MS: Minnow Server HTTP(S) and (secure) WebSocket Server
 */
typedef struct
{
   WssReadState rs;
   MST mst;
} MS;



#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup MsHelperFunc Minnow Server Helper Functions
    @ingroup MSLib

    \brief Minnow Server Helper Functions.

@{
*/

/* Similar to the standard ANSI function strstr, except for that it
   does not rely on str being a zero terminated string. The compare is
   also case insensitive.

  \param str string to search.
  \param slen str len
  \param substrs substring to find in str.
*/

U8* msstrstrn(U8* str, int slen, const U8* substr);

/** Copies 'src' to 'dest'
    \param dest the destination buffer
    \param dlen destination buffer length
    \param src buffer to copy
    \param slen 'src' length
    \return 'dest' pointer incremented by amount of data added
 */
U8* msCpAndInc(U8* dest, int* dlen, const U8* src, int slen);


/** Encodes 'src' as B64 and adds the string to 'dest'
    \param dest the destination buffer
    \param dlen destination buffer length
    \param src (binary) buffer to B64 encode
    \param slen 'src' length
    \return 'dest' pointer incremented by amount of data added
 */
U8* msB64Encode(U8* dest, int* dlen, const U8* src, int slen);


/** Formats 'n' as a string and adds the string to 'dest'
    \param dest the destination buffer
    \param dlen destination buffer length
    \param n the number to format
    \return 'dest' pointer incremented by amount of data added
 */
U8* msi2a(U8* dest, int* dlen, U32 n);


/** Adds the following HTTP response to buffer 'dest'
    \code
    HTTP/1.0 200 OK
    Content-Length: ['contentLen' converted 2 string]

    \endcode
    \param dest the destination buffer
    \param dlen destination buffer length
    \param contentLen the size of the static HTML page
    \param extHeader optional extra headers formatted as '\\r\\nkey:val'
    \return 'dest' pointer incremented by amount of data added
 */
U8* msRespCT(U8* dest, int* dlen, int contentLen, const U8* extHeader);

/** @} */ /* end group MsHelperFunc */ 


/** Minnow Server Constructor
    \param o MS instance
 */
#define MS_constructor(o) memset(o,0,sizeof(MS))

/** Set the SharkSSL object and the socket connection after socket
 * select accepts and creates a new server socket. The Minnow server
 * instance will operate in secure (SSL) mode when this function is
 * called.
 * \param o the Minnow Server (#MS) instance
 * \param sharkCon the object returned by SharkSsl_createCon
 * \param socket the #SOCKET instance created after accepting a new client
 * \sa MS_setSocket
 */
#define MS_setSharkCon(o, sharkCon, socket)                \
   (o)->mst.u.sc=sharkCon,                                 \
      (o)->mst.sock=socket,                                \
      (o)->mst.isSecure=TRUE
   
#ifdef MS_SEC
#define MS_setSocket(o,socket,rec,recSize,send,sendSize)         \
   (o)->mst.sock=socket,                                         \
      (o)->mst.u.b.recBuf=rec,                                   \
      (o)->mst.u.b.recBufSize=recSize,                           \
      (o)->mst.u.b.sendBuf=send,                                 \
      (o)->mst.u.b.sendBufSize=sendSize,                         \
      (o)->mst.isSecure=FALSE
#else

/** Set the socket connection after socket
 * select accepts and creates a new server socket. The Minnow server
 * instance will operate in non secure mode when this function is
 * called.
 * \param o the Minnow Server (#MS) instance
 * \param socket the #SOCKET instance created after accepting a new client
 * \param rec a buffer the Minnow Server instance can use when receiving data
 * \param recSize size of 'rec' buffer
 * \param send a buffer the Minnow Server instance can use when writing data
 * \param sendSize size of 'send' buffer
 * \sa MS_setSharkCon
 */

#define MS_setSocket(o,socket,rec,recSize,send,sendSize)         \
   (o)->mst.sock=socket,                                         \
      (o)->mst.b.recBuf=rec,                                   \
      (o)->mst.b.recBufSize=recSize,                           \
      (o)->mst.b.sendBuf=send,                                 \
      (o)->mst.b.sendBufSize=sendSize,                         \
      (o)->mst.isSecure=FALSE
#endif      

/** Format a HTTP 200 OK response with Content Length.
    \param o Minnow Server.
    \param dlen destination buffer length.
    \param contentLen Content Lengt.
    \param extHeader optional extra header values.
    \return
    The input parameter 'dest' on success or NULL if the destination
    buffer is not sufficiently large for the response.
*/
U8* MS_respCT(MS* o, int* dlen, int contentLen, const U8* extHeader);

/** The Web Server function is responsible for parsing incoming HTTP
    requests and upgrading HTTP requests to WebSocket
    connections.

    It's main functionality is designed for upgrading an HTTPS request
    to a WebSocket connection; however, it also includes basic
    functionality for responding to HTTP static content requests such
    as fetching HTML files, JavaScript code, etc.

    <b>Note:</b> WssProtocolHandshake#fetchPage must have been
    initialized for the function to respond to HTTP GET requests.

    \return Zero on successful WebSocket connection upgrade. Returns
    an error code for all other operations, including fetching static
    content using the callback MSFetchPage -- in this case,
    #MS_ERR_NOT_WEBSOCKET is returned. See \link MSLibErrCodes Error
    Codes \endlink for more information.
 */
int MS_webServer(MS *o, WssProtocolHandshake* wph);

/** Prepare sending a WebSocket frame using one of MS_sendBin or
    MS_sendText. This function returns a pointer to the SharkSSL send
    buffer, offset to the start of the WebSocket's payload
    data. The maximum payload data is returned in 'maxSize'.

    \param o Minnow Server instance.
    
    \param extSize is a Boolean value that should be set to FALSE if
    the payload will be less than or equal to 125 bytes. The parameter
    must be set to TRUE if the payload will be greater than 125 bytes.

    \param maxSize is an out value set to the SharkSSL buffer size
    minus the WebSocket frame size.

    \return The SharkSSL send buffer, offset to the start of the
    WebSocket's payload data. This function cannot fail.
*/
U8* MS_prepSend(MS* o, int extSize, int* maxSize);

int MS_send(MS* o, U8 opCode, int len);


/** Send a WebSocket binary frame using the SharkSSL zero copy API.

    \param o Minnow Server instance.

    \param len is the length of the buffer you copied into the pointer
    returned by #MS_prepSend.

    \return The data length 'len' on success or an \link MSLibErrCodes
    Error Code \endlink on error.

    \sa MS_writeBin
*/
#define MS_sendBin(o, len) MS_send(o, WSOP_Binary, len)


/** Send a WebSocket text frame using the SharkSSL zero copy API.

    \param o Minnow Server instance.

    \param len is the length of the buffer you copied into the pointer
    returned by #MS_prepSend.

    \return The data length 'len' on success or an \link MSLibErrCodes
    Error Code \endlink on error.

    \sa MS_writeText
*/
#define MS_sendText(o, len) MS_send(o, WSOP_Text, len)


/** Function used by the two inline functions (macros) #MS_writeBin and
    #MS_writeText.

    \param o Minnow Server instance.
    \param opCode the WebSocket opcode.
    \param data the data to send.
    \param len data length.

    \return zero success or an \link MSLibErrCodes
    Error Code \endlink on error.

    \sa MS_writeText
 */
int MS_write(MS *o, U8 opCode,const void* data,int len);

/** Sends data as WebSocket text frame(s). The data will be sent as
    multiple frames if len is longer than the SharkSSL send buffer
    size.
    \param o Minnow Server instance.
    \param data the data to send.
    \param len data length.

    \return Zero on success or an \link MSLibErrCodes Error Code
    \endlink on error.

    \sa MS_sendBin
 */
#define MS_writeBin(o,data,len) MS_write(o,WSOP_Binary,data,len)

/** Send data as WebSocket binary frame(s). The data will be sent as
    multiple frames if len is longer than the SharkSSL send buffer
    size.
    \param o Minnow Server instance.
    \param data the data to send.
    \param len data length.

    \return Zero on success or an \link MSLibErrCodes Error Code
    \endlink on error.

    \sa MS_sendText
 */
#define MS_writeText(o,data,len) MS_write(o,WSOP_Text,data,len)

/** Sends a WebSocket close frame command to the peer and close the socket.

    \param o Minnow Server instance.

    \param statusCode is a <a target="_blank" href=
    "http://tools.ietf.org/html/rfc6455#section-7.4">
    WebSocket status code</a>.

    \return The inverted statusCode on success or an \link
    MSLibErrCodes Error Code \endlink if sending the close frame command to
    the peer failed.
 */
int MS_close(MS *o, int statusCode);

/** Waits for WebSocket frames sent by the peer side. The function
    returns when data is available or on timeout. The function returns
    zero on timeout, but the peer can send zero length frames, so you must
    verify that it is a timeout by checking the status of
    WssReadState#isTimeout.
  
    The WebSocket protocol is frame based, but the function can return
    a fragment before the complete WebSocket frame is received if the frame
    sent by the peer is larger than the SharkSSL receive buffer. The
    frame length is returned in WssReadState#frameLen, and the data
    consumed thus far is returned in WssReadState#bytesRead. The
    complete frame is consumed when frameLen == bytesRead.

    \param o Minnow Server instance.
    \param buf is a pointer set to the SharkSSL receive buffer, offset
    to the start of the WebSocket payload data.
    \param timeout in milliseconds. The timeout can be set to #INFINITE_TMO.

    \return The payload data length or zero for zero length frames and
    timeout.  The function returns one of the See \link MSLibErrCodes
    Error Codes \endlink on error.
*/
int MS_read(MS *o,U8 **buf,U32 timeout);

#ifdef __cplusplus
}
#endif

/** @} */ /* end group MSLib */ 

#endif
