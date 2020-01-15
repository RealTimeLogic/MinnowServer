
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

.PHONY: packwww clean help

help:
	@echo "make minnow  -> Build the Minnow Server reference example"
	@echo "make packwww -> Pack the www directory and replace ../src/index.c"
	@echo "make clean   -> Remove object files and executable"
	@echo "Build with debug information: make minnow build=debug"


minnow: $(ODIR) $(OBJ)
	$(CC) $(LNKOFT)$@ $(OBJ)

packwww: $(ODIR)
	@echo "Zipping the 'www' directory...."
	cd ../..&&zip -r example/make/obj/www.zip www
	@echo "Uploading the zipped 'www' directory to the HTML amalgamator..."
	cd obj&&curl -F download=true -F jsc=SIMPLE -F zip=true -F upload=@www.zip https://realtimelogic.info/htmla/ --output index.c
	@echo "Replacing ../src/index.c with the new amalgamated file"
	mv obj/index.c ../src/

$(ODIR):
	mkdir $(ODIR)

clean:
	rm -rf minnow obj
