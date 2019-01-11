
/* Include this file in your build if main.c is compiled with
   'USE_STATIC_ALLOC' defined. See the introductory comment to using
   these basic allocators in main.c.

   The three allocators in this file implement the following interface:
   https://realtimelogic.com/ba/doc/en/C/reference/html/structAllocatorIntf.html

   Code implementing the 'AllocatorIntf' should normally follow the "C
   object oriented" design as outlined in the following tutorial:
   https://realtimelogic.com/ba/doc/?url=C/introduction.html#oo_c
   However, we have simplified the following code since we do not need
   any objects, except for the data contained in an instance of
   AllocatorIntf. All functions below take AllocatorIntf, which should
   normally be the super class, as the first argument.

   Why three allocators?

   The reason is simplicity based on an understanding of what the JSON
   parser (JParser) and the JVal node factory (JParserValFact)
   require.

   JParser allocates memory for a temporary string as it parses JSON
   with string elements. The parser only allocates one buffer, but
   this buffer may have a need to grow, thus it calls realloc if the
   buffer is too small.

   Doc: https://realtimelogic.com/ba/doc/en/C/reference/html/structJParser.html

   JParserValFact takes two allocators as arguments. The code is
   designed this way to optimize memory use when using static
   allocators. One allocator allocates memory objects with sizeof(JVal)
   and the other allocator allocates memory for strings. The reason
   for using two allocators is that JVal must be memory aligned, but
   strings do not need to be memory aligned. By using two allocators,
   we can optimize the use of the memory.

   Doc:
   https://realtimelogic.com/ba/doc/en/C/reference/html/structJParserValFact.html
*/

#include "JsonStaticAlloc.h"
#include <JDecoder.h>
#include <selib.h>

/*
  We do not need to implement 'free' since we simply reset the
  allocator's buffer index pointer when we start parsing a new JSON
  message.

 */
static void
doNothingOnFree(AllocatorIntf* super, void* memblock)
{
   (void)super;
   (void)memblock;
}

/****************************************************************************
 **************************| JParser Allocator |*****************************
 ****************************************************************************/

/* No strings can be longer than this size. Try to keep JSON messages
 * including strings as short as possible, including the object member
 * names. The minimum size allocated by JParser is 256 bytes.
 */
#define MAX_STRING_LEN 256

/*
  Called when the one and only string buffer must grow.
*/
static void*
JParserAlloc_realloc(AllocatorIntf* o, void* memblock, size_t* size)
{
   static U8 buf[MAX_STRING_LEN];
   (void)o;
   baAssert(memblock == 0 || memblock == buf);
   if(*size <= MAX_STRING_LEN)
      return buf;
   xprintf(("MAX_STRING_LEN too small\n"));
   return 0;
}

static void*
JParserAlloc_malloc(AllocatorIntf* o, size_t* size)
{
   return JParserAlloc_realloc(o, 0, size);
}

void
JParserAlloc_constructor(AllocatorIntf* o)
{
   AllocatorIntf_constructor(
      o, JParserAlloc_malloc,JParserAlloc_realloc,doNothingOnFree);
}



/****************************************************************************
 *************************| JVal Node Allocator |****************************
 ****************************************************************************
 The allocator used by JParserValFact when creating JVal nodes.
*/

/* Maximum number of JVal nodes. Small microcontrollers should try to
   keep JSON messages small.
*/
#define MAX_JVAL_NODES 15
static U32 vAlloxIx;

static void*
vAlloc_malloc(AllocatorIntf* o, size_t* size)
{
   static JVal buf[MAX_JVAL_NODES];
   (void)o;
   baAssert(*size == sizeof(JVal));
   if(vAlloxIx < MAX_JVAL_NODES)
      return buf+(vAlloxIx++);
   xprintf(("MAX_JVAL_NODES too small\n"));
   return 0;
}


/* This simplified constructor is also used for resetting the buffer index
   pointer for each JSON message received -- i.e. :
   #define VAlloc_reset VAlloc_constructor
 */
void
VAlloc_constructor(AllocatorIntf* o)
{
   vAlloxIx=0;
   /* JParserValFact does not use realloc */
   AllocatorIntf_constructor(o,vAlloc_malloc,0,doNothingOnFree);
}


/****************************************************************************
 ************************| JVal string Allocator |***************************
 ****************************************************************************
 The allocator used by JParserValFact when creating JVal nodes with
 strings, including object member names.
*/

/* The maximum length of all strings combined. Small microcontrollers
   should avoid using too many strings and object member names. You
   will use less memory if you send JSON arrays instead of JSON
   objects since objects include member names.
 */
#define MAX_JSON_STRINGS_COMBINED 512
static U32 dAlloxIx;

static void*
DAlloc_malloc(AllocatorIntf* o, size_t* size)
{
   static char buf[MAX_JSON_STRINGS_COMBINED];
   (void)o;
   if((dAlloxIx + *size) < MAX_JSON_STRINGS_COMBINED)
   {
      char* mem = buf+dAlloxIx;
      dAlloxIx += *size;
      return mem;
   }
   xprintf(("MAX_JSON_STRINGS_COMBINED too small\n"));
   return 0;
}

/* This simplified constructor is also used for resetting the buffer index
   pointer for each JSON message received  -- i.e. :
   #define DAlloc_reset DAlloc_constructor
 */
void
DAlloc_constructor(AllocatorIntf* o)
{
   dAlloxIx=0;
   /* JParserValFact does not use realloc */
   AllocatorIntf_constructor(o,DAlloc_malloc,0,doNothingOnFree);
}
