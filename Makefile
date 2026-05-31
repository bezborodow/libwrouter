.PHONY: build test ctags format install clean

build:
	meson setup build
	meson compile -C build

test:
	meson test -C build --print-errorlogs -v

pytest:
	PYTHONPATH=build/bindings/python pytest bindings/python/tests/

ctags:
	ctags -R --kinds-C=+stu --extras=+q -f tags .

format:
	clang-format -i include/* src/* tests/*.c examples/libmicrohttpd/*.c bindings/python/*.c bindings/python/*.h

coverage:
	meson setup build-coverage -Db_coverage=true -Db_sanitize=none
	meson compile -C build-coverage
	meson test -C build-coverage --print-errorlogs -v
	lcov --capture --directory build-coverage --output-file coverage.info
	lcov --extract coverage.info "$$(pwd)/src/*" --output-file coverage.filtered.info
	genhtml coverage.filtered.info --output-directory coverage_html

clean:
	meson setup --wipe build

install:
	meson setup build
	meson compile -C build
	meson install -C build
