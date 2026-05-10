.PHONY: default test ctags format

default:
	meson setup build
	meson compile -C build

test:
	meson test -C build --print-errorlogs -v

ctags:
	ctags -R --languages=C --c-kinds=+p --fields=+iaS --extras=+q .

format:
	clang-format -i include/* src/*
