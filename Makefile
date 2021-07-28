
CC=cc
C_FLAGS = -g -Wall
INCLUDE =-I/home/frankzhang/work/curl/install/include
#LIBS := -L/home/frankzhang/work/curl/install/lib -lcurl
#LIBS += /home/frankzhang/work/curl/install/lib/libcurl.a

LIBS += /home/frankzhang/work/curl/install/lib/libcurl.a -lssl -lcrypto -lpthread -lz

SOURCES:=$(wildcard *.c)
TARGETLIST:=$(patsubst %.c,%,$(SOURCES))

.c:
	$(CC) $(C_FLAGS) $(LFLAGS) -o $@ $< $(INCLUDE) $(LIBS)

all:$(TARGETLIST)

.PRONY: clean
clean:
	@echo "Removing linked and compiled files......"
	rm -f *.o
	rm -f test


include $(BUILD_APP)
