# important directories
SRC = src
BIN = bin
INC = include

# compiler settings
CXX      = g++
CXXFLAGS = -O2 -I$(INC) -gdwarf-5 -std=c++20
LDFLAGS  =

# identify sources; create targets
SOURCES = $(wildcard $(SRC)/*.cpp)
TARGETS = $(patsubst $(SRC)/%.cpp, $(BIN)/%, $(SOURCES))

# default target
build: $(TARGETS)

%/:
	@mkdir -p $@

$(BIN)/%: $(SRC)/%.cpp | $(BIN)/
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	@rm -rf $(BIN)

.PHONY: build clean

.SECONDARY:

