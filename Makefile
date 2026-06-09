.PHONY: build test cpptests erltest ctags format install clean pytest clang-tidy alltest hex_prepare hex_build publish_hex clean_hex

HEX_LIBWROUTER_DIR := bindings/erlang/c_src/libwrouter

build:
	meson setup build
	meson compile -C build

test: build
	meson test -C build --print-errorlogs -v

cpptests: build
	meson test -C build cpp --print-errorlogs -v

erltest: build
	meson test -C build erlang --print-errorlogs -v;

pytest: test
	PYTHONPATH=build/bindings/python pytest -s bindings/python/tests/

ctags:
	ctags -R --kinds-C=+stu --extras=+q -f tags .

format:
	clang-format -i include/* src/* tests/*.c tests/helpers/* examples/libmicrohttpd/*.c bindings/python/*.c bindings/python/*.h

alltest: test cpptests erltest pytest

coverage:
	meson setup build-coverage -Db_coverage=true -Db_sanitize=none
	meson compile -C build-coverage
	meson test -C build-coverage --print-errorlogs -v
	lcov --capture --directory build-coverage --output-file coverage.info
	lcov --extract coverage.info "$$(pwd)/src/*" --output-file coverage.filtered.info
	genhtml coverage.filtered.info --output-directory coverage_html

clang-tidy:
	clang-tidy src/params.c -p build --fix --checks=misc-include-cleaner

hex_prepare:
	rm -rf $(HEX_LIBWROUTER_DIR)
	mkdir -p $(HEX_LIBWROUTER_DIR)/include $(HEX_LIBWROUTER_DIR)/src
	cp LICENSE $(HEX_LIBWROUTER_DIR)/LICENSE
	cp include/wrouter.h $(HEX_LIBWROUTER_DIR)/include/
	cp src/*.c src/*.h $(HEX_LIBWROUTER_DIR)/src/

hex_build: hex_prepare
	rebar3 hex build

publish_hex: hex_prepare
	rebar3 hex publish package --repo hexpm --yes

clean_hex:
	rm -rf $(HEX_LIBWROUTER_DIR)

clean:
	meson setup --wipe build

install:
	meson setup build
	meson compile -C build
	meson install -C build
