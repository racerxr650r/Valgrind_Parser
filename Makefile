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
# Valgrind output file
VALGRIND.OUT = valgrind.out
# VGP output file
VGP.OUT = vgp.out
# Integration test output file
INTEGRATION_TEST_RUN.OUT = integration_test_run.out
# Project root directory
PROJECT_ROOT = $(shell pwd)
# Source directory
SRCDIR = $(PROJECT_ROOT)/src
# Test directory
TESTDIR = $(PROJECT_ROOT)/test
# Unit test directory
UNITDIR = $(TESTDIR)/unit
# Integration test directory
INTEGRATIONDIR = $(TESTDIR)/integration
# App main source file
APP_SRC = $(SRCDIR)/main.c
# App object file
APP_OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(APP_SRC))
# Unit test main source file
UNIT_TEST_SRC = $(wildcard $(UNITDIR)/*.c)
# Unit test object files (one for each source file)
UNIT_TEST_OBJ = $(patsubst $(UNITDIR)/%.c,$(BUILDDIR)/%.o,$(UNIT_TEST_SRC))
# Unit test executables (one for each source file, e.g., build/test_foo_runner)
UNIT_TEST_EXECUTABLES = $(patsubst $(UNITDIR)/%.c,$(BUILDDIR)/%_runner,$(UNIT_TEST_SRC))
# Integration test main source file
INTEGRATION_TEST_C_SRC = $(wildcard $(INTEGRATIONDIR)/*.c)
INTEGRATION_TEST_F90_SRC = $(wildcard $(INTEGRATIONDIR)/*.f90)
INTEGRATION_TEST_CPP_SRC = $(wildcard $(INTEGRATIONDIR)/*.cpp)
INTEGRATION_TEST_SRC = $(INTEGRATION_TEST_C_SRC) $(INTEGRATION_TEST_F90_SRC) $(INTEGRATION_TEST_CPP_SRC)
# Integration test C object files (if we were to compile them separately, not strictly needed for single-file apps)
INTEGRATION_TEST_C_OBJ = $(patsubst $(INTEGRATIONDIR)/%.c,$(BUILDDIR)/%.o,$(INTEGRATION_TEST_C_SRC))
# Integration test executables (one for each source file)
INTEGRATION_TEST_C_APPS = $(patsubst $(INTEGRATIONDIR)/%.c,$(BUILDDIR)/%_int_app_c,$(INTEGRATION_TEST_C_SRC))
INTEGRATION_TEST_F90_APPS = $(patsubst $(INTEGRATIONDIR)/%.f90,$(BUILDDIR)/%_int_app_f90,$(INTEGRATION_TEST_F90_SRC))
INTEGRATION_TEST_CPP_APPS = $(patsubst $(INTEGRATIONDIR)/%.cpp,$(BUILDDIR)/%_int_app_cpp,$(INTEGRATION_TEST_CPP_SRC))
INTEGRATION_TEST_EXECUTABLES = $(INTEGRATION_TEST_C_APPS) $(INTEGRATION_TEST_F90_APPS) $(INTEGRATION_TEST_CPP_APPS)
# Common source files (library code like vgp.c)
COMMON_SRC = $(filter-out $(APP_SRC) $(UNIT_TEST_SRC) $(INTEGRATION_TEST_SRC),$(wildcard $(SRCDIR)/*.c))
# Common object files
COMMON_OBJS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(COMMON_SRC))
# Dependency files
DEPENDS = $(OBJECTS:.o=.d)
# Linux usr binaries directory
BINDIR =	/usr/local/bin
# All C object files for dependency generation
ALL_OBJS = $(APP_OBJ) $(COMMON_OBJS) $(UNIT_TEST_OBJ) $(INTEGRATION_TEST_C_OBJ)
# Linux manual directory for the man command
MANDIR =	/usr/local/man/man1
# Documentation directory
DOCDIR = $(PROJECT_ROOT)/doc
# Include directory
INCDIR = $(PROJECT_ROOT)/inc
# Build directory
BUILDDIR = $(PROJECT_ROOT)/build
# Systemd services directory
SYSDDIR = /etc/systemd/system
# Udev rules directory
UDEVDIR = /etc/udev/rules.d
# C compiler command
CC = gcc
# C++ compiler command
CXX = g++
# Fortran compiler command
FC = gfortran
# Fortran Flags (add -g for Valgrind debug info)
FFLAGS = -g -Wall -Wno-uninitialized
# Release flags for c files
RELEASE_FLAGS = -g -I/usr/local/include -I$(INCDIR) -Wno-return-local-addr -Wno-free-nonheap-object -MMD -MP -std=c99
# C++ Flags (similar to C flags, adjust as needed, e.g., -std=c++11)
CXXFLAGS = -g -I/usr/local/include -I$(INCDIR) -MMD -MP # -std=c++11 or newer
# Debug flags for c files
DEBUG_FLAGS = -g3 -I/usr/local/include -I/usr/include/cmocka -I$(INCDIR) -fanalyzer -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wredundant-decls -Wno-long-long -Wno-parentheses -fprofile-arcs -ftest-coverage -DNCURSES_WIDECHAR=1 -MMD -MP -std=c99
# Default flags for c files
CFLAGS = $(RELEASE_FLAGS)
# Release Linker Flags
RELEASE_LDFLAGS = -L/usr/local/lib
# Debug Linker Flags
DEBUG_LDFLAGS = -L/usr/local/lib -lgcov -lcmocka
# Default Linker Flags
LDFLAGS = $(RELEASE_LDFLAGS)
# Default C++ Linker Flags (can be same as LDFLAGS or specific)
CXXLDFLAGS = $(LDFLAGS)
# Path to the main application executable
APP_EXE = $(BUILDDIR)/$(APP_TARGET)
# Application command line options
OPTIONS = -c -v

# Build Targets +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Build all target files with out debug info
release:		CFLAGS = $(RELEASE_FLAGS)
release:		LDFLAGS = $(RELEASE_LDFLAGS)
release:		build $(BUILDDIR)/$(APP_TARGET)

# Build all target files with debug info
debug:		CFLAGS = $(DEBUG_FLAGS)
debug:		LDFLAGS = $(DEBUG_LDFLAGS)
debug:		build $(BUILDDIR)/$(APP_TARGET)

# Build integration test target files without debug info
integration_tests:		CFLAGS = $(RELEASE_FLAGS)
integration_tests:		LDFLAGS = $(RELEASE_LDFLAGS)
integration_tests:		build $(INTEGRATION_TEST_EXECUTABLES)

# Build all test target files
test:		CFLAGS = $(DEBUG_FLAGS)
test:		LDFLAGS = $(DEBUG_LDFLAGS)
test:		clean build debug $(UNIT_TEST_EXECUTABLES) integration_tests
	@echo "Running unit tests..."
	@for test_exe in $(UNIT_TEST_EXECUTABLES); do \
		echo "--------------------------------------------------"; \
		echo "***Running unit test $$test_exe..."; \
		$$test_exe; \
	done
	@echo "--------------------------------------------------"
#	lcov --branch-coverage --capture --directory $(BUILDDIR) --output-file $(BUILDDIR)/coverage.info --ignore-errors mismatch
#	genhtml $(BUILDDIR)/coverage.info --branch-coverage --output-directory $(BUILDDIR)/coverage
	@echo "Cleaning previous integration test output files..."
	@$(RM) $(BUILDDIR)/*_valgrind.log $(BUILDDIR)/*_vgp_output.log $(BUILDDIR)/*_app_stdout.log
	@echo "Running integration tests..."
	@for test_exe in $(INTEGRATION_TEST_EXECUTABLES); do \
		test_exe_basename=$$(basename $$test_exe); \
		valgrind_log_for_vgp=$(BUILDDIR)/$${test_exe_basename}_valgrind.log; \
		vgp_output_file=$(BUILDDIR)/$${test_exe_basename}_vgp_output.log; \
		test_app_stdout_file=$(BUILDDIR)/$${test_exe_basename}_app_stdout.log; \
		\
		echo "--------------------------------------------------"; \
		echo "*** Generating Valgrind log for $$test_exe_basename (output to $$valgrind_log_for_vgp)..."; \
		valgrind --leak-check=full --log-file=$$valgrind_log_for_vgp --fullpath-after=string $$test_exe > $$test_app_stdout_file 2>&1; \
		\
		echo "*** Running vgp on $$valgrind_log_for_vgp (output to $$vgp_output_file)..."; \
		$(APP_EXE) -v -l $$valgrind_log_for_vgp > $$vgp_output_file; \
		\
		echo "*** vgp output for $$test_exe_basename is in $$vgp_output_file"; \
		echo "*** Valgrind log for $$test_exe_basename is in $$valgrind_log_for_vgp"; \
		echo "*** stdout/stderr of $$test_exe_basename is in $$test_app_stdout_file"; \
	done
	@echo "--------------------------------------------------"

# Create the build directory
build:
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR)/cppcheck
	mkdir -p $(BUILDDIR)/coverage

# Build the app from the object files
$(BUILDDIR)/$(APP_TARGET): $(APP_OBJ) $(COMMON_OBJS)
	@echo "***Linking $(APP_TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile the .c files to .o files in the build directory
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "***Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile unit test .c files from TEST_UNIT_DIR to .o files in the build directory
$(BUILDDIR)/%.o: $(UNITDIR)/%.c
	@echo "***Compiling unit test source $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile integration test .c files from TESTDIR to .o files in the build directory
$(BUILDDIR)/%.o: $(INTEGRATIONDIR)/%.c
	@echo "***Compiling integration test source $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Build individual unit test executables
$(BUILDDIR)/%_runner: $(BUILDDIR)/%.o $(COMMON_OBJS)
	@echo "***Linking unit test $@..."
	$(CC) $(CFLAGS) -o $@ $< $(COMMON_OBJS) $(LDFLAGS)

# Build C integration test executables (assuming they are single-file and self-contained)
$(BUILDDIR)/%_int_app_c: $(INTEGRATIONDIR)/%.c
	@echo "***Compiling and Linking C integration test $@ from $<..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Build Fortran integration test executables (assuming they are single-file and self-contained)
$(BUILDDIR)/%_int_app_f90: $(INTEGRATIONDIR)/%.f90
	@echo "***Compiling and Linking Fortran integration test $@ from $<..."
	$(FC) $(FFLAGS) -o $@ $< $(LDFLAGS)

# Build C++ integration test executables (assuming they are single-file and self-contained)
$(BUILDDIR)/%_int_app_cpp: $(INTEGRATIONDIR)/%.cpp
	@echo "***Compiling and Linking C++ integration test $@ from $<..."
	$(CXX) $(CXXFLAGS) -o $@ $< $(CXXLDFLAGS)

# Run TARGET with the provided command line options
run: debug
	$(BUILDDIR)/$(APP_TARGET) $(OPTIONS)

# Install the application
install:	clean release
	sudo cp $(BUILDDIR)/$(APP_TARGET) $(BINDIR)
	sudo cp $(DOCDIR)/$(APP_TARGET).1 $(MANDIR)

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
	cppcheck -v --std=c99 --platform=avr8 --library=avr.cfg --max-ctu-depth=10 --cppcheck-build-dir=$(BUILDDIR)/cppcheck --language=c --inconclusive -I . $(APP_SRC) $(COMMON_SRC)

# Install prereqeuisites
prereqs:
	sudo apt update
	sudo apt install -y universal-ctags lcov valgrind sloccount complexity cppcheck libcmocka-dev cmocka-doc gfortran g++

# Clean up all generated files
clean:
	rm -rf $(BUILDDIR)

# Include dependencies generated by GCC (-MMD -MP flags)
-include $(ALL_OBJS:.o=.d)

.PHONY: all clean install uninstall run test debug release integration_test complexity sloc analyze prereqs
