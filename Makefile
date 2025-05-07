# Makefile Targets
#          all:	compiles the source code
#        clean: removes all .hex, .elf, and .o files in the source code and
#              	library directories
#      install:	installs the application and documentation
#    uninstall:	uninstalls the application and documentation

# Build Variables +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Application target executable name
APP_TARGET = vgp
# Unit test target executable name
UNIT_TEST_TARGET = test_vgp
# Integration test target executable name
INTEGRATION_TEST_TARGET = integration_test
# Valgrind output file
VALGRIND.OUT = valgrind.out
# VGP output file
VGP.OUT = vgp.out
# Integration test output file
INTEGRATION_TEST.OUT = integration_test.out
# Project root directory
PROJECT_ROOT = $(shell pwd)
# Source directory
SRCDIR = $(PROJECT_ROOT)
# App main source file
APP_SRC = $(SRCDIR)/main.c
# App object file
APP_OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILD_DIR)/%.o,$(APP_SRC))
# Unit test main source file
UNIT_TEST_SRC = $(SRCDIR)/test_vgp.c
# Unit test object file
UNIT_TEST_OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILD_DIR)/%.o,$(UNIT_TEST_SRC))
# Integration test main source file
INTEGRATION_TEST_SRC = $(SRCDIR)/integration_test.c
# Integration test object file
INTEGRATION_TEST_OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILD_DIR)/%.o,$(INTEGRATION_TEST_SRC))
# Common source files
COMMON_SRC = $(filter-out $(APP_SRC) $(UNIT_TEST_SRC) $(INTEGRATION_TEST_SRC),$(wildcard $(SRCDIR)/*.c))
# Common object files
COMMON_OBJS = $(patsubst $(SRCDIR)/%.c,$(BUILD_DIR)/%.o,$(COMMON_SRC))
# Dependency files
DEPENDS = $(OBJECTS:.o=.d)
# Linux usr binaries directory
BINDIR =	/usr/local/bin
# Linux manual directory for the man command
MANDIR =	/usr/local/man/man1
# Build directory
BUILD_DIR = $(PROJECT_ROOT)/build
# Systemd services directory
SYSDDIR = /etc/systemd/system
# Udev rules directory
UDEVDIR = /etc/udev/rules.d
# C compiler command
CC = gcc
# C++ compiler command
CXX = g++
# Build flags for c files
CFLAGS = -g -I/usr/local/include -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wredundant-decls -Wno-long-long -Wno-parentheses -DNCURSES_WIDECHAR=1 -MMD -MP -std=c99
# Build flags for c files
CXXFLAGS = -g -I/usr/local/include -I/usr/local/include/CppUTest -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wredundant-decls -Wno-long-long -Wno-parentheses -DNCURSES_WIDECHAR=1 -MMD -MP
# Debug flags for c files
DFLAGS = -g3 -fsanitize=address -I/usr/local/include -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wredundant-decls -Wno-long-long -Wno-parentheses -DNCURSES_WIDECHAR=1 -MMD -MP -std=c99
# Linker Flags
LDFLAGS = -L/usr/local/lib -lCppUTest -lCppUTestExt
#-lncursesw
# Application command line options
OPTIONS = -c -v

# Build Targets +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Build all target files
all:		build $(BUILD_DIR)/$(APP_TARGET)

# Build all test target files
test:		build $(BUILD_DIR)/$(UNIT_TEST_TARGET) $(BUILD_DIR)/$(INTEGRATION_TEST_TARGET) $(BUILD_DIR)/$(APP_TARGET)
	@echo "Running unit tests..."
	$(BUILD_DIR)/$(UNIT_TEST_TARGET) $(OPTIONS)
	@echo "Running integration tests..."
	valgrind --leak-check=full --log-file=$(BUILD_DIR)/$(VALGRIND.OUT) --fullpath-after=string $(BUILD_DIR)/$(INTEGRATION_TEST_TARGET) >> $(BUILD_DIR)/$(INTEGRATION_TEST.OUT)
	$(BUILD_DIR)/$(APP_TARGET) $(BUILD_DIR)/$(VALGRIND.OUT) > $(BUILD_DIR)/$(VGP.OUT)

# Create the build directory
build:
	mkdir -p $(BUILD_DIR)

# Build the app from the object files
$(BUILD_DIR)/$(APP_TARGET): $(APP_OBJ) $(COMMON_OBJS)
	@echo "Linking $(APP_TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile the .c files to .o files in the build directory
$(BUILD_DIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile the .cpp files to .o files in the build directory
$(BUILD_DIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build the unit test app from the object files
$(BUILD_DIR)/$(UNIT_TEST_TARGET): $(UNIT_TEST_OBJ) $(COMMON_OBJS)
	@echo "Linking $(UNIT_TEST_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build the integration test app from the object files
$(BUILD_DIR)/$(INTEGRATION_TEST_TARGET): $(INTEGRATION_TEST_OBJ) $(COMMON_OBJS)
	@echo "Linking $(INTEGRATION_TEST_TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build the app with debug symbols
debug: build $(COMMON_OBJS)
	@echo "Building $(APP_TARGET) with debug symbols..."
	$(CC) $(DFLAGS) -o $(BUILD_DIR)/$(APP_TARGET) $(COMMON_OBJS) $(LDFLAGS)

# Run TARGET with the provided command line options
run: all
	$(BUILD_DIR)/$(APP_TARGET) $(OPTIONS)

# Install the application
install:	all
	sudo cp $(BUILD_DIR)/$(APP_TARGET) $(BINDIR)
	sudo cp $(APP_TARGET).1 $(MANDIR)

# Uninstall the application
uninstall:
	sudo rm -f $(BINDIR)/$(APP_TARGET)
	sudo rm -f $(MANDIR)/$(APP_TARGET).1

# Install prereqeuisites
prereqs:
	sudo apt update
	sudo apt install libncurses5-dev libncursesw5-dev

# Clean up all generated files
clean:
	rm -rf $(BUILD_DIR)

# Include dependencies generated by GCC (-MMD -MP flags)
-include $(DEPENDS)
