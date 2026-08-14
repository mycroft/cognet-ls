# ----------------------------------------------------------------------------
# "THE BEER-WARE LICENSE" (Revision 42):
# mycroft wrote this file. As long as you retain this notice you
# can do whatever you want with this stuff. If we meet some day, and you think
# this stuff is worth it, you can buy me a beer in return.
# ----------------------------------------------------------------------------
CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2
TARGET  := ls
SRC     := ls.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
