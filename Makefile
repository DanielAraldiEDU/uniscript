# Compiler and flags
CXX := g++
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -Wpedantic -fPIC
CPPFLAGS := -Isrc

# Folders
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

# Target binary name
TARGET := $(BIN_DIR)/uniscript

# Source files
SRCS := \
  $(SRC_DIR)/main.cpp \
  $(SRC_DIR)/CompatibilityTable.cpp \
  $(wildcard $(SRC_DIR)/gals/*.cpp)

# Object files mapped to build directory, preserving tree
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Pattern rule to compile .cpp to .o under build directory
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Ensure output directories exist
$(BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

# Convenience target to build and execute the CLI quickly
run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: docker-build docker-artifacts wasm-docker

docker-build:
	docker build -f docker/Dockerfile -t uniscript-build --target artifacts .

docker-artifacts:
	CID=$$(docker create uniscript-build); \
	docker cp $$CID:/out ./artifacts; \
	docker rm $$CID

# Build WASM via Docker and place artifacts into web/public
wasm-docker: docker-build | web/public
	@set -e; \
	CID=$$(docker create uniscript-build /bin/true); \
	if [ -z "$$CID" ]; then echo "[wasm-docker] Falha ao criar container"; exit 1; fi; \
	docker cp $$CID:/out/. web/public/; \
	docker rm $$CID >/dev/null; \
	echo "[wasm-docker] Done: web/public/uniscript.js + web/public/uniscript.wasm"

# -----------------
# Frontend helpers
# -----------------
.PHONY: web-install web-dev web-build web-preview

web-install:
	cd web && npm install

# Starts Vite dev server. Ensures WASM is present first (local or Docker fallback)
web-dev: wasm
	cd web && npm run dev

# Build production bundle. Ensures WASM is present first
web-build: wasm
	cd web && npm run build

web-preview:
	cd web && npm run preview

# -----------------
# WebAssembly (Emscripten)
# -----------------

.PHONY: wasm

# Build the compiler core to WebAssembly for use in the React SPA.
wasm: | web/public
	@if command -v emcc >/dev/null 2>&1; then \
	  echo "[wasm] Building WebAssembly module into web/public ..."; \
	  emcc \
	    src/gals/*.cpp \
	    bridge.cpp \
	    -O3 -s MODULARIZE=1 -s EXPORT_NAME=createUniscriptModule -s ENVIRONMENT=web -fwasm-exceptions \
	    -s EXPORTED_FUNCTIONS='["_uniscript_compile","_free"]' \
	    -s EXPORTED_RUNTIME_METHODS='["cwrap","UTF8ToString"]' \
	    -I src \
	    -o web/public/uniscript.js; \
	  echo "[wasm] Done: web/public/uniscript.js + web/public/uniscript.wasm"; \
	else \
	  echo "[wasm] 'emcc' não encontrado. Usando fallback via Docker..."; \
	  $(MAKE) wasm-docker; \
	fi

web/public:
	@mkdir -p web/public
