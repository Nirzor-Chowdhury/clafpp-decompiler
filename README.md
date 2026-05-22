# CLAF++ Decompiler: AI-Assisted C-Based Binary Decompiler for Windows and Linux

CLAF++ Decompiler is an AI-enhanced static binary analysis and decompilation framework designed to reverse engineer executable files into readable C-like source code. The project combines disassembly, control flow analysis, abstract syntax reconstruction, and optional Large Language Model (LLM) integration to improve code readability, semantic understanding, and documentation generation.

The framework supports both Windows Portable Executable (PE) files and Linux ELF binaries running on x86/x86-64 architectures. CLAF++ is designed not only as a traditional decompiler, but also as a research-oriented reverse engineering platform for malware analysis, software auditing, vulnerability research, binary inspection, and educational purposes.

The graphical user interface is developed using wxWidgets and provides an interactive environment for disassembly visualization, function analysis, decompiled code inspection, and AI-assisted code interpretation.

---

# Features

* Static binary analysis for PE and ELF executables
* x86/x86-64 disassembly engine
* C-like pseudo code generation
* Function boundary detection and control flow reconstruction
* Conditional statement and loop identification
* Variable and memory assignment reconstruction
* Import table and API resolution
* GUI-based reverse engineering environment using wxWidgets
* AI-assisted semantic analysis using local LLMs via Ollama
* Automatic function comment generation
* Intelligent variable and function naming suggestions
* Local offline AI support without cloud APIs
* Cross-platform support for Windows and Linux
* Research-oriented modular architecture

---

# Screenshot

![GUI](screenshots/main_gui.png)


---

# Installation

## Prebuilt Binaries

Precompiled binaries may be available inside the `bin/` directory.

---

# Dependencies

## Linux

Install the required wxWidgets GTK library:

```bash
sudo apt install libwxgtk3.2-1
```

You will also need:

* gcc
* g++
* make
* wxWidgets 3.2+

---

# Building the Project

## Linux

Clone the repository:

```bash
git clone <your-repository-url>
cd <repository-name>
```

Compile the GUI version:

```bash
make jdc-gui
```

The Makefile assumes a wx-config path similar to:

```bash
../wxWidgets-3.2.10/gtk-build/wx-config
```

Update the Makefile if your wxWidgets installation path differs.

---

## Windows

Use the included Visual Studio solution:

```text
jdc.sln
```

Requirements:

* Visual Studio 2022 or newer
* wxWidgets installed and configured
* MSVC compiler toolchain

Build the project using the Visual Studio solution.

---

# How It Works

CLAF++ Decompiler performs reverse engineering through several stages:

1. Binary Parsing
2. Executable Section Extraction
3. Disassembly
4. Control Flow Analysis
5. Intermediate Representation Processing
6. C-like Code Reconstruction
7. AI-Assisted Semantic Enhancement

---

# File Handling

Depending on the operating system, CLAF++ processes either:

* PE (Portable Executable) files on Windows
* ELF (Executable and Linkable Format) files on Linux

The framework performs:

* Import table extraction
* Function address resolution
* Executable section parsing
* Data section loading
* Constant and string resolution
* Symbol analysis

---

# Disassembly Engine

The disassembler reverses Intel x86/x86-64 instruction encoding using opcode maps, ModRM decoding, instruction parsing, and operand analysis.

Current capabilities include:

* One-byte opcode decoding
* Extended opcode decoding
* Register and memory operand parsing
* Stack operation analysis
* Function call decoding
* Conditional and unconditional jump handling

The disassembler is modular and designed for future instruction set expansion.

---

# Decompilation Pipeline

Before generating pseudo code, the framework analyzes all disassembled instructions to recover program structure.

The decompiler performs:

* Function boundary identification
* Basic block analysis
* Conditional jump analysis
* Loop reconstruction
* Variable tracking
* Memory assignment recovery
* Function call reconstruction
* Return statement analysis
* Control flow restructuring

The generated output is transformed into readable C-like pseudo code.

---

# AI-Assisted LLM Enhancement

CLAF++ integrates local Large Language Models using Ollama to improve semantic understanding of the generated code.

The AI enhancement layer provides:

* Automatic function documentation generation
* Variable naming suggestions
* Human-readable code explanations
* Semantic behavior analysis
* Reverse engineering assistance
* Malware behavior summarization
* Decompiled code annotation

![GUI](screenshots/LLM_Setting.png)

The system works completely offline without requiring external APIs.

---

# Project Architecture

Main modules of the framework:

```text
src/
 ├── decompiler/
 ├── disassembler/
 ├── pe-handler/
 ├── elf-handler/
 ├── file-handler/
 ├── gui/
 ├── llm/
 └── cli/
```

---

# Current Status

CLAF++ Decompiler is currently under active development.

Some components are experimental and research-oriented, including:

* Advanced control flow recovery
* Full Intel instruction coverage
* Data type inference
* Struct reconstruction
* SSA-based optimization
* Advanced AST generation
* CFG visualization improvements
* Real-world malware deobfuscation support

---

# Research Goals

This project aims to evolve into:

* A real-world industrial reverse engineering framework
* An AI-assisted binary analysis platform
* A publishable academic research project
* A malware analysis and software auditing toolkit
* A modular decompiler research environment

---

# Future Improvements

Planned enhancements include:

* Full x86/x64 instruction coverage
* ARM architecture support
* SSA (Static Single Assignment) transformation
* Advanced AST generation
* CFG and DFG visualization
* Type inference engine
* Symbolic execution integration
* Binary diffing support
* Malware detection integration
* Interactive graph visualization
* Real-time decompilation analysis
* Plugin architecture
* Advanced optimization recovery
* AI-powered vulnerability analysis

---

# License

MIT License


# Acknowledgements

* Intel Software Developer Manuals
* wxWidgets Framework
* Ollama
* Reverse engineering and compiler research communities
* Open-source binary analysis projects
