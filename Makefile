AR=ar
CC=gcc

BIN_DIR=bin
INC_DIR=include
LIB_DIR=lib
SRC_DIR=src

CFLAGS := -std=gnu99 -Wall -Wextra

T2FS_SRCS := $(wildcard $(SRC_DIR)/*.c)
T2FS_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(T2FS_SRCS))
T2FS_LIB=$(LIB_DIR)/libt2fs.a

API_SRC=$(LIB_DIR)/apidisk.c
API_OBJ=$(LIB_DIR)/apidisk.o
API_OBJ64=$(LIB_DIR)/apidisk64.o

.PHONY: all all64 clean

all: $(T2FS_OBJS) $(API_OBJ)
	rm -f $(T2FS_LIB)
	$(AR) crs $(T2FS_LIB) $^

all64: $(T2FS_OBJS) $(API_OBJ64)
	rm -f $(T2FS_LIB)
	$(AR) crs $(T2FS_LIB) $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@ -I$(INC_DIR)

$(API_OBJ):
	

$(API_OBJ64): $(API_SRC)
	$(CC) $(CFLAGS) -c $^ -o $@ -I$(INC_DIR)

clean:
	rm -f $(BIN_DIR)/*.o $(T2FS_LIB) $(API_OBJ64)

