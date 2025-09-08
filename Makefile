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
  $(wildcard $(SRC_DIR)/gals/*.cpp)

# Object files mapped to build directory, preserving tree
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Pattern rule to compile .cpp to .o under build directory

# Try to detect Qt6 or Qt5 Widgets via pkg-config (optional)
QT_CFLAGS := $(shell pkg-config --cflags Qt6Widgets 2>/dev/null || pkg-config --cflags Qt5Widgets 2>/dev/null)
QT_LIBS   := $(shell pkg-config --libs   Qt6Widgets 2>/dev/null || pkg-config --libs   Qt5Widgets 2>/dev/null)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(QT_CFLAGS) -c $< -o $@

# Ensure output directories exist
$(BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

# Convenience target to build and run
run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# -----------------
# gui
# -----------------

GUI_TARGET := $(BIN_DIR)/uniscript-gui
GUI_SRCS := \
  $(SRC_DIR)/gui/main.cpp \
  $(SRC_DIR)/gui/MainWindow.cpp \
  $(wildcard $(SRC_DIR)/gui/components/Header/*.cpp) \
  $(wildcard $(SRC_DIR)/gui/components/Editor/*.cpp) \
  $(wildcard $(SRC_DIR)/gui/components/Console/*.cpp) \
  $(wildcard $(SRC_DIR)/gals/*.cpp)
GUI_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(GUI_SRCS))

.PHONY: gui run-gui

gui: $(GUI_TARGET)

$(GUI_TARGET): $(GUI_OBJS) | $(BIN_DIR)
	@if [ -z "$(QT_LIBS)" ]; then \
	  echo "\n[!] Qt Widgets não encontrado via pkg-config (Qt6/Qt5)."; \
	  echo "    Instale Qt dev e pkg-config, ou ajuste QT_CFLAGS/QT_LIBS no Makefile."; \
	  exit 1; \
	fi
	$(CXX) $(CXXFLAGS) -o $@ $(GUI_OBJS) $(QT_LIBS)

run-gui: $(GUI_TARGET)
	$(GUI_TARGET)

# -------------
# Docker helpers
# -------------
.PHONY: docker-build docker-artifacts

docker-build:
	docker build -f docker/Dockerfile -t uniscript-build --target artifacts .

docker-artifacts:
	CID=$$(docker create uniscript-build); \
	docker cp $$CID:/out ./artifacts; \
	docker rm $$CID
