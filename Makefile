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
