## Installation

### Prerequisites

**Windows:**
- Visual Studio 2022 or newer
- wxWidgets 3.2+
- vcpkg (for dependency management)
- Optional: Ollama (for AI features)

**Linux:**
- GCC/G++ compiler
- wxWidgets 3.2+ with GTK bindings
- Make
- Optional: Ollama (for AI features)

### Quick Start

#### Windows

```batch
# Clone the repository
git clone https://github.com/Nirzor-Chowdhury/clafpp-decompiler.git
cd clafpp-decompiler

# Install dependencies via vcpkg
vcpkg install curl[ssl]:x64-windows nlohmann-json:x64-windows wxwidgets:x64-windows
vcpkg integrate install

# Open and build the solution
start jdc.sln
```

Build using Visual Studio (Ctrl+Shift+B) with **Release|x64** configuration.

#### Linux

```bash
# Install system dependencies
sudo apt update
sudo apt install build-essential libwxgtk3.2-dev libcurl4-openssl-dev

# Clone the repository
git clone https://github.com/Nirzor-Chowdhury/clafpp-decompiler.git
cd clafpp-decompiler

# Build the GUI version
make jdc-gui

# Run the application
./bin/linux/x64/gui/jdc
```