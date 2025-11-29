CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude -pthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Sources for the Control System (App)
APP_SRCS = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/gerenciador_dados.cpp \
	$(SRC_DIR)/eventos_sistema.cpp \
	$(SRC_DIR)/interface_caminhao.cpp \
	$(SRC_DIR)/drivers/mqtt_driver.cpp \
	$(SRC_DIR)/task_tratamento_sensores.cpp \
	$(SRC_DIR)/task_logica_comando.cpp \
	$(SRC_DIR)/task_controle_navegacao.cpp \
	$(SRC_DIR)/task_monitoramento_falhas.cpp \
	$(SRC_DIR)/utils/sleep_asynch.cpp

# Sources for the Headless Simulator
SIM_SRCS = \
	$(SRC_DIR)/simulador_headless.cpp \
	$(SRC_DIR)/simulacao_mina.cpp \
	$(SRC_DIR)/mine_generator.cpp \
	$(SRC_DIR)/server_ipc.cpp \
	$(SRC_DIR)/gerenciador_dados.cpp \
	$(SRC_DIR)/eventos_sistema.cpp \
	$(SRC_DIR)/utils/sleep_asynch.cpp

APP_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(APP_SRCS))
SIM_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SIM_SRCS))

APP_TARGET = $(BIN_DIR)/app
SIM_TARGET = $(BIN_DIR)/simulador

all: $(APP_TARGET) $(SIM_TARGET)

$(APP_TARGET): $(APP_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lncurses -lmosquitto

$(SIM_TARGET): $(SIM_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lmosquitto

# Pattern rule for objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run