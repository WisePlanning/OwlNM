##########################################
# Make file for network sync player system
# Author: Drew(Drew@wise-p.co.jp)
# Version: 0.7
##########################################

PREFIX := /usr/local
SRC := src
BUILDDIR := build
CC := clang
MKDIR_P = mkdir -p
BIN := bin
CPPFLAGS := -Wall -c -I/usr/local/include -I/usr/src/sys/dev/evdev/
OBJS := config.o common.o key_util.o server.o client.o control.o service_client.o service_server.o video.o

.PHONY: directories

directories:
	${MKDIR_P} ${BIN}
	${MKDIR_P} ${BUILDDIR}

OwlNM: $(OBJS)
	$(CC) -Wall -O2 -L/usr/local/lib -Ibuild -Isrc -I/usr/local/include -lavahi-common -lavahi-client -levent $(BUILDDIR)/${OBJS} $(SRC)/OwlNM.c -o $(OUT_DIR)/OwlNM

client.o: $(SRC)/config.h $(SRC)/client.c
	$(CC) $(CPPFLAGS) $(SRC)/client.c -o $(BUILDDIR)/client.o

server.o: $(SRC)/config.h $(SRC)/server.c
	$(CC) $(CPPFLAGS) $(SRC)/server.c -o $(BUILDDIR)/server.o

control.o: $(SRC)/config.h $(SRC)/control.c $(BUILDDIR)/common.o
	$(CC) $(CPPFLAGS) $(SRC)/control.c -o $(BUILDDIR)/control.o

config.o: $(SRC)/config.h
	$(CC) $(CPPFLAGS) $(SRC)/config.c -o $(BUILDDIR)/config.o

common.o: $(SRC)/config.h $(SRC)/common.c
	$(CC) $(CPPFLAGS) $(SRC)/common.c -o $(BUILDDIR)/common.o

key_util.o: $(SRC)/key_util.c
	$(CC) $(CPPFLAGS) $(SRC)/key_util.c -o $(BUILDDIR)/key_util.o

video.o: $(SRC)/video.c
	$(CC) $(CPPFLAGS) $(SRC)/video.c -o $(BUILDDIR)/video.o

service_client.o:
	$(CC) $(CPPFLAGS) $(SRC)/service_client.c -o $(BUILDDIR)/service_client.o

service_server.o:
	$(CC) $(CPPFLAGS) $(SRC)/service_server.c -o $(BUILDDIR)/service_server.o

.PHONY: install
install:
	mkdir -p /usr/local/bin
	cp $(BIN)/OwlNM $(DESTDIR)$(PREFIX)/$(BIN)/OwlNM

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/$(BIN)/OwlNM

.PHONY: clean
clean:
	rm -rf $(BIN) $(BUILDDIR) $(BIN)/OwlNM
