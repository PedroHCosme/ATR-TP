CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude -pthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Include files in src, src/utils, and src/drivers
SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/utils/*.cpp) $(wildcard $(SRC_DIR)/drivers/*.cpp)
# Exclude eventos_sistema.cpp and task_monitoramento_falhas.cpp from compilation
SRCS := $(filter-out $(SRC_DIR)/eventos_sistema.cpp $(SRC_DIR)/task_monitoramento_falhas.cpp, $(SRCS))
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/app

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rule that handles subdirectories
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
