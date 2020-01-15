
PROGRAM=minnow
EXTRA_COMPONENTS = extras/SharkSSL extras/SharkSSL-ESP extras/JSON

PROGRAM_EXTRA_SRC_FILES = \
	MinnowServer/example/src/index.c \
	MinnowServer/example/src/JsonStaticAlloc.c \
	MinnowServer/example/src/main.c

PROGRAM_CFLAGS += -DXPRINTF=1 -DB_LITTLE_ENDIAN
PROGRAM_CFLAGS += -Wall

#Enable IoT and connect to minnow.realtimelogic.com
#PROGRAM_CFLAGS += -DUSE_SMQ '-DSMQ_DOMAIN="minnow.realtimelogic.com"'


include ../../common.mk
