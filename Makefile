.PHONY: build test cpptests erlangtest ctags format install clean pytest clang-tidy alltest

build:
	meson setup build
	meson compile -C build

test: build
	meson test -C build --print-errorlogs -v

cpptests: build
	meson test -C build cpp --print-errorlogs -v

erlangtest: build
	@if meson test -C build --list | grep -qx 'libwrouter:erlang'; then \
		meson test -C build erlang --print-errorlogs -v; \
	else \
		echo "Skipping Erlang tests; Erlang bindings were not configured."; \
	fi

pytest: test
	PYTHONPATH=build/bindings/python pytest -s bindings/python/tests/

ctags:
	ctags -R --kinds-C=+stu --extras=+q -f tags .

format:
	clang-format -i include/* src/* tests/*.c tests/helpers/* examples/libmicrohttpd/*.c bindings/python/*.c bindings/python/*.h

alltest: test cpptests erlangtest pytest

coverage:
	meson setup build-coverage -Db_coverage=true -Db_sanitize=none
	meson compile -C build-coverage
	meson test -C build-coverage --print-errorlogs -v
	lcov --capture --directory build-coverage --output-file coverage.info
	lcov --extract coverage.info "$$(pwd)/src/*" --output-file coverage.filtered.info
	genhtml coverage.filtered.info --output-directory coverage_html

clang-tidy:
	clang-tidy src/params.c -p build --fix --checks=misc-include-cleaner

clean:
	meson setup --wipe build

install:
	meson setup build
	meson compile -C build
	meson install -C build
