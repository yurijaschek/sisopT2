CC=gcc

INC_DIR=../include
LIB_DIR=../lib

CFLAGS := 

SRCS := $(wildcard *.c)

.PHONY: all clean

all: $(SRCS:.c=)

%: %.c $(LIB_DIR)/libt2fs.a
	$(CC) $(CFLAGS) $< -o $@ -I$(INC_DIR) -L$(LIB_DIR) -lt2fs

clean:
	rm -f $(patsubst %.c,%,$(SRCS))
