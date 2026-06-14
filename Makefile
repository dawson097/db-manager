.PHONY: debug release install clean distclean

BIN_DIR := build/bin

debug:
	cmake -B build/debug -DCMAKE_BUILD_TYPE=Debug
	cmake --build build/debug
	@mkdir -p $(BIN_DIR)/debug
	mv build/debug/db-manager $(BIN_DIR)/debug/db-manager
	ln -sf build/debug/compile_commands.json compile_commands.json

release:
	cmake -B build/release -DCMAKE_BUILD_TYPE=Release
	cmake --build build/release
	@mkdir -p $(BIN_DIR)/release
	mv build/release/db-manager $(BIN_DIR)/release/db-manager

install:
	cmake --install build/release

clean:
	rm -rf build $(BIN_DIR)
	rm -rf .cache

distclean: clean
	rm -f compile_commands.json
