# Makefile Targets
#      release:	compiles the application source code for release
#        debug:	compiles the application source code with debug info
#         test:	compiles the test application with debug symbols and runs
#				the unit tests, generates code coverage report and runs
#				the integration test
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
# Release flags for c files
RELEASE_FLAGS = -I/usr/local/include -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wredundant-decls -Wno-long-long -Wno-parentheses -DNCURSES_WIDECHAR=1 -MMD -MP -std=c99
# Debug flags for c files
DEBUG_FLAGS = -g3 -fsanitize=address -I/usr/local/include -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wredundant-decls -Wno-long-long -Wno-parentheses -fprofile-arcs -ftest-coverage -DNCURSES_WIDECHAR=1 -MMD -MP -std=c99
# Build flags for c++ files
CXXFLAGS = -g -I/usr/local/include -I/usr/local/include/CppUTest -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wredundant-decls -Wno-long-long -Wno-parentheses -DNCURSES_WIDECHAR=1 -MMD -MP
# Default flags for c files
CFLAGS = $(RELEASE_FLAGS)
# Release Linker Flags
RELEASE_LDFLAGS = -L/usr/local/lib
# Debug Linker Flags
DEBUG_LDFLAGS = -L/usr/local/lib -lCppUTest -lCppUTestExt -lgcov
# Default Linker Flags
LDFLAGS = $(RELEASE_LDFLAGS)
# Application command line options
OPTIONS = -c -v

# Build Targets +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Build all target files with out debug info
release:		clean build $(BUILD_DIR)/$(APP_TARGET)

# Build all target files with debug info
debug:		CFLAGS = $(DEBUG_FLAGS)
debug:		LDFLAGS = $(DEBUG_LDFLAGS)
debug:		clean build $(BUILD_DIR)/$(APP_TARGET)

# Build all test target files
test:		CFLAGS = $(DEBUG_FLAGS)
test:		LDFLAGS = $(DEBUG_LDFLAGS)
test:		clean build $(BUILD_DIR)/$(UNIT_TEST_TARGET) $(BUILD_DIR)/$(INTEGRATION_TEST_TARGET) $(BUILD_DIR)/$(APP_TARGET)
	@echo "Running unit tests..."
	$(BUILD_DIR)/$(UNIT_TEST_TARGET) $(OPTIONS)
#	gcov $(BUILD_DIR)/vgp
	lcov --branch-coverage --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info --ignore-errors mismatch
	genhtml $(BUILD_DIR)/coverage.info --branch-coverage --output-directory $(BUILD_DIR)/coverage
	@echo "Running integration tests..."
	valgrind --leak-check=full --log-file=$(BUILD_DIR)/$(VALGRIND.OUT) --fullpath-after=string $(BUILD_DIR)/$(INTEGRATION_TEST_TARGET) >> $(BUILD_DIR)/$(INTEGRATION_TEST.OUT)
	valgrind --leak-check=full --log-file=$(BUILD_DIR)/integration_test_valgrind.out --fullpath-after=string $(BUILD_DIR)/$(APP_TARGET) -v $(BUILD_DIR)/$(VALGRIND.OUT) > $(BUILD_DIR)/$(VGP.OUT)

# Create the build directory
build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/cppcheck
	mkdir -p $(BUILD_DIR)/coverage

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
#debug: build $(COMMON_OBJS)
#	@echo "Building $(APP_TARGET) with debug symbols..."
#	$(CC) $(DFLAGS) -o $(BUILD_DIR)/$(APP_TARGET) $(COMMON_OBJS) $(LDFLAGS)

# Run TARGET with the provided command line options
run: all
	$(BUILD_DIR)/$(APP_TARGET) $(OPTIONS)

# Install the application
install:	clean all
	sudo cp $(BUILD_DIR)/$(APP_TARGET) $(BINDIR)
	sudo cp $(APP_TARGET).1 $(MANDIR)

# Uninstall the application
uninstall:
	sudo rm -f $(BINDIR)/$(APP_TARGET)
	sudo rm -f $(MANDIR)/$(APP_TARGET).1

# Determine the code complexity
complexity:
	complexity -H -h -c --threshold=0 $(APP_SRC) $(COMMON_SRC)

# SLOC Count
sloc: build
	sloccount --personcost 100000 --datadir ./build .

# static code analysis
analyze: build
	cppcheck -v --std=c99 --platform=avr8 --library=avr.cfg --max-ctu-depth=10 --cppcheck-build-dir=$(BUILD_DIR)/cppcheck --language=c --inconclusive -I . $(APP_SRC) $(COMMON_SRC)

# Install prereqeuisites
prereqs:
	sudo apt update
	sudo apt install -y cpputest libcpputest-dev lcov valgrind sloccount complexity cppcheck
	git clone https://github.com/cpputest/cpputest.git
	cd cpputest
	mkdir cpputest_build
	cmake -B cpputest_build
	cmake --build cpputest_build
	cd ..
	rm -rf cpputest

# Clean up all generated files
clean:
	rm -rf $(BUILD_DIR)

# Include dependencies generated by GCC (-MMD -MP flags)
-include $(DEPENDS)
