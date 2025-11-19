# important directories
SRC = src
OBJ = obj
BIN = bin
INC = include
TEST = test

# compiler settings
CXX      = g++
CC       = gcc
CXXFLAGS = -O2 -I $(INC) -gdwarf-5 -std=c++20
CFLAGS   = -O2 -I $(INC)
LDFLAGS  = -lseccomp

# main program sources
SOURCES_CPP = $(wildcard $(SRC)/*.cpp)
OBJECTS_CPP = $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SOURCES_CPP))

# test program sources
TEST_SRC = $(TEST)/main.c
TEST_OBJ = $(OBJ)/test-main.o

TARGET_MAIN = filter-wx
TARGET_TEST = test

# default: build both
build: $(BIN)/$(TARGET_MAIN) $(BIN)/$(TARGET_TEST)

# -------------------------
# main program (CPP only)
# -------------------------
$(BIN)/$(TARGET_MAIN): $(OBJECTS_CPP) | $(BIN)/
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.cpp | $(OBJ)/
	$(CXX) -c $(CXXFLAGS) -o $@ $<

# -------------------------
# test program (C only)
# -------------------------
$(BIN)/$(TARGET_TEST): $(TEST_OBJ) | $(BIN)/
	$(CC) -o $@ $^

$(OBJ)/test-main.o: $(TEST_SRC) | $(OBJ)/
	$(CC) -c $(CFLAGS) -o $@ $<

# directories
%/:
	@mkdir -p $@

clean:
	@rm -rf $(BIN) $(OBJ)

.SECONDARY:
.PHONY: build clean

