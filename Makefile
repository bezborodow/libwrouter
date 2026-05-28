.PHONY: build test ctags format install

build:
	meson setup build
	meson compile -C build

test:
	meson test -C build --print-errorlogs -v

ctags:
	ctags -R --kinds-C=+stu --extras=+q -f tags .

format:
	clang-format -i include/* src/* tests/*.c

coverage:
	meson setup build-coverage -Db_coverage=true -Db_sanitize=none
	meson compile -C build-coverage
	meson test -C build-coverage --print-errorlogs -v
	lcov --capture --directory build-coverage --output-file coverage.info
	lcov --extract coverage.info "$$(pwd)/src/*" --output-file coverage.filtered.info
	genhtml coverage.filtered.info --output-directory coverage_html

install:
	meson setup build-install --prefix=$(HOME)/.local
	meson compile -C build-install
	meson install -C build-install
