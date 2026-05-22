# Makefile -- updated with LLM integration
#
# Replaces the existing Makefile. The LLM module needs:
#   * libcurl  (linked via `curl-config`)
#   * nlohmann/json single-header (assumes the include path is on the system,
#     or set NLOHMANN_INCLUDE=-I/path/to/json/single_include)
#   * C++17  (filesystem)
#
# The CLI target (jdc-cli) is unchanged - LLM features are GUI-only.

# ---- Toolchain knobs --------------------------------------------------------
CXXSTD       = -std=c++17
CFLAGS_COMMON= -w
WX_CONFIG    = ../wxWidgets/buildgtk/wx-config
WX_FLAGS     = `$(WX_CONFIG) --cxxflags --libs all`
CURL_FLAGS   = `curl-config --cflags`
CURL_LIBS    = `curl-config --libs`
# Override on the command line if nlohmann/json isn't on the default include
# path, e.g.   make jdc-gui NLOHMANN_INCLUDE=-I/opt/json/single_include
NLOHMANN_INCLUDE ?=

LLM_FLAGS    = $(CURL_FLAGS) $(NLOHMANN_INCLUDE) $(CXXSTD)
LLM_LIBS     = $(CURL_LIBS)

# ---- CLI target (unchanged) -------------------------------------------------
jdc-cli:
	gcc -c $(CFLAGS_COMMON) ./src/cli/main.c -o main.o
	gcc -c $(CFLAGS_COMMON) ./src/disassembler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/decompiler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/elf-handler/*.c
	gcc *.o -o ./bin/linux/x64/cli/jdc

debug-jdc-cli:
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/cli/main.c -o main.o
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/disassembler/*.c
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/decompiler/*.c
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/elf-handler/*.c
	gcc *.o -o ./bin/linux/x64/cli/debug-jdc

# ---- GUI target (now with LLM module) ---------------------------------------
jdc-gui:
	g++ -c $(CFLAGS_COMMON) $(CXXSTD) ./src/gui/*.cpp $(WX_FLAGS)
	g++ -c $(CFLAGS_COMMON) $(LLM_FLAGS) ./src/llm/*.cpp $(WX_FLAGS)
	gcc -c $(CFLAGS_COMMON) ./src/disassembler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/decompiler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/elf-handler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/file-handler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/jdc-str/*.c
	g++ -o ./bin/linux/x64/gui/jdc *.o $(WX_FLAGS) $(LLM_LIBS)

debug-jdc-gui:
	g++ -g -O0 -c $(CFLAGS_COMMON) $(CXXSTD) ./src/gui/*.cpp $(WX_FLAGS)
	g++ -g -O0 -c $(CFLAGS_COMMON) $(LLM_FLAGS) ./src/llm/*.cpp $(WX_FLAGS)
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/disassembler/*.c
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/decompiler/*.c
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/elf-handler/*.c
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/file-handler/*.c
	gcc -g -O0 -c $(CFLAGS_COMMON) ./src/jdc-str/*.c
	g++ -o ./bin/linux/x64/gui/debug-jdc *.o $(WX_FLAGS) $(LLM_LIBS)

# ---- Build the GUI WITHOUT the LLM module (for environments without
#       libcurl / nlohmann). The LLM menu items will be absent.
# Useful for verifying that the new module is properly modular. Apply the
# LLM_DISABLED preprocessor flag if you want to retain the menu code but
# stub it out.
jdc-gui-no-llm:
	g++ -c $(CFLAGS_COMMON) $(CXXSTD) -DJDC_LLM_DISABLED ./src/gui/*.cpp $(WX_FLAGS)
	gcc -c $(CFLAGS_COMMON) ./src/disassembler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/decompiler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/elf-handler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/file-handler/*.c
	gcc -c $(CFLAGS_COMMON) ./src/jdc-str/*.c
	g++ -o ./bin/linux/x64/gui/jdc-no-llm *.o $(WX_FLAGS)

clean:
	rm -f *.o
