# Installation Guide

Complete installation instructions for CLAF++ Decompiler on Windows and Linux.

---

## Table of Contents

- [System Requirements](#system-requirements)
- [Windows Installation](#windows-installation)
- [Linux Installation](#linux-installation)
- [Verify Installation](#verify-installation)
- [Troubleshooting](#troubleshooting)

---

## System Requirements

### Minimum Requirements

- **OS**: Windows 10/11 (64-bit) or Linux (Ubuntu 20.04+, Debian 11+)
- **RAM**: 4 GB (8 GB recommended)
- **Storage**: 2 GB free space
- **Architecture**: x86-64 processor

### Development Requirements

**Windows:**
- Visual Studio 2019 or newer (Community Edition is free)
- wxWidgets 3.2 or later
- vcpkg package manager
- Git for Windows

**Linux:**
- GCC 7.0+ or Clang 6.0+
- GNU Make
- wxWidgets 3.2+ with GTK3 bindings
- libcurl with SSL support
- Git

---

## Windows Installation

### Step 1: Install Visual Studio

1. Download [Visual Studio 2022 Community](https://visualstudio.microsoft.com/downloads/) (free)
2. Run the installer
3. Select **"Desktop development with C++"** workload
4. Ensure these components are checked:
   - MSVC v142 or newer C++ build tools
   - Windows 10/11 SDK
   - C++ CMake tools
5. Click **Install** (requires ~7 GB)

### Step 2: Install vcpkg

Open **PowerShell** or **Command Prompt**:

```batch
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

Add vcpkg to PATH (optional but recommended):
```batch
setx PATH "%PATH%;C:\vcpkg"
```

### Step 3: Install Dependencies

```batch
cd C:\vcpkg

:: Install required libraries
vcpkg install curl[ssl]:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install wxwidgets:x64-windows

:: Integrate with Visual Studio (auto-links libraries)
vcpkg integrate install
```

**This will take 15-30 minutes** depending on your internet speed.

### Step 4: Clone CLAF++ Repository

```batch
cd D:\
git clone https://github.com/Nirzor-Chowdhury/clafpp-decompiler.git
cd clafpp-decompiler
```

### Step 5: Build the Project

**Option A: Using Visual Studio GUI**

1. Open `jdc.sln` in Visual Studio
2. Set build configuration to **Release | x64** (top toolbar)
3. Press **F7** or **Build → Build Solution**
4. Wait for compilation (2-5 minutes)

**Option B: Using Command Line**

```batch
msbuild jdc.sln /p:Configuration=Release /p:Platform=x64
```

### Step 6: Locate the Executable

After successful build, the executable will be at:
bin\windows\x64\gui\jdc.exe

Or:
x64\Release\jdc.exe

### Step 7: Run CLAF++

```batch
cd bin\windows\x64\gui
jdc.exe
```

---

## Linux Installation

### Step 1: Install System Dependencies

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    git \
    libwxgtk3.2-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev
```

**Fedora/RHEL:**

```bash
sudo dnf install -y \
    gcc-c++ \
    git \
    wxGTK-devel \
    libcurl-devel \
    json-devel
```

**Arch Linux:**

```bash
sudo pacman -S \
    base-devel \
    git \
    wxwidgets-gtk3 \
    curl \
    nlohmann-json
```

### Step 2: Clone CLAF++ Repository

```bash
cd ~
git clone https://github.com/Nirzor-Chowdhury/clafpp-decompiler.git
cd clafpp-decompiler
```

### Step 3: Build the Project

```bash
# Build GUI version
make jdc-gui

# Or build with debug symbols (for development)
make DEBUG=1 jdc-gui
```

Build time: **2-5 minutes** on modern systems.

### Step 4: Verify Build Output

Check that the executable was created:

```bash
ls -lh bin/linux/x64/gui/jdc
```

You should see a file size of approximately **2-5 MB**.

### Step 5: Run CLAF++

```bash
./bin/linux/x64/gui/jdc
```

Or create a desktop shortcut:

```bash
sudo cp bin/linux/x64/gui/jdc /usr/local/bin/
```

Now you can run it from anywhere:
```bash
jdc
```

---

## Verify Installation

### Test Basic Functionality

1. Launch CLAF++
2. Click **File → Open**
3. Select a test binary:
   - **Windows**: `C:\Windows\System32\calc.exe`
   - **Linux**: `/bin/ls`
4. You should see:
   - Function list populated in bottom panel
   - Assembly instructions in left panel
   - Status shows "File loaded successfully"

### Expected Output
✓ Binary parsed successfully
✓ Found X functions
✓ Disassembly complete
✓ Ready for analysis

---

## Troubleshooting

### Windows Issues

#### **Error: "Cannot find vcpkg"**

**Solution:**
```batch
cd C:\vcpkg
.\vcpkg integrate install
```

Restart Visual Studio after integration.

---

#### **Error: "libcurl.dll not found"**

**Solution:**
```batch
copy C:\vcpkg\installed\x64-windows\bin\libcurl.dll bin\windows\x64\gui\
copy C:\vcpkg\installed\x64-windows\bin\zlib1.dll bin\windows\x64\gui\
```

---

#### **Build Error: "C2039: 'filesystem' is not a member of 'std'"**

**Solution:**

Your compiler doesn't support C++17.

1. Open `jdc.vcxproj` in a text editor
2. Find `<LanguageStandard>`
3. Change to: `<LanguageStandard>stdcpp17</LanguageStandard>`
4. Rebuild

---

#### **Error: "wxWidgets not found"**

**Solution:**
```batch
vcpkg install wxwidgets:x64-windows
vcpkg integrate install
```

---

### Linux Issues

#### **Error: "wx-config not found"**

**Solution:**
```bash
# Ubuntu/Debian
sudo apt install libwxgtk3.2-dev

# Verify installation
wx-config --version
```

---

#### **Error: "cannot find -lcurl"**

**Solution:**
```bash
sudo apt install libcurl4-openssl-dev
```

---

#### **Error: "Makefile:X: recipe for target 'jdc-gui' failed"**

**Solution:**

Check which dependency is missing:
```bash
# Check for wxWidgets
pkg-config --modversion wxwidgets

# Check for curl
pkg-config --modversion libcurl

# Install missing dependencies
sudo apt install libwxgtk3.2-dev libcurl4-openssl-dev
```

---

#### **Segmentation Fault on Launch**

**Solution:**

This may be due to wxWidgets version mismatch. Rebuild wxWidgets:

```bash
# Download wxWidgets 3.2.4
wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.4/wxWidgets-3.2.4.tar.bz2
tar xf wxWidgets-3.2.4.tar.bz2
cd wxWidgets-3.2.4

# Build
mkdir gtk-build
cd gtk-build
../configure --with-gtk=3 --enable-unicode
make -j$(nproc)
sudo make install
sudo ldconfig
```

Update Makefile to point to this installation.

---

## Next Steps

After successful installation:

1. **[Read the User Guide](USER_GUIDE.md)** - Learn how to use CLAF++
2. **[Set up LLM Features](LLM_SETUP.md)** - Enable AI-powered enhancements (optional)
3. **Test with sample binaries** - Try decompiling known executables

---

## Getting Help

If you encounter issues not covered here:

1. Check [GitHub Issues](https://github.com/Nirzor-Chowdhury/clafpp-decompiler/issues)
2. Search existing discussions
3. Create a new issue with:
   - Your OS and version
   - Build error messages (full output)
   - Steps you've already tried

---

**Installation complete!** You're ready to start using CLAF++. 🎉
