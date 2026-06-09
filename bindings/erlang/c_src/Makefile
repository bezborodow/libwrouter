MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
APP_DIR := $(abspath $(MAKEFILE_DIR)/..)
LIBWROUTER_DIR := $(APP_DIR)/c_src/libwrouter
APP_PRIV_DIR := $(REBAR_BUILD_DIR)/lib/wrouter/priv

ifeq ($(strip $(REBAR_BUILD_DIR)),)
APP_PRIV_DIR := $(APP_DIR)/priv
endif

ERLANG_ROOT_DIR ?= $(shell erl -noshell -eval 'io:format("~s", [code:root_dir()]), halt().')
ERLANG_INCLUDE_DIR ?= $(ERLANG_ROOT_DIR)/usr/include

TARGET := $(APP_PRIV_DIR)/wrouter_nif.so

SOURCES := \
	$(APP_DIR)/c_src/wrouter_nif.c \
	$(LIBWROUTER_DIR)/src/arena.c \
	$(LIBWROUTER_DIR)/src/builder.c \
	$(LIBWROUTER_DIR)/src/compiler.c \
	$(LIBWROUTER_DIR)/src/dispatcher.c \
	$(LIBWROUTER_DIR)/src/errors.c \
	$(LIBWROUTER_DIR)/src/graph.c \
	$(LIBWROUTER_DIR)/src/lexer.c \
	$(LIBWROUTER_DIR)/src/params.c \
	$(LIBWROUTER_DIR)/src/prelexer.c \
	$(LIBWROUTER_DIR)/src/router.c \
	$(LIBWROUTER_DIR)/src/segment.c \
	$(LIBWROUTER_DIR)/src/symbol.c \
	$(LIBWROUTER_DIR)/src/terminal.c \
	$(LIBWROUTER_DIR)/src/token.c

CFLAGS ?= -O3
CFLAGS += -std=c11 -fPIC -Wall -Wextra -Wpedantic -I$(LIBWROUTER_DIR)/include -I$(LIBWROUTER_DIR)/src -I$(ERLANG_INCLUDE_DIR)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -dynamiclib -undefined dynamic_lookup
else
	LDFLAGS += -shared
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(LIBWROUTER_DIR)/include/wrouter.h
	mkdir -p $(APP_PRIV_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)
