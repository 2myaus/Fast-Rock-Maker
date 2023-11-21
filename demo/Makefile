# Makefile for demo.cpp

# Compiler
CXX := g++

# Source file
SOURCE := demo.cpp

# Output executable
TARGET := demo

# Compiler flags
CXXFLAGS := -pthread

# Linker flags
LDFLAGS := -L.. -lfastrm

# Rule to build the executable
$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Clean rule
clean:
	rm -f $(TARGET)

# Phony target to prevent conflicts with files named 'clean' or 'all'
.PHONY: clean
