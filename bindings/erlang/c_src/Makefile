MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
APP_DIR := $(abspath $(MAKEFILE_DIR)/..)
ROOT_DIR := $(abspath $(APP_DIR)/../..)
APP_PRIV_DIR := $(REBAR_BUILD_DIR)/lib/wrouter/priv

ifeq ($(strip $(REBAR_BUILD_DIR)),)
APP_PRIV_DIR := $(APP_DIR)/priv
endif

ERLANG_ROOT_DIR ?= $(shell erl -noshell -eval 'io:format("~s", [code:root_dir()]), halt().')
ERLANG_INCLUDE_DIR ?= $(ERLANG_ROOT_DIR)/usr/include

TARGET := $(APP_PRIV_DIR)/wrouter_nif.so

SOURCES := \
	$(APP_DIR)/c_src/wrouter_nif.c \
	$(ROOT_DIR)/src/arena.c \
	$(ROOT_DIR)/src/builder.c \
	$(ROOT_DIR)/src/compiler.c \
	$(ROOT_DIR)/src/dispatcher.c \
	$(ROOT_DIR)/src/errors.c \
	$(ROOT_DIR)/src/graph.c \
	$(ROOT_DIR)/src/lexer.c \
	$(ROOT_DIR)/src/params.c \
	$(ROOT_DIR)/src/prelexer.c \
	$(ROOT_DIR)/src/router.c \
	$(ROOT_DIR)/src/segment.c \
	$(ROOT_DIR)/src/symbol.c \
	$(ROOT_DIR)/src/terminal.c \
	$(ROOT_DIR)/src/token.c

CFLAGS ?= -O3
CFLAGS += -std=c11 -fPIC -Wall -Wextra -Wpedantic -I$(ROOT_DIR)/include -I$(ROOT_DIR)/src -I$(ERLANG_INCLUDE_DIR)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -dynamiclib -undefined dynamic_lookup
else
	LDFLAGS += -shared
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(ROOT_DIR)/include/wrouter.h
	mkdir -p $(APP_PRIV_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)
