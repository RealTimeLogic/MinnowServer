
#General macros
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

ifndef XCOMPILE
export CC := gcc
endif

export O := .o
export IFT := -I
export OFT := -o$(SPACE)
export LNKOFT := -o$(SPACE)
export LIBPFX := lib
export LIBEXT := .a
CFLAGS+=$(XCFLAGS)
CFLAGS+=-DB_LITTLE_ENDIAN
CFLAGS+=-Wall -c
ifeq (debug,$(build))
CFLAGS += -g
else
CFLAGS += -Os -O3
endif

#IoT enable the example. See documention for details.
#CFLAGS += -DUSE_SMQ

#Prints info in console
CFLAGS += -DXPRINTF

ifndef PLAT
PLAT=Posix
CFLAGS+=$(IFT)../../../SMQ/src/arch/Posix
EXTRALIBS += -lrt
endif

CFLAGS += $(IFT)../../../JSON/inc \
	$(IFT)../../../SMQ/src \
	$(IFT)../../../SMQ/examples \
	$(IFT)../../src

ifndef ODIR
ODIR = obj
endif

VPATH=../../../JSON/src:../../../SMQ/src:../../src:../src

# Implicit rules for making .o files from .c files
$(ODIR)/%$(O) : %.c
	$(CC) $(CFLAGS) $(OFT)$@ $<

SOURCE = AllocatorIntf.c \
	BaAtoi.c \
	BufPrint.c \
	JEncoder.c \
	JParser.c \
	JVal.c \
	selib.c \
	SMQClient.c \
	MSLib.c \
	index.c \
	JsonStaticAlloc.c \
	main.c

OBJ := $(SOURCE:%.c=$(ODIR)/%$(O))


minnow: $(ODIR) $(OBJ)
	$(CC) $(LNKOFT)$@ $(OBJ)

$(ODIR):
	mkdir $(ODIR)

clean:
	rm -rf minnow obj
