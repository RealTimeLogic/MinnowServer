
#include <MSLib.h>


int setcredentials(const char* username, const char* password)
{
   return -1; /* Not implemented */
}


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


/* Hard coded credentials only
 */
int checkCredentials(const char* name, U8 nonce[12], const U8 hash[20])
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


int
saveFirmware(U8* data, int len, BaBool open, BaBool eof)
{
   xprintf(("Uploading %s\n", eof ? "EOF" : ""));
   return 0; /* Not implemented, but it's OK returning 'success' */
}


static int currentTemperature=0; /* simulated value */
static int tempInc = 80; // 8 degrees increment
static int pollcnt;

int getTemp(void)
{
   if(++pollcnt >= 20) /* 50ms polling, thus trigger every second */
   {
      pollcnt = 0;
      currentTemperature += tempInc;
      if(currentTemperature >= 1000)
      {
         currentTemperature=1000;
         tempInc = -80;
      }
      else if(currentTemperature <= 0)
      {
         currentTemperature=0;
         tempInc = 80;
      }
   }
   return currentTemperature;
}
