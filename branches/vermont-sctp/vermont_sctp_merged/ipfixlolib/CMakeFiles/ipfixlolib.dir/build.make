# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.4

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/alex/uni/vermont_merged

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/alex/uni/vermont_merged

# Include any dependencies generated for this target.
include ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make

# Include the progress variables for this target.
include ipfixlolib/CMakeFiles/ipfixlolib.dir/progress.make

# Include the compile flags for this target's objects.
include ipfixlolib/CMakeFiles/ipfixlolib.dir/flags.make

ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark: ipfixlolib/CMakeFiles/ipfixlolib.dir/flags.make
ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark: ipfixlolib/encoding.c

ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o: ipfixlolib/CMakeFiles/ipfixlolib.dir/flags.make
ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o: ipfixlolib/encoding.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/alex/uni/vermont_merged/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o"
	/usr/bin/gcc  $(C_FLAGS) -o ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o   -c /home/alex/uni/vermont_merged/ipfixlolib/encoding.c

ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.i"
	/usr/bin/gcc  $(C_FLAGS) -E /home/alex/uni/vermont_merged/ipfixlolib/encoding.c > ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.i

ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.s"
	/usr/bin/gcc  $(C_FLAGS) -S /home/alex/uni/vermont_merged/ipfixlolib/encoding.c -o ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.s

ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o.requires:

ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o.provides: ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o.requires
	$(MAKE) -f ipfixlolib/CMakeFiles/ipfixlolib.dir/build.make ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o.provides.build

ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o.provides.build: ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o

ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark: ipfixlolib/CMakeFiles/ipfixlolib.dir/flags.make
ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark: ipfixlolib/ipfixlolib.c

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o: ipfixlolib/CMakeFiles/ipfixlolib.dir/flags.make
ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o: ipfixlolib/ipfixlolib.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/alex/uni/vermont_merged/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o"
	/usr/bin/gcc  $(C_FLAGS) -o ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o   -c /home/alex/uni/vermont_merged/ipfixlolib/ipfixlolib.c

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.i"
	/usr/bin/gcc  $(C_FLAGS) -E /home/alex/uni/vermont_merged/ipfixlolib/ipfixlolib.c > ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.i

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.s"
	/usr/bin/gcc  $(C_FLAGS) -S /home/alex/uni/vermont_merged/ipfixlolib/ipfixlolib.c -o ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.s

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o.requires:

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o.provides: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o.requires
	$(MAKE) -f ipfixlolib/CMakeFiles/ipfixlolib.dir/build.make ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o.provides.build

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o.provides.build: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o

ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark: ipfixlolib/CMakeFiles/ipfixlolib.dir/flags.make
ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark: ipfixlolib/ipfix_names.c

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o: ipfixlolib/CMakeFiles/ipfixlolib.dir/flags.make
ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o: ipfixlolib/ipfix_names.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/alex/uni/vermont_merged/CMakeFiles $(CMAKE_PROGRESS_3)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o"
	/usr/bin/gcc  $(C_FLAGS) -o ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o   -c /home/alex/uni/vermont_merged/ipfixlolib/ipfix_names.c

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.i"
	/usr/bin/gcc  $(C_FLAGS) -E /home/alex/uni/vermont_merged/ipfixlolib/ipfix_names.c > ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.i

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.s"
	/usr/bin/gcc  $(C_FLAGS) -S /home/alex/uni/vermont_merged/ipfixlolib/ipfix_names.c -o ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.s

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o.requires:

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o.provides: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o.requires
	$(MAKE) -f ipfixlolib/CMakeFiles/ipfixlolib.dir/build.make ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o.provides.build

ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o.provides.build: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o

ipfixlolib/CMakeFiles/ipfixlolib.dir/depend: ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark

ipfixlolib/CMakeFiles/ipfixlolib.dir/depend.make.mark:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --magenta --bold "Scanning dependencies of target ipfixlolib"
	cd /home/alex/uni/vermont_merged && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/alex/uni/vermont_merged /home/alex/uni/vermont_merged/ipfixlolib /home/alex/uni/vermont_merged /home/alex/uni/vermont_merged/ipfixlolib /home/alex/uni/vermont_merged/ipfixlolib/CMakeFiles/ipfixlolib.dir/DependInfo.cmake

# Object files for target ipfixlolib
ipfixlolib_OBJECTS = \
"CMakeFiles/ipfixlolib.dir/encoding.o" \
"CMakeFiles/ipfixlolib.dir/ipfixlolib.o" \
"CMakeFiles/ipfixlolib.dir/ipfix_names.o"

# External object files for target ipfixlolib
ipfixlolib_EXTERNAL_OBJECTS =

ipfixlolib/libipfixlolib.a: ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o
ipfixlolib/libipfixlolib.a: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o
ipfixlolib/libipfixlolib.a: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o
ipfixlolib/libipfixlolib.a: ipfixlolib/CMakeFiles/ipfixlolib.dir/build.make
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C static library libipfixlolib.a"
	cd /home/alex/uni/vermont_merged/ipfixlolib && $(CMAKE_COMMAND) -P CMakeFiles/ipfixlolib.dir/cmake_clean_target.cmake
	cd /home/alex/uni/vermont_merged/ipfixlolib && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ipfixlolib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
ipfixlolib/CMakeFiles/ipfixlolib.dir/build: ipfixlolib/libipfixlolib.a

ipfixlolib/CMakeFiles/ipfixlolib.dir/requires: ipfixlolib/CMakeFiles/ipfixlolib.dir/encoding.o.requires
ipfixlolib/CMakeFiles/ipfixlolib.dir/requires: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfixlolib.o.requires
ipfixlolib/CMakeFiles/ipfixlolib.dir/requires: ipfixlolib/CMakeFiles/ipfixlolib.dir/ipfix_names.o.requires

ipfixlolib/CMakeFiles/ipfixlolib.dir/clean:
	cd /home/alex/uni/vermont_merged/ipfixlolib && $(CMAKE_COMMAND) -P CMakeFiles/ipfixlolib.dir/cmake_clean.cmake

