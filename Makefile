CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
DEBUG_FLAGS = -std=c++17 -Wall -Wextra -g

TARGET = ssvm
DEBUG_TARGET = ssvm_debug

SOURCES = main.cpp SSVM.cpp Repl.cpp
HEADERS = SSVM.hpp Repl.hpp
OBJECTS = $(SOURCES:.cpp=.o)
DEBUG_OBJECTS = $(SOURCES:.cpp=_debug.o)

.PHONY: all clean debug help

all: $(TARGET)

debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: $(DEBUG_TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"

$(DEBUG_TARGET): $(DEBUG_OBJECTS)
	$(CXX) $(DEBUG_FLAGS) -o $@ $^
	@echo "Debug build complete: $(DEBUG_TARGET)"

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%_debug.o: %.cpp $(HEADERS)
	$(CXX) $(DEBUG_FLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEBUG_OBJECTS) $(TARGET) $(DEBUG_TARGET)
	@echo "Clean complete"

help:
	@echo "SSVM (Simple Stack Virtual Machine) - Build Targets"
	@echo "===================================================="
	@echo "make              - Build release version"
	@echo "make debug        - Build debug version with symbols"
	@echo "make clean        - Remove all build artifacts"
	@echo "make help         - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  ./$(TARGET)           - Interactive REPL mode"
	@echo "  ./$(TARGET) --demo    - Run demo programs"
