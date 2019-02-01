/*

  Minnow Server Reference Example

 ## Use this example with or without TLS.
 The code can be used with or without a TLS stack. The code is
 tailored for the SharkSSL API and the code automatically uses the
 SharkSSL API if compiled together with a SharkSSL delivery. The code
 uses standard non secure connections if not using TLS. See macro
 MS_SEC below for details. Note that all Minnow Server and SMQ APIs
 used in this code utilize the secure API. This also works for non secure
 mode since the header files includes macros that redirect the secure
 API to the non secure API.

 Minnow Server (MS) Reference manual:
 https://realtimelogic.com/ba/doc/en/C/shark/group__MSLib.html


 ## Macro USE_SMQ: compile with or without IoT functionality
 As explained in the example's tutorial, the example by default
 operates as a WebSocket server only. Enable IoT mode by compiling
 with macro USE_SMQ defined.

 SMQ Reference manual
  Standard: https://realtimelogic.com/ba/doc/en/C/reference/html/structSMQ.html
  Secure: https://realtimelogic.com/ba/doc/en/C/shark/group__SMQLib.html


 ## JSON:
 This example requires the following JSON library:
 https://github.com/RealTimeLogic/JSON
 The parser, the JSON node factory, and the JSON encoder can be used
 without a dynamic allocator as we do by default in this code. See the
 macro USE_STATIC_ALLOC below for details. The JSON parser takes a
 JSON value node factory as an argument. The JSON library supports two
 node factories, one design for super small microcontrollers and
 another designed for simplicity. The one for super small micros is
 called JDecoder and the one used for simplicity is called
 JParserValFact. We use JParserValFact in the code below since
 JDecoder is more difficult to use. For a detailed introduction to the
 various JParser options, see the following tutorial:
 https://realtimelogic.com/ba/doc/en/C/reference/html/md_en_C_md_JSON.html
*/




/* 'MS_SEC' gets automatically defined in 'MSLib.h' when compiled with
   a SharkSSL delivery. The code excludes TLS if MS_SEC is not defined.
*/
#include <MSLib.h>
#ifdef MS_SEC
#include "certificates/device_RSA_2048.h"
#endif


/* IoT mode: include the optional SMQ client if 'USE_SMQ' is defined */
#ifdef USE_SMQ
#include <SMQ.h>
#endif


/* JSON and static allocator:

  The JSON parser and the JVal node factory require a memory
  allocator, but you do not need to use a standard dynamic memory
  allocator. In the following which is enabled by default, three
  allocators are used that is internally using some very small static
  buffers. This type of allocation works great for small
  microcontrollers with little memory since we do not run into
  fragmentation issues and we have better use of the memory. See the
  file JsonStaticAlloc.c for more on how the allocators work.

  Remove '#define USE_STATIC_ALLOC' if you want to use standard
  dynamic allocation. You can still control what dynamic allocator to
  use by setting the macros baMalloc, baRealloc, and beFree.
 */
#define USE_STATIC_ALLOC
#ifdef USE_STATIC_ALLOC
#include "JsonStaticAlloc.h"
static AllocatorIntf jpa; /* JParser Alloc */
static AllocatorIntf va; /* JVal Node Allocator */
static AllocatorIntf da; /* JVal String Allocator */
#endif


/* JSON lib */
#include <JParser.h>
#include <JEncoder.h> 

#include <stdio.h>
#include <stdlib.h>

/* Part of example code for managing LEDs.
   The header file can be found in the SMQ example.
*/
#include "ledctrl.h"


/* If IoT enabled */
#ifdef USE_SMQ
/* The domain name (or IP address) for the (online) SMQ broker.
 */
#ifndef SMQ_DOMAIN
#define SMQ_DOMAIN "MakoServer"
#endif
#ifdef MS_SEC
#define SMQ_PROTOCOL "https://"
#else
#define SMQ_PROTOCOL "http://"
#endif
/* The url must include '?device=' so the server side knows this is a
 * device. See the file IoT/www/.preload for details.
 */
#define SMQ_URL SMQ_PROTOCOL SMQ_DOMAIN "/minnow-smq.lsp?device="
#endif


#ifndef MS_SEC

/* We need a send and receive buffer for the Minnow Server (MS) when
   using the standard (non secure version). The SharkSSL send/receive
   buffers are used in secure mode.
 */
struct{
   U8 rec[1500];
   U8 send[1500];
} msBuf;
#endif


 /* Fetch the SPA. See index.c for details. */
extern int fetchPage(void* hndl, MST* mst, U8* path);



/****************************************************************************
 **************************-----------------------***************************
 **************************| BOARD SPECIFIC CODE |***************************
 **************************-----------------------***************************
 ****************************************************************************/

/*
  The macro HOST_PLATFORM must be defined if compiled and run on a
  non embedded platform (HLOS). The code within this section sets up the
  simulated environment. Do not set this macro if the code is to be
  cross compiled for an embedded device.

  See the following documentation for how to interface the LED
  functions to the hardware:

  https://realtimelogic.com/ba/doc/en/C/shark/md_md_Examples.html#LedDemo
*/


#if HOST_PLATFORM

/*
  Peripheral simulation code. Not to be used for production.
*/

/* Include the simulated LED environment/functions */
#include "led-host-sim.ch"

/* The out value 'digest' is the value SHA-1(string)
 */
static void
string2Sha1(const char* string, U8 digest[20])
{
   SharkSslSha1Ctx ctx;
   SharkSslSha1Ctx_constructor(&ctx);
   SharkSslSha1Ctx_append(&ctx, (U8*)string, strlen(string));
   SharkSslSha1Ctx_finish(&ctx, digest);
}

/* Returns 0 if username found and passwords matches, otherwise a negative
   value is returned.
   params:
    name: the username from the browser in cleartext
    nonce: the nonce that was sent to the browser.
    hash: the password hash calulated by the browser i.e.
          SHA-1(SHA-1(password) + nonce)

  In this example code, new credentials set by the user are saved in
  the file "CREDENTIALS.SHA1". You should find a way to hide this
  information and/or include the data in a larger binary blob.
 */
static int
checkCredentials(const char* name, U8 nonce[12], const U8 hash[20])
{
   /* The hard coded default credentials */
   static const U8 defaultUnameSha1[]={ /* SHA1('root') */
      0xDC,0x76,0xE9,0xF0,0xC0,0x00,0x6E,0x8F,0x91,0x9E,
      0x0C,0x51,0x5C,0x66,0xDB,0xBA,0x39,0x82,0xF7,0x85};
   static const U8 defaultPasswordSha1[]={ /* SHA1('password') */
      0x5B,0xAA,0x61,0xE4,0xC9,0xB9,0x3F,0x3F,0x06,0x82,
      0x25,0x0B,0x6C,0xF8,0x33,0x1B,0x7E,0xE6,0x8F,0xD8};
   const U8* unameSha1=defaultUnameSha1;
   const U8* passwordSha1=defaultPasswordSha1;
   U8 digest[20];
   U8 unamePwdSha1[40];
   FILE* fp = fopen("CREDENTIALS.SHA1", "rb");
   if(fp) /* Use default or user saved credentials */
   {
      if(fread(unamePwdSha1, 40, 1, fp) == 1)
      {
         unameSha1=unamePwdSha1;
         passwordSha1=unamePwdSha1+20;
      }
      fclose(fp);
   }

   /* Convert username to sha1 */
   string2Sha1(name,digest);
   if( ! memcmp(digest, unameSha1, 20) )
   {  /* If username matches locally stored SHA-1 hash */
      if(nonce)
      {
         SharkSslSha1Ctx ctx;
         /* Check if password matches. The browser sent us
            SHA-1(SHA-1(password) + nonce). We need to calculate the same
            hash before we can compare the two password hashes.
         */
         SharkSslSha1Ctx_constructor(&ctx);
         SharkSslSha1Ctx_append(&ctx, passwordSha1, 20);
         SharkSslSha1Ctx_append(&ctx, nonce, 12);
         SharkSslSha1Ctx_finish(&ctx, digest);
         /* Return 0 if local hash matches hash calculated by browser */
         return memcmp(digest, hash, 20) == 0 ? 0 : -1;
      }
      else
         return memcmp(hash, passwordSha1, 20) == 0 ? 0 : -1;
   }
   return -2; /* name not found */
}

/* Called when user sets new credentials in the browser.
 */
static int
setcredentials(const char* username, const char* password)
{
   /* See the comment in 'checkCredentials()' regarding the location
    * of the saved credentials.
    */
   FILE* fp = fopen("CREDENTIALS.SHA1", "wb");
   if(fp)
   {
      /* Convert username and password to SHA-1 hashes and save the data. */
      U8 digest[40];
      string2Sha1(username, digest);
      string2Sha1(password, digest+20);
      fwrite(digest, 40, 1, fp);
      fclose(fp);
      return 0;
   }
   return -1;
}


/*
  A basic firmware example "save" function that saves a file to
  FIRMWARE.bin. We use a static variable here for simplicity.

  'data' and 'len': a firmware chunk and chunk length.
  The 'open' argument guards against previously failed uploads.
  'eof': end of file i.e. done uploading.

  Note: you can easily sign the firmware using a private key and check
  the firmware in this file using the public certificate. See the
  following whitepaper for details:
  https://realtimelogic.com/downloads/docs/IoT-Security-Solutions.pdf
 */
static int
saveFirmware(U8* data, int len, BaBool open, BaBool eof)
{
   static FILE* fp;
   if(open && fp)
   {
      fclose(fp);
      fp=0;
   }
   if(!fp)
   {
      fp=fopen("FIRMWARE.bin","wb");
      if(!fp)
      {
         xprintf(("Cannot open FIRMWARE.bin\n"));
         return -1;
      }
   }
   if(data)
      fwrite(data, (size_t)len, 1, fp);
   if(eof)
   {
      fclose(fp);
      fp=0;
   }
   return 0;
}


#ifndef NO_MAIN

#ifndef _WIN32
#include <signal.h>
static void
ignoreSignal(int sig)
{
   struct sigaction sa;
   sa.sa_handler = SIG_IGN;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sigaction(sig, &sa, NULL);
}
#endif

int
main()
{
   xprintf(("%s",
            "Minnow Server and IoT example.\n"
            "Copyright (c) 2019 Real Time Logic.\n"
            "\n"));
#ifdef _WIN32
   /* Windows specific: Start winsock library */
   { WSADATA wsaData; WSAStartup(MAKEWORD(1,1), &wsaData); }
#else
   /* Assuming POSIX (Linux). Prevent signals from terminating program */
    ignoreSignal(SIGHUP);
    ignoreSignal(SIGTTOU);
    ignoreSignal(SIGTTIN);
    ignoreSignal(SIGTSTP);
    ignoreSignal(SIGPIPE);
    ignoreSignal(SIGXFSZ);
   setConioTerminalMode();
#endif
   mainTask(0);
   xprintf(("Exiting...\n"));
   return 0;
}
#endif

#else /* HOST_PLATFORM */
/* target env */
extern int setcredentials(const char* username, const char* password);
extern int checkCredentials(const char* name, U8 nonce[12], const U8 hash[20]);
extern int saveFirmware(U8* data, int len, BaBool open, BaBool eof);
/* See ledctrl.h for additional required interfaces. */
#endif /* HOST_PLATFORM */


/****************************************************************************
 **************************----------------------****************************
 **************************| GENERIC CODE BELOW |****************************
 **************************----------------------****************************
 ****************************************************************************/

/******************************  SendData ************************************/

/* Connection Data: this container object stores either a Minnow
   Server or an SMQ connection.
 */
typedef struct {
   union { /* union: Minnow or SMQ */
#ifdef USE_SMQ
      struct {
         SharkMQ smq;
         U32 browserTID; /*Initially set to 1, then to browser's TID(Ref-etid)*/
      }s;
#endif
      MS* ms; /* MS: Minnow Server */
   } u; /* Union: MS or SMQ */
#ifdef USE_SMQ
   BaBool isWS; /* TRUE: WebSocket, FALSE: SMQ */
#endif
} ConnData;

#ifdef USE_SMQ
#define ConnData_setWS(o,_ms) (o)->u.ms=_ms,(o)->isWS=TRUE;
#define ConnData_WebSocketMode(o) (o)->isWS
#else
#define ConnData_setWS(o,_ms) (o)->u.ms=_ms
#define ConnData_WebSocketMode(o) TRUE
#endif


/*   This container object stores data objects used when encoding/sending JSON.
     See function SendData_wsSendJSON for an explanation on how the
     objects are used.
 */
typedef struct {
   BufPrint super; /* Ref-bp: Used as super class. Buffer needed by JEncoder */
   JErr err;
   JEncoder encoder;
   BaBool committed; /* Send: If a complete JSON message assembled */
} SendData;


/* Send a complete JSON message over WebSockets.

   This is the BufPrint flush function that gets called when the
   BufPrint buffer is flushed, either when the buffer is full or when
   actively flushed, however, this code is designed to only accept a
   full JSON message, thus an indirect flush is not accepted. The flag
   "committed" helps us check if we are performing an active flush in
   SendData_commit. The buffer used by BufPrint is either the Minnow
   Server buffer or the SMQ buffer. The buffer is too small if the
   flush callback is called and if the flag "committed" is false.

   The JEncoder object (used for encoding JSON) requires a BufPrint
   instance.
   https://realtimelogic.com/ba/doc/en/C/reference/html/structJEncoder.html
   https://realtimelogic.com/ba/doc/en/C/reference/html/structBufPrint.html
 */
static int
SendData_wsSendJSON(BufPrint* bp, int sizeRequired)
{
   MS* ms;
   SendData* o = (SendData*)bp; /* (Ref-bp) */
   (void)sizeRequired; /* not used */
   /* From SendData_constructor >  BufPrint_setBuf */
   ms = (MS*)BufPrint_getUserData(bp);
   if( ! o->committed )
   {
      xprintf(("ERR: WebSocket send buffer too small\n"));
      baAssert(0);/* This is a 'design' error */
      return -1;
   }

   /* Minnow Server (MS) in large mode. Pad with spaces if size less than
      128 (Ref-Size).
      https://realtimelogic.com/ba/doc/en/C/shark/group__MSLib.html
    */
   while(bp->cursor < 128)
      bp->buf[bp->cursor++] = ' '; /* cursor is current bufsize */
   if(MS_sendText(ms, bp->cursor) < 0)
   {
      xprintf(("WebSocket connection closed on send\n"));
      return -1;
   }
   return 0;
}

/* If IoT enabled */
#ifdef USE_SMQ
/* Similar to SendData_wsSendJSON, but sends data over an SMQ
   connection and not a WebSocket connection. The function is
   activated if SendData_commit() is called and the ConnData send
   state is 'SMQ' -- i.e. when ConnData_WebSocketMode() returns false.
 */
static int
SendData_smqSendJSON(BufPrint* bp, int sizeRequired)
{
   ConnData* cd;
   SendData* o = (SendData*)bp; /* (Ref-bp) */
   (void)sizeRequired; /* not used */
   /* From SendData_constructor >  BufPrint_setBuf */
   cd = (ConnData*)BufPrint_getUserData(bp);
   if( ! o->committed )
   {
      xprintf(("SMQ buffer too small\n"));
      baAssert(0);/* This is a 'design' error */
      return -1;
   }

   /* Publish to the SMQ ephemeral topic ID 'browserTID', which is
      initially one (the server) and later set to whatever the
      ephemeral browser TID is (Ref-etid). For more information on
      SMQ addressing and ephemeral topic IDs, see:
      https://realtimelogic.com/ba/doc/?url=SMQ.html#SendersAddress
    */
#ifdef MS_SEC
   /* TLS connection: Data set to NULL for zero copy SharkSSL API. */
   if(SharkMQ_publish(&cd->u.s.smq, NULL, bp->cursor, cd->u.s.browserTID, 1))
#else
   if(SMQ_publish(&cd->u.s.smq, bp->buf, bp->cursor, cd->u.s.browserTID, 1))
#endif

   {
      xprintf(("Socket err: SMQ connection closed on send\n"));
      return -1;
   }
   return 0;
}
#endif

/* Construct the SendData container object used for sending formatted JSON.
   We use a BufPrint instance as the output buffer for JEncoder (Ref-bp).
   https://realtimelogic.com/ba/doc/en/C/reference/html/structBufPrint.html
   https://realtimelogic.com/ba/doc/en/C/reference/html/structJEncoder.html
*/
static void
SendData_constructor(SendData* o, ConnData* cd)
{
   int sendBufSize;
   U8* buf;
   if(ConnData_WebSocketMode(cd)) /* Always true if USE_SMQ not set */
   {
      BufPrint_constructor(&o->super, cd->u.ms, SendData_wsSendJSON);
       /* Minnow Server: second arg=TRUE: Set size > 128 (Ref-Size) */
      buf = MS_prepSend(cd->u.ms, TRUE, &sendBufSize);
      /* JEncoder is formatting data via BufPrint directly into MS buffer */
      BufPrint_setBuf(&o->super, (char*)buf, sendBufSize);
   }
#ifdef USE_SMQ
   else
   {
      BufPrint_constructor(&o->super, cd, SendData_smqSendJSON);
#ifdef MS_SEC
      /* Use the SharkSSL buffer and the zero copy API */
      BufPrint_setBuf(&o->super, (char*)SharkMQ_getSendBufPtr(&cd->u.s.smq),
                      SharkMQ_getSendBufSize(&cd->u.s.smq));
#else
      /* Minnow Server is inactive when in IoT mode, thus we can use
       * it's receive buffer.
       */
      BufPrint_setBuf(&o->super, (char*)msBuf.rec, sizeof(msBuf.rec));
#endif
   }
#endif
   JErr_constructor(&o->err);
   JEncoder_constructor(&o->encoder, &o->err, &o->super);
   o->committed=FALSE;
}

/* Called when we are done creating a JSON message.
 */
static int
SendData_commit(SendData* o)
{
   if(o->committed) return -1;
   o->committed=TRUE;
   /* Trigger SendData_wsSendJSON() or SendData_smqSendJSON() */
   return JEncoder_commit(&o->encoder);
}



/****************************  Application ********************************/

/* All JSON messages start with the following: [messagename, ... */
static void
beginMessage(SendData* sd, const char* messagename)
{
   JEncoder_beginArray(&sd->encoder);
   JEncoder_setString(&sd->encoder, messagename);
}

/* All messages end with: ...] */
static int
endMessage(SendData* sd)
{
   JEncoder_endArray(&sd->encoder);
   return SendData_commit(sd);
}

/* For the LED example. Convert type to string */
static const char*
ledType2String(LedColor t)
{
   switch(t)
   {
      case LedColor_red: return "red";
      case LedColor_yellow: return "yellow";
      case LedColor_green: return "green";
      case LedColor_blue: return "blue";
   }
   baAssert(0);
   return "";
}



/* 
   ["ledinfo", {"leds" : [...]}]
   where ... is one or several objects of type:
      {"name":"ledname","id":number, "color":"cname"}
      See func. ledType2String for color names
 */
static int
sendLedInfo(ConnData* cd)
{
   int i, ledLen;
   SendData sd;
   const LedInfo* ledInfo = getLedInfo(&ledLen);
   SendData_constructor(&sd, cd);
   beginMessage(&sd, "ledinfo");
   JEncoder_beginObject(&sd.encoder);
   JEncoder_setName(&sd.encoder,"leds");
   JEncoder_beginArray(&sd.encoder);
   for(i = 0 ; i < ledLen ; i++)
   {
      JEncoder_set(&sd.encoder, "{dssb}",
                   JE_MEMBER(ledInfo+i, id),
                   "color", ledType2String(ledInfo[i].color),
                   JE_MEMBER(ledInfo+i, name),
                   "on",(BaBool)getLedState(ledInfo[i].id));
   }
   JEncoder_endArray(&sd.encoder);
   JEncoder_endObject(&sd.encoder);
   return endMessage(&sd);
}


/* 
   ["setled", {"id": number, "on": boolean}]
 */
static int
sendSetLED(ConnData* cd, int ledId, int on)
{
   SendData sd;
   SendData_constructor(&sd, cd);
   beginMessage(&sd, "setled");
   JEncoder_set(&sd.encoder, "{db}",
                "id", ledId,
                "on", (BaBool)on);
   return endMessage(&sd);
}

/* 
   Manage 'setled' command received from browser.
 */
static int
manageSetLED(ConnData* cd, JErr* e, JVal* v)
{
   S32 ledID;
   BaBool on;
   JVal_get(v, e, "{db}", "id",&ledID, "on",&on);
   if(JErr_isError(e) || setLed(ledID, (int)on)) return -1;
   return sendSetLED(cd, ledID, (int)on);
}


/* 
   ["settemp", number]
 */
static int
sendSetTemp(ConnData* cd, int temp)
{
   SendData sd;
   SendData_constructor(&sd, cd);
   beginMessage(&sd, "settemp");
   JEncoder_setInt(&sd.encoder, temp);
   return endMessage(&sd);
}


/*
  ["DeviceName", ["the-name"]
 */
static int
sendDeviceName(ConnData* cd)
{
   SendData sd;
   SendData_constructor(&sd, cd);
   beginMessage(&sd, "devname");
   JEncoder_beginArray(&sd.encoder);
   JEncoder_setString(&sd.encoder,getDevName());
   JEncoder_endArray(&sd.encoder);
   return endMessage(&sd);
}

/*
  ["uploadack", number]
 */
static int
sendUploadAck(ConnData* cd, S32 messages)
{
   SendData sd;
   SendData_constructor(&sd, cd);
   beginMessage(&sd, "uploadack");
   JEncoder_setInt(&sd.encoder, messages);
   return endMessage(&sd);
}


/* The idle function, which simulates events in the system, is called
 * when not receiving data.
 */
static int
eventSimulator(ConnData* cd)
{
   int x; /* used for storing ledId and temperature */
   int on;
   static int temperature=0;
   if(setLedFromDevice(&x,&on) && /* If a local button was pressed */
      sendSetLED(cd, x, on) )
   {
      return -1;
   }
   x = getTemp();
   return x != temperature ? temperature=x,sendSetTemp(cd, x) : 0;
}

/****************************  Application: AJAX *****************************/

/* All AJAX messages begin with: ["AJAX",[RPC-ID, .... */
static void
beginAjaxResp(SendData* sd, S32 ajaxHandle)
{
   beginMessage(sd, "AJAX");
   JEncoder_beginArray(&sd->encoder);
   JEncoder_setInt(&sd->encoder, ajaxHandle);
}


/* All AJAX messages end with: ...]] */
static int
endAjaxResp(SendData* sd)
{
   JEncoder_endArray(&sd->encoder);
   return endMessage(sd);
}


/* Send ["AJAX",[RPC-ID, {"err": emsg}]]
 */
static int
sendAjaxErr(ConnData* cd, S32 ajaxHandle, const char* emsg)
{
   SendData sd;
   SendData_constructor(&sd, cd);
   beginAjaxResp(&sd, ajaxHandle);
   JEncoder_set(&sd.encoder,"{s}", "err", emsg);
   return endAjaxResp(&sd);
}



/* Manages the following AJAX requests:
   math/add
   math/subtract
   math/mul
   math/div
 */
static int
math_xx(ConnData* cd,const char* op, JErr* e,S32 ajaxHandle, JVal* v)
{
   double arg1, arg2, resp;
   SendData sd;
   JVal_get(v, e, "[ff]", &arg1, &arg2);
   if(JErr_isError(e))
      return sendAjaxErr(cd, ajaxHandle, "math/: invalid args");
   switch(op[0]) /* Use first letter of 'add', 'subtract', 'mul', and 'div' */
   {
      case 'a': resp = arg1 + arg2; break;
      case 's': resp = arg1 - arg2; break;
      case 'm': resp = arg1 * arg2; break;
      case 'd':
         if(arg2 == 0)
            return sendAjaxErr(cd, ajaxHandle, "Divide by zero!");
         resp = arg1 / arg2;
         break;
      default:
         return sendAjaxErr(cd, ajaxHandle, "Unknown math operation");
   }
   SendData_constructor(&sd, cd);
   beginAjaxResp(&sd, ajaxHandle);
   JEncoder_set(&sd.encoder,"{f}", "rsp", resp);
   return endAjaxResp(&sd);
}


static int
auth_setcredentials(ConnData* cd,JErr* e,S32 ajaxHandle, JVal* v)
{
   SendData sd;
   SharkSslSha1Ctx ctx;
   U8 digest[20];
   const char* curUname;
   const char* curPwd;
   const char* newUname;
   const char* newPwd;
   JVal_get(v, e, "[ssss]", &curUname, &curPwd,&newUname,&newPwd);
   if(JErr_isError(e))
      return sendAjaxErr(cd, ajaxHandle, "auth/setcredentials: invalid args");
   SharkSslSha1Ctx_constructor(&ctx);
   SharkSslSha1Ctx_append(&ctx, (U8*)curPwd, strlen(curPwd));
   SharkSslSha1Ctx_finish(&ctx, digest);
   SendData_constructor(&sd, cd);
   if(checkCredentials(curUname, 0, digest))
   {
      return sendAjaxErr(cd, ajaxHandle,
                         "Please provide correct current credentials!");
   }
   if(setcredentials(newUname, newPwd))
   {
      return sendAjaxErr(cd, ajaxHandle,
                         "Saving credentials failed!");
   }

   beginAjaxResp(&sd, ajaxHandle);
   JEncoder_set(&sd.encoder,"{b}", "rsp", TRUE);
   return endAjaxResp(&sd);
}


static int
ajax(ConnData* cd,JErr* e,JVal* v)
{
   const char* service; /* The AJAX service name */
   S32 ajaxHandle;
   JVal* vPayload;

   JVal_get(v, e, "[sdJ]", &service, &ajaxHandle, &vPayload);
   if(JErr_isError(e))
   {  /* This error would indicate a problem with the code in connection.js */
      xprintf(("AJAX semantic err: %s\n", e->msg));
      return -1;
   }
   if( ! strncmp("math/", service, 5) )
      return math_xx(cd, service+5, e, ajaxHandle, vPayload);
   if( ! strcmp("auth/setcredentials", service) )
      return auth_setcredentials(cd, e, ajaxHandle, vPayload);
   return sendAjaxErr(cd, ajaxHandle, "Service not found!");
}


/******************************  RecData ************************************/

/*
  The binary message types sent from the browser to the server.
  See the JavaScript equivalent in WebSocketCon.js
*/
typedef enum {
   BinMsg_Upload = 1,
   BinMsg_UploadEOF
} BinMsg;


/*

  This container object stores data objects used when receiving JSON
  messages and sending response data.
  JParser:
    https://realtimelogic.com/ba/doc/en/C/reference/html/structJParser.html
 */
typedef struct {
   JParser parser; /* JSON parser */
   JParserValFact pv; /* JSON Parser Value (JVal) Factory */
   S32 messages; /* Used for counting messages received from browser */
   BaBool authenticated; /* Set to true if the user is authenticated */
   char maxMembN[12]; /* Buffer for holding temporary JSON member name */
   /* nonce: One time key created and sent to the browser as part of
      logic for preventing relay attacks */
   U8 nonce[12];
   U8 binMsg; /* Holds the binary message type 'BinMsg' (enum BinMsg) */
} RecData;


/*
  Construct the RecData object used when receiving JSON messages and
  sending response data.

  Notice how we use the static allocators if USE_STATIC_ALLOC is
  set. See JsonStaticAlloc.c for details.

  https://realtimelogic.com/ba/doc/en/C/reference/html/structJParserValFact.html
  https://realtimelogic.com/ba/doc/en/C/reference/html/structJParser.html
 */
static void
RecData_constructor(RecData* o)
{
   memset(o, 0, sizeof(RecData));
#ifdef USE_STATIC_ALLOC
   {
      JParserAlloc_constructor(&jpa);
      VAlloc_constructor(&va);
      DAlloc_constructor(&da);
      JParserValFact_constructor(&o->pv, &va, &da);
      JParser_constructor(&o->parser, (JParserIntf*)&o->pv, o->maxMembN,
                          sizeof(o->maxMembN), &jpa,0);
   }
#else
   /* Use dynamic allocation */
   JParserValFact_constructor(&o->pv, AllocatorIntf_getDefault(),
                              AllocatorIntf_getDefault());
   JParser_constructor(&o->parser, (JParserIntf*)&o->pv, o->maxMembN,
                       sizeof(o->maxMembN), AllocatorIntf_getDefault(),0);
#endif
}

#if 0
/* Not used since we keep RecData as long as program runs */
static void
RecData_destructor(RecData* o)
{
   JParser_destructor(&o->parser);
   JParserValFact_destructor(&o->pv);
}
#endif


/* Release memory used by JParserValFact (Reset memory buffers if
 * USE_STATIC_ALLOC is defined).
 */
#define RecData_reset(o) JParserValFact_termFirstVal(&(o)->pv);


/* Send the one time nonce key to the browser.
  ["nonce", "the-12-byte-nonce-encoded-as-b64"]
 */
static int
RecData_sendNonce(RecData* o, ConnData* cd)
{
   SendData sd;
#ifdef MS_SEC
   sharkssl_rng(o->nonce, sizeof(o->nonce));
#else
   {
      int i;
      for(i=0; i < sizeof(o->nonce) ; i++)
         o->nonce[i] = (U8)rand()%0xFF;
   }
#endif
   SendData_constructor(&sd, cd);
   beginMessage(&sd, "nonce");
   /* Send the 12 byte binary nonce B64 encoded */
   JEncoder_b64enc(&sd.encoder, o->nonce, sizeof(o->nonce));
   return endMessage(&sd);
}


/* We received an 'auth' message from the browser.
   Complete message: ["auth", {name:"string",hash:"string"}]
   Value in argument 'v' is: {name:"string",hash:"string"}
*/
static int
RecData_authenticate(RecData* o, ConnData* cd, JVal* v, JErr* e)
{
   const char* name; /* username */
   const char* hash; /* 40 byte SHA-1 password hash in hex representation */
   int i;
   U8 digest[20]; /* Convert and store hash in this buffer */
   JVal_get(v, e, "{ss}", "name",&name, "hash",&hash);
   if(JErr_isError(e)) return -1;

   /* A SHA-1 digest is 20 bytes. Convert the 40 byte hex value
    * received from the browser to a 20 byte binary representation.
    */
   for(i=0 ; i < 20 ; i++)
   {
      int j;
      U8 hex=0;
      const U8* ptr=(U8*)hash+2*i;
      for(j = 0 ; j<2 ; j++)
      { /* Convert each hex value to a byte value. Assume hex is all
         * lower case letters.
         */
         U8 c = *ptr++;
         if(c>='0' && c<='9') c -= '0' ; /* 0..9 */
         else c = c-'a'+10 ; /* 10..15 */
         hex = (hex << 4) | c ;
      }
      digest[i]=hex;
   }
   /* Check if digest matches locally stored data */
   if( ! checkCredentials(name, o->nonce, digest) )
   {
      o->authenticated = TRUE;
      /*
        Send the initial LED info message if user provided correct
        credentials. We also send the temperature so the thermostat
        shows the correct temperature.
      */
      return sendLedInfo(cd) || sendSetTemp(cd, getTemp()) ? -1: 0;
   }
   /* Not authenticated */
   return RecData_sendNonce(o, cd);
}

/* Manage binary SMQ and WebSocket frames sent by browser.
   Note that a complete frame may be longer than the data we
   receive. The argument eom (end of message) is true when all data in
   the frame has been consumed. We implement rudimentary state
   management for a frame that is split up into multiple chunks by
   storing the binary message type in RecData:binMsg
*/
static int
RecData_manageBinFrame(RecData* o,ConnData* cd, U8* data, int len,BaBool eom)
{
   int status=-1;
   if(o->binMsg == 0) /* Not set if first chunk in a frame */
   {
      o->binMsg=data[0]; /* Save binary message type (first byte in frame) */
      data++; /* Set pointer to start of payload */
      len--;
      if(o->messages==0)
      {
         if(saveFirmware(0, 0, TRUE, FALSE))
            return -1;
      }
   }
   switch(o->binMsg)
   {
      case BinMsg_Upload:
         status=saveFirmware(data, len, FALSE, FALSE);
         if(!status && eom && ++o->messages % 10 == 0)
            status=sendUploadAck(cd, o->messages);
         break;
      case BinMsg_UploadEOF:
         ++o->messages;
         status=saveFirmware(data, len, FALSE, eom);
         if(!status)
            status=sendUploadAck(cd, o->messages);
         o->messages=0;
         break;
      default:
         xprintf(("Received unknown binary message: %u\n",
                  (unsigned)data[0]));
   }
   if(eom)
      o->binMsg = 0; /* Reset */
   if(status)
      saveFirmware(0,0,FALSE,TRUE);
   return status;
}



/*
  Manages JSON async messages sent by browser, except for 'auth',
  which is handled in RecData_openMessage. The user is authenticated
  if this function is called.
*/
static int
RecData_manageMessage(RecData* o,ConnData* cd,const char* msg,JErr* e,JVal* v)
{
   switch(*msg)
   {
      case 'A': /* AJAX is encapsulated as an async message */
         if(!strcmp("AJAX",msg))
            return ajax(cd,e,v);
         goto L_unknown;

      case 's':
         if(!strcmp("setled",msg))
            return manageSetLED(cd,e,v);
         goto L_unknown;

      default:
      L_unknown:
         xprintf(("Received unknown message: %s\n", msg));
   }
   return -1;
}


/* All messages have the form ["message-name", 'message-body']
   This function extracts the message name and deals with the initial
   state when we are not authenticated.
*/
static int
RecData_openMessage(RecData* o, ConnData* cd)
{
   JErr e;
   JVal* v;
   const char* message; /* The JSON message name */
   JErr_constructor(&e);
    /*  A.1: Get JSON message root obj
        A.2: Get the first element in the array, which should be the
             JSON message name
        B.1: Extract the JSON message name
        B.2: Advance to next element, which should be the message body
    */
   v=JVal_getArray(JParserValFact_getFirstVal(&o->pv),&e);
   if((message=JVal_getString(v,&e)) != 0 && (v=JVal_getNextElem(v)) != 0)
   {
      /* Message OK so far, now let's look at the message content */
      if( ! o->authenticated )
      { /* The only message accepted when not authenticated is 'auth' */
         if( strcmp(message, "auth") || RecData_authenticate(o,cd,v,&e))
            goto L_semantic;
         /* Else msg OK. */
      }
      else
      {
         if(RecData_manageMessage(o, cd, message, &e, v))
            return -1;
      }
      RecData_reset(o);
      return 0;
   }
   else
   {
     L_semantic:
      xprintf(("Semantic JSON message error\n"));
   }
   return -1;
}


/*
  Parse the JSON 'data' with 'len' bytes received either from a
  WebSocket connection or SMQ.  A very large message may be in a
  WebSocket frame that is too large for the Minnow Server receive
  buffer (or SMQ buffer). The Minnow server (and SMQ) include state
  information telling the application about these conditions. The
  argument 'eom' (end of message) is false as long as we receive
  chunks that is not the last chunk. We do not use large JSON messages
  thus eom should always be TRUE, however, this code is designed to
  manage receiving chunks. Note that the code uses the parse state and
  'eom' for integrity validation. 
*/
static int
RecData_parse(RecData* o,ConnData* cd,U8* data,int len,BaBool eom)
{
   int status;

#ifdef USE_STATIC_ALLOC
   /* For each new JSON message received, reset the internal static buffer
    * index pointer for the basic allocators.
    */
   JParserAlloc_reset(&jpa);
   VAlloc_reset(&va);
   DAlloc_reset(&da);
#endif
   status = JParser_parse(&o->parser, data, len);
   if(status)
   {
      if(status < 0 || ! eom)
      {
         xprintf(("JSON: %s\n",status<0 ? "parse error":"expected end of MSG"));
         return -1;
      }
      /* Got a JSON message */
      return RecData_openMessage(o,cd);
   }
   else if(eom)
   {
      xprintf(("Expected more JSON"));
   }
   return 0; /* OK, but need more data */
}


/*
  This function gets called when the Minnow Server's listen socket is
  activated i.e. when socket 'accept' returns a new socket.
  https://realtimelogic.com/ba/doc/en/C/shark/structMS.html
*/
static void
RecData_runServer(RecData* rd, ConnData* cd, WssProtocolHandshake* wph)
{
   MS* ms=cd->u.ms;
   /* MS_webServer: Manage HTTP GET or upgrade WebSocket request */
   if( ! MS_webServer(ms,wph) && ! sendDeviceName(cd))
   { /* We get here if HTTP(S) was upgraded to a WebSocket con. */
      int rc;
      U8* msg;
      /* We send the nonce to the browser so the user can
       * safely authenticate.
       */
      RecData_sendNonce(rd, cd);
      while((rc=MS_read(ms,&msg,50)) >= 0)
      {
         if(rc) /* incomming data from browser */
         {
            if(ms->rs.frameHeader[0] == WSOP_Text)
            {  /* All text frames should contain JSON */
               if(RecData_parse(
                     rd,cd,msg,rc,ms->rs.frameLen-ms->rs.bytesRead==0))
               {
                  break; /* err */
               }
            }
            else /* Manage binary WebSocket frames */
            {
               if(RecData_manageBinFrame(
                     rd,cd,msg,rc,ms->rs.frameLen-ms->rs.bytesRead==0))
               {
                  break; /* err */
               }
            }
         }
         else /* timeout (Ref-D) */
         {
            if(rd->authenticated && eventSimulator(cd))
               break; /* on sock error */
         }
      }
      rd->authenticated=FALSE;
      xprintf(("Closing WS connection: ecode = %d\n",rc));
   }
}


/* Called when in IoT mode and after an IoT connection has been established */
#ifdef USE_SMQ
static int
RecData_runSMQ(RecData* rd, ConnData* cd)
{
   U8* msg;
   int len;
   for(;;) /* Run as long as we have data */
   {
      if( (len = SharkMQ_getMessage(&cd->u.s.smq, &msg)) == 0)
      {  /* No data */
         return rd->authenticated ? eventSimulator(cd) : 0;
      }
      if(len < 0)
      {
         xprintf(("SMQ closed, status: %d\n", cd->u.s.smq.status));
         return -1;
      }
      /* browserTID is initially set to 1 (Ref-etid). Set browserTID
         to the address of the sender of this message.
       */ 
      if(cd->u.s.browserTID == 1)
         cd->u.s.browserTID = cd->u.s.smq.ptid;
      else if(cd->u.s.browserTID != cd->u.s.smq.ptid)
      {
         xprintf(("Received data from unknown sender: %u\n",
                  cd->u.s.smq.ptid));
      }
      if(cd->u.s.smq.subtid == 1) /* Text message i.e. JSON */
      {
         if(RecData_parse(
               rd,cd,msg,len,cd->u.s.smq.frameLen-cd->u.s.smq.bytesRead==0))
         {
            return -1;
         }

      }
      else if(cd->u.s.smq.subtid == 2) /* Binary message */
      {
         if(RecData_manageBinFrame(
               rd,cd,msg,len,cd->u.s.smq.frameLen-cd->u.s.smq.bytesRead==0))
         {
            return -1;
         }

      }
      else
      {
         xprintf(("Received unknown SMQ subtid %d\n", cd->u.s.smq.subtid));
         return -1;
      }
   }
}


/* Called if Minnow Server has been idle for 3 seconds */
static int
RecData_connectSMQ(RecData* rd, ConnData* cd, SharkSsl* sharkSSL)
{
   static const char* smqUniqueId=0;
   static int smqUniqueIdLen;
   SharkSslCon* scon;
#ifdef MS_SEC
   static U8 smqBuf[127];
#else
   (void)sharkSSL;
#endif
   if(!smqUniqueId) /* Change this code to use MAC address from device */
   {
      /* See led-host-sim.ch */
      smqUniqueIdLen = getUniqueId(&smqUniqueId);
      if(smqUniqueIdLen < 1)
      {
         xprintf(("Cannot get unique ID.\n"));
         /* Fetching MAC address in a device should never fail */
         baAssert(0);
         return -2;
      }
   } /* End change code */
   cd->isWS=FALSE; /* Switch to SMQ mode */
   scon = SharkSsl_createCon(sharkSSL);
   if(!scon) return -1; /* Should probably do system reboot */

#ifdef MS_SEC
    /* Small buffer needed for housekeeping. See docs:
       https://realtimelogic.com/ba/doc/en/C/shark/group__SMQLib.html
    */
   SharkMQ_constructor(&cd->u.s.smq, smqBuf, sizeof(smqBuf));
#else
   /* 
      Minnow Server is inactive, thus we can use it's buffer.
      https://realtimelogic.com/ba/doc/en/C/reference/html/structSMQ.html
   */
   SMQ_constructor(&cd->u.s.smq, msBuf.send, sizeof(msBuf.send));
#endif
   xprintf(("SMQ: connecting to %s\n", SMQ_PROTOCOL SMQ_DOMAIN));
   if(SharkMQ_init(&cd->u.s.smq, scon, SMQ_URL, 0) < 0)
   {
      xprintf(("Cannot connect SMQ, status: %d\n",cd->u.s.smq.status));
      return -1;
   }
#ifdef MS_SEC
   if(cd->u.s.smq.status != SharkSslConTrust_CertCn)
   { /* This should be an error in a production environment */
      xprintf(("WARNING: certificate received from %s not trusted!\n",
               SMQ_DOMAIN));
   }
#endif
   if(SharkMQ_connect(&cd->u.s.smq,
                      smqUniqueId, smqUniqueIdLen,
                      0, 0, /* credentials */
                      getDevName(), strlen(getDevName()),1420))
   {
      xprintf(("SMQ connect failed, status: %d\n", cd->u.s.smq.status));
      /* Assert: program err, more mem needed. */
      baAssert(cd->u.s.smq.status != SMQE_BUF_OVERFLOW);
      return -1;
   }

   /* Set browserTID to the TID of SMQ client in server so all initial
    * messages are sent to the server SMQ client, where they are
    * stored util a browser connects (Ref-etid).
    */
   cd->u.s.browserTID = 1;
   if(sendDeviceName(cd) || RecData_sendNonce(rd, cd))
      return -1;
   cd->u.s.smq.timeout = 1; /* poll mode */
   return 0;
}


/* Revert to the default state "WebSocket server" if the IoT connection
 * goes down or if a local Intranet user initiates a new HTTP or
 * WebSocket connection.
 */
static void
revert2WsCon(SharkSsl* sharkSsl, RecData* rd, ConnData* cd, MS* ms)
{
#ifndef MS_SEC
   (void)sharkSsl;
#endif
   SharkMQ_disconnect(&cd->u.s.smq);
   SharkSsl_terminateCon(sharkSsl, cd->u.s.smq.scon);
   SharkMQ_destructor(&cd->u.s.smq);
   ConnData_setWS(cd, ms); /* Set default setup */
   rd->authenticated=FALSE;
}

#else /* if SMQ */
#define revert2WsCon(sharkSsl, rd, cd, ms) /* Do nothing */
#endif


/* Attempt to open the default server listening port 80 (secure mode
 * 443) or use an alternative port number if the default port is in
 * use.
 */
static int
openServerSock(SOCKET* sock)
{
   int status;
   U16 port;
#ifdef MS_SEC
   port=443;
#else
   port=80;
#endif
   status = se_bind(sock, port);
   if(status)
   {
      port=9442;
      while(status == -3 && ++port < 9460)
         status = se_bind(sock, port);
   }
   if(!status)
   {
      xprintf(("WebSocket server listening on %d\n", (int)port));
   }
   return status;
}


/*
  The main function initiates everything and opens a socket
  connection. When in secure mode (TLS), create a SharkSsl object and
  one SharkSslCon (connection) object.

  ctx: Bare Metal only (remove for other platforms), see:
  https://realtimelogic.com/ba/doc/en/C/shark/group__BareMetal.html
 */
void
mainTask(SeCtx* ctx)
{
#ifdef MS_SEC
   static SharkSsl sharkSslServer; /* For the Minnow Server when in TLS mode */
#endif
#ifdef USE_SMQ
   /* We need a SharkSSL client object for the IoT connection. Note
    * that a placeholder object is created when in non secure mode
    * since the functions in this example code take this object as
    * argument, even when not used.
    */
   static SharkSsl sharkSslClient;
#endif
   static WssProtocolHandshake wph={0};
   static ConnData cd;
   static RecData rd;
   static MS ms; /* The Minnow Server */
#ifdef USE_SMQ
   static int timeoutCounter=0;
#endif
   static SOCKET listenSock;
   static SOCKET sock;
   SOCKET* listenSockPtr = &listenSock;
   SOCKET* sockPtr = &sock;

   (void)ctx; /* Not used */

   RecData_constructor(&rd);
   ConnData_setWS(&cd, &ms); /* Set default setup */
   MS_constructor(&ms);

   SOCKET_constructor(listenSockPtr, ctx);
   SOCKET_constructor(sockPtr, ctx);

   wph.fetchPage = fetchPage;
   if(openServerSock(listenSockPtr))
   {
      return;
   }

#ifdef MS_SEC
#ifdef USE_SMQ
   SharkSsl_constructor(&sharkSslClient,
                        SharkSsl_Client,
                        0,   /* SSL cache not used by client */
                        2000,  /* inBuf size */
                        3000); /*outBuf size is fixed and must fit server cert*/
#endif
   SharkSsl_constructor(&sharkSslServer,
                        SharkSsl_Server,
                        1,   /* SSL cache size */
                        2000,  /* inBuf size */
                        3000); /*outBuf size is fixed and must fit server cert*/

   /* At least one server certificate is required */
   SharkSsl_addCertificate(&sharkSslServer, sharkSSL_RTL_device);

   /* It is very important to seed the SharkSSL RNG generator */
   sharkssl_entropy(baGetUnixTime() ^ (ptrdiff_t)&sharkSslServer);
#endif

   for(;;)
   {
      switch(se_accept(&listenSockPtr, 50, &sockPtr))
      {
         case 1: /* Accepted new client connection i.e. new browser conn. */
         {
            /* If SMQ connection active: terminate immediately. */
            if( ! ConnData_WebSocketMode(&cd) )
               revert2WsCon(&sharkSslClient,&rd,&cd,&ms);
#ifdef MS_SEC
            {
               SharkSslCon* scon = SharkSsl_createCon(&sharkSslServer);
               if(!scon) return; /* Must do system reboot */
               /* Keep seeding (make it more secure) */
               sharkssl_entropy(baGetUnixTime() ^ (U32)scon);
               MS_setSharkCon(&ms, scon, sockPtr);
               RecData_runServer(&rd, &cd, &wph);
               SharkSsl_terminateCon(&sharkSslServer, scon);
            }
#else
            MS_setSocket(&ms,sockPtr,msBuf.rec,sizeof(msBuf.rec),
                         msBuf.send,sizeof(msBuf.send));
            RecData_runServer(&rd, &cd, &wph);
#endif
            se_close(sockPtr);
#ifdef USE_SMQ
            timeoutCounter=0;
#endif
         }
         break;

         case 0: /* se_accept 50ms timeout */
#ifdef USE_SMQ
            /* Activate and run IoT connection if Minnow Server has been
             * inactive for 3 seconds.
             */
            if(++timeoutCounter > 60)
            {
               if( (ConnData_WebSocketMode(&cd) &&
                    RecData_connectSMQ(&rd,&cd,&sharkSslClient)) ||
                   RecData_runSMQ(&rd, &cd))
               { /* If we cannot connect or SMQ connection goes down */
                  revert2WsCon(&sharkSslClient,&rd,&cd,&ms);
                  timeoutCounter=0;
               }
            }
#endif
            break;

         default:
            /* We get here if 'accept' fails.
               This is probably where you reboot.
            */
            se_close(listenSockPtr);
            return; /* Must do system reboot */
      }
   }
}
