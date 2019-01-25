
PROGRAM=minnow
EXTRA_COMPONENTS = extras/SharkSSL extras/SharkSSL-ESP extras/JSON

PROGRAM_EXTRA_SRC_FILES = \
	MinnowServer/example/src/index.c \
	MinnowServer/example/src/JsonStaticAlloc.c \
	MinnowServer/example/src/main.c

PROGRAM_CFLAGS += -DXPRINTF=1 -DB_LITTLE_ENDIAN
PROGRAM_CFLAGS += -Wall

include ../../common.mk
