# CLAF++ Decompiler

<div align="center">

**AI-Enhanced Static Binary Analysis and Decompilation Framework**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)](https://github.com/Nirzor-Chowdhury/clafpp-decompiler)
[![Architecture](https://img.shields.io/badge/arch-x86%20%7C%20x86--64-brightgreen)](https://github.com/Nirzor-Chowdhury/clafpp-decompiler)
[![Status](https://img.shields.io/badge/status-Active%20Development-orange)](https://github.com/Nirzor-Chowdhury/clafpp-decompiler)

[Features](#features) • [Installation](#installation) • [Usage](#usage) • [Architecture](#architecture) • [Documentation](#documentation) • [Contributing](#contributing)

</div>

---

## Overview

CLAF++ (C-Like Analysis Framework++) is a research-oriented binary decompilation framework that combines traditional static analysis with modern AI-powered semantic enhancement. The system transforms compiled executables into human-readable C-like pseudo code while providing intelligent documentation, variable naming suggestions, and complexity analysis through local Large Language Model integration.

**Key Differentiators:**
- 🧠 **AI-Enhanced Analysis**: Local LLM integration via Ollama for semantic code understanding
- 🔒 **Privacy-First**: Completely offline operation with no cloud dependencies
- 🎯 **Dual Architecture**: Modular design separating core decompilation from AI enhancement
- 🖥️ **Cross-Platform**: Native support for Windows PE and Linux ELF binaries
- 📊 **Research-Ready**: Designed for malware analysis, vulnerability research, and academic study

---

## Features

### Core Decompilation Engine

- ✅ **Multi-Format Support**: PE (Windows) and ELF (Linux) executable parsing
- ✅ **x86/x86-64 Disassembly**: Comprehensive instruction decoding with opcode mapping
- ✅ **Control Flow Recovery**: Automatic function boundary detection and CFG construction
- ✅ **Structure Reconstruction**: Pattern-based identification of loops, conditionals, and branches
- ✅ **Pseudo-C Generation**: Human-readable code output with proper formatting
- ✅ **Symbol Resolution**: Import table extraction and API call identification
- ✅ **Memory Analysis**: Variable tracking and assignment reconstruction

### AI-Powered Enhancement (Optional)

- 🤖 **Function Documentation**: Automatic generation of comprehensive function comments
- 🏷️ **Smart Variable Naming**: Context-aware suggestions replacing generic identifiers
- 📈 **Complexity Analysis**: Cyclomatic complexity calculation with detailed reports
- 💬 **Inline Comments**: Selective annotation of non-trivial operations
- 🎯 **Semantic Understanding**: Deep code comprehension via DeepSeek-Coder and similar models
- ⚡ **Async Processing**: Non-blocking AI operations with progress indication

### User Interface

- 🎨 **Modern GUI**: Professional wxWidgets-based interface with dark theme
- 📋 **Multi-Panel Layout**: Simultaneous assembly and pseudo-code visualization
- 🔍 **Function Explorer**: Interactive function list with sorting and filtering
- ⚙️ **Configurable Settings**: Comprehensive LLM parameter tuning
- 📊 **Real-time Analysis**: Immediate feedback on code complexity and quality

---

## Screenshots

<div align="center">

### Main Interface
![Main GUI](screenshots/main_gui.png)

### LLM Settings Dialog
![LLM Settings](screenshots/LLM_Setting.png)

### Complexity Analysis
![Complexity Analysis](screenshots/complexity_analysis.png)

### AI-Enhanced Output
![Enhanced Code](screenshots/enhanced_output.png)