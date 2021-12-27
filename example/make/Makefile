
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

# Enable IoT mode and connect to the test server minnow.realtimelogic.com
# See documention for details. 
ifdef IOT
CFLAGS += -DUSE_SMQ '-DSMQ_DOMAIN="minnow.realtimelogic.com"'
endif

#Prints info in console
CFLAGS += -DXPRINTF

ifneq ($(wildcard ../../../SharkSSL/.*),)
USE_SHARKSSL=1
endif


ifndef PLAT
PLAT=Posix
ifdef USE_SHARKSSL
CFLAGS+=$(IFT)../../../SharkSSL/inc/arch/Posix $(IFT)../../../SharkSSL/src/arch/Posix
else
CFLAGS+=$(IFT)../../../SMQ/src/arch/Posix
endif
EXTRALIBS += -lrt
endif

ifdef USE_SHARKSSL
CFLAGS += $(IFT)../../../SharkSSL/inc $(IFT)../../../SharkSSL/examples
else
CFLAGS += $(IFT)../../../SMQ/src $(IFT)../../../SMQ/examples
endif
CFLAGS += $(IFT)../../src $(IFT)../../../JSON/inc


ifndef ODIR
ODIR = obj
endif

VPATH=../../../JSON/src:../../src:../src
ifdef USE_SHARKSSL
VPATH+=../../../SharkSSL/src
else
VPATH+=../../../SMQ/src
endif

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
	MSLib.c \
	index.c \
	JsonStaticAlloc.c \
	MinnowRefPlatMain.c

ifdef USE_SHARKSSL
SOURCE += SharkSSL.c SharkMQ.c
else
SOURCE += SMQClient.c
endif


OBJ := $(SOURCE:%.c=$(ODIR)/%$(O))

.PHONY: packwwwifchanged packwww clean help

help:
	@echo "make minnow  -> Build the Minnow Server reference example"
	@echo "make packwww -> Pack the www directory and replace ../src/index.c"
	@echo "make clean   -> Remove object files and executable"
	@echo "Build with debug information: make minnow build=debug"


minnow: $(ODIR) $(OBJ)
	$(CC) $(LNKOFT)$@ $(OBJ)

packwwwifchanged: ../src/index.c

# minnow above depends on ../src/index.c. This section auto builds
# ../src/index.c if any www files are updated.
WWWDIRS := $(shell find ../../www -type d)
WWWFILES = $(foreach dir,$(WWWDIRS),$(wildcard $(dir)/*))
../src/index.c: $(WWWFILES)
	$(MAKE) packwww

packwww: $(ODIR)
	@echo "Zipping the 'www' directory...."
	cd ../..&&zip -r example/make/obj/www.zip www
	@echo "Uploading the zipped 'www' directory to the HTML amalgamator..."
	cd obj&&curl --progress-bar -F download=true -F jsc=SIMPLE -F zip=true -F upload=@www.zip https://realtimelogic.info/htmla/ --output index.c 2>&1
	@echo "Replacing ../src/index.c with the new amalgamated file"
	mv obj/index.c ../src/

$(ODIR):
	mkdir $(ODIR)

clean:
	rm -rf minnow obj
