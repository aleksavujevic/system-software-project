CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -Iinc -Ibuild

BUILD = build

COMMON_SRC = src/symbol_table.cpp src/section_table.cpp src/object_file.cpp

ASM_SRC = src/main_assembler.cpp src/assembler.cpp $(COMMON_SRC)
LNK_SRC = src/main_linker.cpp src/linker.cpp $(COMMON_SRC)
EMU_SRC = src/main_emulator.cpp src/emulator.cpp

ASM_GEN = $(BUILD)/bison.tab.cpp $(BUILD)/lex.yy.cc

.PHONY: all clean

all: assembler linker emulator

$(BUILD)/bison.tab.cpp $(BUILD)/bison.tab.hpp: misc/bison.y
	@mkdir -p $(BUILD)
	bison -d -v -o $(BUILD)/bison.tab.cpp misc/bison.y

$(BUILD)/lex.yy.cc: misc/flex.l $(BUILD)/bison.tab.hpp
	flex -o $(BUILD)/lex.yy.cc misc/flex.l

assembler: $(ASM_SRC) $(ASM_GEN)
	$(CXX) $(CXXFLAGS) -o assembler $(ASM_SRC) $(ASM_GEN)

linker: $(LNK_SRC)
	$(CXX) $(CXXFLAGS) -o linker $(LNK_SRC)

emulator: $(EMU_SRC)
	$(CXX) $(CXXFLAGS) -o emulator $(EMU_SRC)

clean:
	rm -rf $(BUILD) assembler linker emulator