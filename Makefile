.DEFAULT_GOAL := all

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := .

TARGET_NAME := code

FLAGS    := -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE -Incurses/install/include/ncursesw/ -Incurses/install/include/
CXXFLAGS := -Wall -Wextra -O2 -g -fPIE $(FLAGS)
LDFLAGS  := -Wl,-z,relro,-z,now -pie -Lncurses/install/lib/ $(FLAGS)
LDLIBS   := -lncursesw

export TMPDIR := $(abspath ncurses/tmp)
export TMP    := $(abspath ncurses/tmp)
export TEMP   := $(abspath ncurses/tmp)
export MYTEMP := $(abspath ncurses/tmp)

.PHONY: lib_all lib_clean
lib_all:
	(cd ncurses && mkdir -p tmp && ./configure --prefix=$(abspath ./ncurses/install) --with-normal --without-shared --without-debug --without-cxx --enable-widec --with-fallbacks=xterm-256color)
	$(MAKE) -C ncurses -j$(nproc)
	$(MAKE) -C ncurses install.libs
	$(MAKE) -C ncurses install.includes

lib_clean:
	$(MAKE) -C ncurses distclean
	rm -r ncurses/install

TARGET := $(BIN_DIR)/$(TARGET_NAME)
HDRS   := $(shell find $(SRC_DIR)/ -type f -name "*.h")
SRCS   := $(shell find $(SRC_DIR)/ -type f -name "*.c")
OBJS   := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS)) 

.PHONY: all clean debug

all: lib_all $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(addprefix -I, $(INCS)) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -MMD -MP -c -I$(SRC_DIR) -o $@ $<

DEPS := $(OBJS:.o=.d)
-include $(DEPS)

clean: lib_clean
	@rm -rf $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -ggdb3 -O0
debug: $(TARGET)

