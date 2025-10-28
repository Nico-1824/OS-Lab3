# Makefile for pagingwithpr

# Compiler and flags
CXX := g++

# Output executable name
TARGET := pagingwithpr

# Source files
SRCS := main.cpp pagetable.cpp vaddr_tracereader.cpp log_helpers.cpp
OBJS := $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Build target
$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS)

# Pattern rule for .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
