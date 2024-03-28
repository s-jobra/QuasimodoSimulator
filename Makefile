SRC_DIR:=src
OBJ_DIR:=obj
BIN_DIR:=.
QUASIMODO_DIR:=Quasimodo

SRCS:=$(wildcard $(SRC_DIR)/*.cpp)
EXEC:=$(BIN_DIR)/QuasimodoSim
OBJS:=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

CXX:=g++
CXXFLAGS:=-g -O2 -std=c++2a
LDFLAGS:=-L$(QUASIMODO_DIR) -lquasimodo -Wl,-rpath=./$(QUASIMODO_DIR)
INC_DIRS:=-I $(QUASIMODO_DIR)

.DEFAULT : all
.PHONY : clean

all: $(OBJS) | $(BIN_DIR)
	$(CXX) $(INC_DIRS) $(CXXFLAGS) -o $(EXEC) $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(INC_DIRS) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

-include $(OBJ:.o=.d)

# CLEAN:
clean:
	rm -rf $(EXEC) $(OBJ_DIR)