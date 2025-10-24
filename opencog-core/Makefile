# OpenCog Distributed OS Makefile
# Build system for C/C++/Yacc/Lex/ASM components

# Compiler settings
CC = gcc
CXX = g++
ASM = nasm
YACC = bison
LEX = flex

# Compiler flags
CFLAGS = -Wall -Wextra -O3 -march=native -pthread -fPIC -I./include
CXXFLAGS = -Wall -Wextra -O3 -march=native -pthread -fPIC -std=c++17 -I./include
ASMFLAGS = -f elf64 -g
YACCFLAGS = -d -v
LEXFLAGS = 

# Linker flags
LDFLAGS = -pthread -lrt

# Directories
SRC_DIR = src
INC_DIR = include
LIB_DIR = lib
PARSER_DIR = parsers
ASM_DIR = asm
TEST_DIR = tests
BUILD_DIR = build

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
CXX_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
ASM_SOURCES = $(wildcard $(ASM_DIR)/*.asm)
YACC_SOURCES = $(wildcard $(PARSER_DIR)/*.y)
LEX_SOURCES = $(wildcard $(PARSER_DIR)/*.l)

# Object files
C_OBJECTS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CXX_OBJECTS = $(CXX_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
ASM_OBJECTS = $(ASM_SOURCES:$(ASM_DIR)/%.asm=$(BUILD_DIR)/%.o)
YACC_OBJECTS = $(YACC_SOURCES:$(PARSER_DIR)/%.y=$(BUILD_DIR)/%.tab.o)
LEX_OBJECTS = $(LEX_SOURCES:$(PARSER_DIR)/%.l=$(BUILD_DIR)/%.yy.o)

# Generated parser files
YACC_C_FILES = $(YACC_SOURCES:$(PARSER_DIR)/%.y=$(BUILD_DIR)/%.tab.c)
YACC_H_FILES = $(YACC_SOURCES:$(PARSER_DIR)/%.y=$(BUILD_DIR)/%.tab.h)
LEX_C_FILES = $(LEX_SOURCES:$(PARSER_DIR)/%.l=$(BUILD_DIR)/%.yy.c)

# Library
STATIC_LIB = $(LIB_DIR)/libopencog_core.a
SHARED_LIB = $(LIB_DIR)/libopencog_core.so

# Test executable
TEST_EXEC = $(BUILD_DIR)/test_opencog

# Targets
.PHONY: all clean test install

all: dirs $(STATIC_LIB) $(SHARED_LIB)

dirs:
	@mkdir -p $(BUILD_DIR) $(LIB_DIR)

# Build static library
$(STATIC_LIB): $(C_OBJECTS) $(CXX_OBJECTS) $(ASM_OBJECTS) $(YACC_OBJECTS) $(LEX_OBJECTS)
	@echo "Creating static library: $@"
	ar rcs $@ $^

# Build shared library
$(SHARED_LIB): $(C_OBJECTS) $(CXX_OBJECTS) $(ASM_OBJECTS) $(YACC_OBJECTS) $(LEX_OBJECTS)
	@echo "Creating shared library: $@"
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling C: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling C++: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Assemble ASM sources
$(BUILD_DIR)/%.o: $(ASM_DIR)/%.asm
	@echo "Assembling: $<"
	$(ASM) $(ASMFLAGS) $< -o $@

# Generate parser from Yacc
$(BUILD_DIR)/%.tab.c $(BUILD_DIR)/%.tab.h: $(PARSER_DIR)/%.y
	@echo "Generating parser: $<"
	$(YACC) $(YACCFLAGS) -o $(BUILD_DIR)/$*.tab.c $<

# Generate lexer from Lex
$(BUILD_DIR)/%.yy.c: $(PARSER_DIR)/%.l $(BUILD_DIR)/%.tab.h
	@echo "Generating lexer: $<"
	$(LEX) $(LEXFLAGS) -o $@ $<

# Compile generated parser
$(BUILD_DIR)/%.tab.o: $(BUILD_DIR)/%.tab.c
	@echo "Compiling parser: $<"
	$(CC) $(CFLAGS) -Wno-unused-function -c $< -o $@

# Compile generated lexer
$(BUILD_DIR)/%.yy.o: $(BUILD_DIR)/%.yy.c
	@echo "Compiling lexer: $<"
	$(CC) $(CFLAGS) -Wno-unused-function -c $< -o $@

# Build tests
test: $(TEST_EXEC)
	@echo "Running tests..."
	./$(TEST_EXEC)

$(TEST_EXEC): $(STATIC_LIB) $(wildcard $(TEST_DIR)/*.c)
	@echo "Building test executable: $@"
	$(CC) $(CFLAGS) $(TEST_DIR)/*.c $(STATIC_LIB) -o $@ $(LDFLAGS)

# Install libraries and headers
install: all
	@echo "Installing libraries and headers..."
	install -d /usr/local/lib
	install -m 644 $(STATIC_LIB) /usr/local/lib/
	install -m 755 $(SHARED_LIB) /usr/local/lib/
	install -d /usr/local/include/opencog
	install -m 644 $(INC_DIR)/*.h /usr/local/include/opencog/
	ldconfig

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(LIB_DIR)

# Show build configuration
info:
	@echo "OpenCog Distributed OS Build Configuration"
	@echo "==========================================="
	@echo "CC:           $(CC)"
	@echo "CXX:          $(CXX)"
	@echo "ASM:          $(ASM)"
	@echo "YACC:         $(YACC)"
	@echo "LEX:          $(LEX)"
	@echo "CFLAGS:       $(CFLAGS)"
	@echo "CXXFLAGS:     $(CXXFLAGS)"
	@echo "ASMFLAGS:     $(ASMFLAGS)"
	@echo "C sources:    $(C_SOURCES)"
	@echo "CXX sources:  $(CXX_SOURCES)"
	@echo "ASM sources:  $(ASM_SOURCES)"
	@echo "YACC sources: $(YACC_SOURCES)"
	@echo "LEX sources:  $(LEX_SOURCES)"

# Performance optimized build
optimize: CFLAGS += -O3 -flto -march=native -mtune=native -funroll-loops
optimize: CXXFLAGS += -O3 -flto -march=native -mtune=native -funroll-loops
optimize: all

# Debug build
debug: CFLAGS += -g -O0 -DDEBUG
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: ASMFLAGS += -g
debug: all

# Profile-guided optimization build
pgo-generate: CFLAGS += -fprofile-generate
pgo-generate: CXXFLAGS += -fprofile-generate
pgo-generate: all

pgo-use: CFLAGS += -fprofile-use
pgo-use: CXXFLAGS += -fprofile-use
pgo-use: all
