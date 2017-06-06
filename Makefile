# *********************************************************************
#  Generic Makefile With Header Dependencies
# *********************************************************************
#  Author:         	(c) Fabrizio Pietrucci
#  License:       	See the end of this file for license information
#  Created:        	December 17, 2016

#  Last changed:   	@Date: 2017/5/26 14:00
#  Changed by:     	@Author: fabrizio pietrucci
#  Version:  		@Verision 1.0

####### SETTINGS #######

# The compiler
CC = gcc

# Find the platform on which the Makefile is executing
PLATFORM =
ifeq ($(OS),Windows_NT)
	PLATFORM = win32
else
	PLATFORM = linux
endif

# Executable name
SRV_EXEC_NAME = server
CLNT_EXEC_NAME = client

# Top level SRC folder
SRC = src
# Directory containing the server and client code 
SRC_MAIN = $(SRC)/main
# Directory containing the win32/linux cross platform modules
SRC_COMPAT = $(SRC)/compat
# Directory containing the code common to all project
SRC_COMMON = $(SRC)/common

# Folder in which the object files will be placed
BUILD = build/$(PLATFORM)
# Folder in which the binary will be generated
BIN = bin/$(PLATFORM)
# Where to install the binary file (optional)
INST_PATH =

# Path containing project libraries (optional)
LIBS_PATH =
# Path in wich static libraries will be placed (must be one of the path in LIBS_PATH or none).
# This will be used to relink the project if one of the static lib changes (optional).
STATIC_PATH =

# Paths containing header files (optional)
INCLUDES = -I$(SRC_COMPAT) -I$(SRC_COMMON)
INCLUDES_SERVER = -I$(SRC_MAIN)/server
INCLUDES_CLIENT = -I$(SRC_MAIN)/client

#Linker flags
LDFLAGS =
# Compiler flags
CFLAGS = -O3 -Wall
# Libraries
LIBS =

# Source extension
SRC_EXT = c

###### SETTINGS END ######

ifeq ($(DBG),1)
	CFLAGS += -DDEBUG
endif

# Recursive wildcard, used to get all c files in a directory recursively
rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard,$d/,$2) \
						$(filter $(subst *,%,$2), $d))

# Get all static libraires in LIBS_PATH (used in order to relink the program if one of the static libs changes)
STATICLIBS = $(call rwildcard, $(STATIC_PATH), *.a)

# Get all the source files in SRC, SRC_COMPAT, SRC_COMMON
SOURCES = $(call rwildcard, $(SRC_MAIN), *.$(SRC_EXT))
SOURCES_COMMON = $(call rwildcard, $(SRC_COMMON), *.$(SRC_EXT))
# Get only *_linux.c under linux and *_win32.c under windows
SOURCES_COMPAT = $(call rwildcard, $(SRC_COMPAT), *$(PLATFORM).$(SRC_EXT))

# Set object files names from source file names (used in order
# to relink the program if one of the object file changes)
OBJECTS = $(SOURCES:$(SRC)/%.$(SRC_EXT)=$(BUILD)/%.o)
OBJECTS_COMMON = $(SOURCES_COMMON:$(SRC)/%.$(SRC_EXT)=$(BUILD)/%.o)
OBJECTS_COMPAT = $(SOURCES_COMPAT:$(SRC)%$(PLATFORM).$(SRC_EXT)=$(BUILD)%$(PLATFORM).o)

# The dependency files that will be used in order to add header dependencies
DEPEND = $(OBJECTS:.o=.d)
DEPEND_COMMON = $(OBJECTS_COMMON:.o=.d)
DEPEND_COMPAT = $(OBJECTS_COMPAT:.o=.d)

#get the server and client object files in separate variables
SRC_SERVER = $(call rwildcard, $(SRC_MAIN)/server, *.$(SRC_EXT))
SRC_CLIENT = $(call rwildcard, $(SRC_MAIN)/client, *.$(SRC_EXT))
OBJ_SERVER = $(SRC_SERVER:$(SRC)/%.$(SRC_EXT)=$(BUILD)/%.o)
OBJ_CLIENT = $(SRC_CLIENT:$(SRC)/%.$(SRC_EXT)=$(BUILD)/%.o)

.PHONY: all
all:
	@echo "Usage: make [server|client]"

# Creates the needed directories
.PHONY: createdirs
createdirs:
	@echo "Generic Make File With Header Dependencies, Copyright (C) 2016 Fabrizio Pietrucci"
	@echo "Creating directories"
	@mkdir -p $(dir $(OBJECTS))
	@mkdir -p $(dir $(OBJECTS_COMPAT))
	@mkdir -p $(BIN)

# Main target, it creates the folders needed by the build and launches 'all' target
.PHONY: server
server: createdirs
	@echo "Beginning build..."
	@echo ""
	@make _server --no-print-directory
	@echo ""
	@echo "Build successful"

.PHONY: client
client: createdirs
	@echo "Beginning build..."
	@echo ""
	@make _client --no-print-directory
	@echo ""
	@echo "Build successful"

.PHONY: _server
_server: $(BIN)/$(SRV_EXEC_NAME)

# Links the object files into an executable
$(BIN)/$(SRV_EXEC_NAME): $(OBJECTS_COMPAT) $(OBJECTS_COMMON) $(OBJ_SERVER) $(STATICLIBS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $(OBJECTS_COMPAT) $(OBJECTS_COMMON) $(OBJ_SERVER) $(LDFLAGS) -o $@ $(LIBS_PATH) $(LIBS)

.PHONY: _client
_client: $(BIN)/$(CLNT_EXEC_NAME)

# Links the object files into an executable
$(BIN)/$(CLNT_EXEC_NAME): $(OBJECTS_COMPAT) $(OBJECTS_COMMON) $(OBJ_CLIENT) $(STATICLIBS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $(OBJECTS_COMPAT) $(OBJECTS_COMMON) $(OBJ_CLIENT) $(LDFLAGS) -o $@ $(LIBS_PATH) $(LIBS)

# Rules for the source files. It compiles source files if obj files are outdated.
# It also creates header dependency files (.d files) used to add headers as
# dependencies for the object files with -include later.
$(BUILD)/main/server/%.o: $(SRC_MAIN)/server/%.$(SRC_EXT)
	@echo "[CC] Compiling server $< -> $@"
	@$(CC) -c $(CFLAGS) $(INCLUDES) $(INCLUDES_SERVER) -MP -MMD $< -o $@
	
$(BUILD)/main/client/%.o: $(SRC_MAIN)/client/%.$(SRC_EXT)
	@echo "[CC] Compiling client $< -> $@"
	@$(CC) -c $(CFLAGS) $(INCLUDES) $(INCLUDES_CLIENT) -MP -MMD $< -o $@
	
$(BUILD)/%.o: $(SRC)/%.$(SRC_EXT)
	@echo "[CC] Compiling $< -> $@"
	@$(CC) -c $(CFLAGS) $(INCLUDES) -MP -MMD $< -o $@

# Header dependencies. Adds the rules in the .d files, if they exists, in order to
# add headers as dependencies of obj files (see .d files in BUILD for more info).
# This rules will be merged with the previous rules.
-include $(DEPEND_COMMON)
-include $(DEPEND_COMPAT)
-include $(DEPEND)

# Removes all the build directories with obj files and executable
.PHONY: clean
clean:
	@echo "Deleting directories..."
	@rm -rf $(BUILD) $(BIN)

.PHONY: cleanServer
cleanServer:
	@echo "Deleting directories..."
	@rm -rf $(BUILD)/main/server $(BIN)/main/server

.PHONY: cleanClient
cleanClient:
	@echo "Deleting directories..."
	@rm -rf $(BUILD)/main/client $(BIN)/main/client

# Copyright (C) 2016 Fabrizio Pietrucci

# This program is free software: you can redistribute it and/or
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
