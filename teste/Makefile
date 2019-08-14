CXX=g++

OBJ_DIR=./obj
SRC_DIR=./src

CPPFLAGS := -std=c++11 -Wall -Wextra

SHELL_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
SHELL_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SHELL_SRCS))

.PHONY: all clean

all: t2shell

t2shell: $(SHELL_OBJS)
	$(CXX) $(CPPFLAGS) $^ -o $@ -L../lib -lt2fs

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) -c $^ -o $@ -I../include

clean:
	rm -f t2shell
	rm -f $(OBJ_DIR)/*.o
