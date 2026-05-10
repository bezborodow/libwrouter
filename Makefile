.PHONY: build test ctags format

build:
	meson setup build
	meson compile -C build

test:
	meson test -C build --print-errorlogs -v

ctags:
	ctags -R --languages=C --c-kinds=+p --fields=+iaS --extras=+q .

format:
	clang-format -i include/* src/* tests/*.c

coverage:
	meson setup build-coverage -Db_coverage=true -Db_sanitize=none
	meson compile -C build-coverage
	meson test -C build-coverage --print-errorlogs -v
	lcov --capture --directory build-coverage --output-file coverage.info
	lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.filtered.info
	genhtml coverage.filtered.info --output-directory coverage_html
