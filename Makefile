
MM_PREFIX=/opt/ffmpeg

CC=gcc
CFLAGS=-std=c99
#CFLAGS=-O0 -Drestrict=__restrict__ --std=c99

CPP=g++
CPPFLAGS=-std=c++11

LD_STDCPP=-lstdc++

IFLAGS=-I. -I${MM_PREFIX}/include
LFLAGS=-lcryptopp -L${MM_PREFIX}/lib -lavformat -lavcodec -lavutil -lswscale

.SUFFIXES:.cpp .c .o
.cpp.o:
	g++ ${CPPFLAGS} -c $< ${IFLAGS}
.c.o:
	gcc ${CFLAGS} -c $< ${IFLAGS}

all:TEST

TEST:test_utils.o util_crypt.o util_media.o util_convt.o
	${CPP} ${CFLAGS} -o $@ $? ${LFLAGS} ${LD_STDCPP}
	rm -rf $?
