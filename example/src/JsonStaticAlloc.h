#include <AllocatorIntf.h>

void JParserAlloc_constructor(AllocatorIntf* o);
void VAlloc_constructor(AllocatorIntf* o);
void DAlloc_constructor(AllocatorIntf* o);

#define JParserAlloc_reset JParserAlloc_constructor
#define VAlloc_reset VAlloc_constructor
#define DAlloc_reset DAlloc_constructor
