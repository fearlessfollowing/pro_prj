# Makefile for adbd

SRCS := adb.c
SRCS += adb_client.c
SRCS += commandline.c
SRCS += adb_auth_host.c
SRCS += console.c
SRCS += file_sync_client.c
SRCS += fdevent.c
SRCS += get_my_path_linux.c
SRCS += services.c
SRCS += sockets.c
SRCS += transport.c
SRCS += transport_local.c
SRCS += transport_usb.c
SRCS += usb_linux.c
SRCS += usb_vendors.c
SRCS += utils.c
SRCS += list.c
SRCS += load_file.c

VPATH += ./libcutils
SRCS += abort_socket.c
SRCS += socket_inaddr_any_server.c
SRCS += socket_local_client.c
SRCS += socket_local_server.c
SRCS += socket_loopback_client.c
SRCS += socket_loopback_server.c
SRCS += socket_network_client.c

VPATH += ./libzipfile
SRCS += centraldir.c
SRCS += zipfile.c

VPATH+= ./zlib
SRCS += adler32.c
SRCS += compress.c
SRCS += crc32.c
SRCS += deflate.c
SRCS += infback.c
SRCS += inffast.c
SRCS += inflate.c
SRCS += inftrees.c
SRCS += trees.c
SRCS += uncompr.c
SRCS += zutil.c

CFLAGS += -DADB_HOST=0
CFLAGS += -DHAVE_FORKEXEC=1
CFLAGS += -DHAVE_SYMLINKS
CFLAGS += -DHAVE_TERMIO_H
CFLAGS += -D_GNU_SOURCE
CFLAGS += -D_XOPEN_SOURCE
CFLAGS += -I.
CFLAGS += -I../include
CFLAGS += -I./zlib
CFLAGS += -I../../inc
CFLAGS += -I/home/roadjun/work/pro_prj/extlib/install/include


CFLAGS += -O2 -g -Wall -Wno-unused-parameter
LDFLAGS = -static -L/home/roadjun/work/pro_prj/extlib/install/lib
LIBS = -lrt -lpthread  -lssl -lcrypto -ldl

TOOLCHAIN = arm-hisiv400-linux-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)gcc

OBJS= $(SRCS:.c=.o)

all: adbd

adbd: $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS)
