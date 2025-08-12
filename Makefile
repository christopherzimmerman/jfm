# JFM Compiler Makefile
# Single portable executable build

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2
DEBUGFLAGS = -g -DDEBUG
TESTFLAGS = -Wall -Wextra -std=c11 -g

# Source files
SRCS = src/jfmc.c \
       src/lexer.c \
       src/parser.c \
       src/ast.c \
       src/semantic.c \
       src/symbol_table.c \
       src/type.c \
       src/error.c \
       src/codegen.c \
       src/utils.c

# Single portable executable
TARGET = jfmc

# Default target - build single executable
all: $(TARGET)

# Build compiler as a single portable executable
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Successfully built JFM compiler: $@"

# Debug build
debug: CFLAGS += $(DEBUGFLAGS)
debug: clean $(TARGET)

# Build and run tests
test: $(SRCS)
	@echo "Building and running tests..."
	@$(CC) $(TESTFLAGS) -o test_lexer.exe tests/test_lexer.c $(filter-out src/jfmc.c, $(SRCS))
	@./test_lexer.exe && rm test_lexer.exe
	@$(CC) $(TESTFLAGS) -o test_parser.exe tests/test_parser.c $(filter-out src/jfmc.c, $(SRCS))
	@./test_parser.exe && rm test_parser.exe
	@$(CC) $(TESTFLAGS) -o test_semantic.exe tests/test_semantic.c $(filter-out src/jfmc.c, $(SRCS))
	@./test_semantic.exe && rm test_semantic.exe
	@$(CC) $(TESTFLAGS) -o test_codegen.exe tests/test_codegen.c $(filter-out src/jfmc.c, $(SRCS))
	@./test_codegen.exe && rm test_codegen.exe
	@echo "All tests passed!"

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TARGET).exe
	rm -f test_*.exe
	rm -f examples/*.c examples/*.exe
	rm -f test_output.c
	rm -rf obj bin build

# Compile examples
examples: $(TARGET)
	@echo "Compiling example programs..."
	@for file in examples/*.jfm; do \
		echo "  Compiling $$file..."; \
		./$(TARGET) "$$file" || exit 1; \
	done
	@echo "All examples compiled successfully!"

# Run a specific example
run-example: $(TARGET)
	@if [ -z "$(EXAMPLE)" ]; then \
		echo "Usage: make run-example EXAMPLE=01_hello_world"; \
	else \
		./$(TARGET) examples/$(EXAMPLE).jfm && \
		./examples/$(EXAMPLE).exe; \
	fi

# Install (portable - just copy the exe)
install: $(TARGET)
	@echo "To install JFM compiler:"
	@echo "  Windows: Copy $(TARGET).exe to a folder in your PATH"
	@echo "  Unix/Mac: sudo cp $(TARGET) /usr/local/bin/"
	@echo ""
	@echo "Or just run it from the current directory: ./$(TARGET)"

# Help
help:
	@echo "JFM Compiler - Portable Build"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build the compiler (default)"
	@echo "  debug        - Build with debug symbols"
	@echo "  test         - Run all tests"
	@echo "  examples     - Compile all examples"
	@echo "  run-example  - Run a specific example (e.g., make run-example EXAMPLE=01_hello_world)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  install      - Installation instructions"
	@echo "  help         - Show this help"
	@echo ""
	@echo "The compiler is built as a single portable executable: $(TARGET) (or $(TARGET).exe on Windows)"

.PHONY: all debug test clean examples run-example install help