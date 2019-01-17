
PROGRAM=minnow
EXTRA_COMPONENTS = extras/SharkSSL-ESP
PROGRAM_EXTRA_SRC_FILES = \
	./JSON/src/AllocatorIntf.c \
	./JSON/src/BaAtoi.c \
	./JSON/src/BufPrint.c \
	./JSON/src/JEncoder.c \
	./JSON/src/JParser.c \
	./JSON/src/JVal.c \
	MinnowServer/src/MSLib.c \
	MinnowServer/example/src/index.c \
	MinnowServer/example/src/JsonStaticAlloc.c \
	MinnowServer/example/src/main.c

PROGRAM_CFLAGS += -DXPRINTF=1 -DB_LITTLE_ENDIAN
PROGRAM_CFLAGS += -Wall

include ../../common.mk

INC_DIRS += JSON/inc
