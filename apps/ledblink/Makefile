# Build with the Yocto SDK: source its environment-setup script first, which
# sets CC/CFLAGS/LDFLAGS to the ARM cross-compiler for the board.
#   source ~/sdk/dk1/environment-setup-cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi
#   make            build
#   make run        build, copy to the board and run it
#
# The same Makefile is reused later by the Yocto recipe (make install DESTDIR=...).

# ssh alias defined in ~/.ssh/config (root@192.168.0.19)
BOARD  ?= dk1
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin

CFLAGS += -Wall -Wextra

ledblink: ledblink.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

deploy: ledblink
	scp $< $(BOARD):

run: deploy
	ssh $(BOARD) ./ledblink $(ARGS)

install: ledblink
	install -D -m 0755 ledblink $(DESTDIR)$(BINDIR)/ledblink

clean:
	rm -f ledblink

.PHONY: deploy run install clean
