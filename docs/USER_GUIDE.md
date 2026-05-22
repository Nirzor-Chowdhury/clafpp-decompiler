# CLAF++ User Guide

Complete guide to using CLAF++ Decompiler for binary analysis and reverse engineering.

---

## Table of Contents

- [Getting Started](#getting-started)
- [User Interface Overview](#user-interface-overview)
- [Basic Workflow](#basic-workflow)
- [AI-Enhanced Workflow](#ai-enhanced-workflow)
- [Advanced Features](#advanced-features)
- [Best Practices](#best-practices)
- [Tips and Tricks](#tips-and-tricks)
- [Troubleshooting](#troubleshooting)

---

## Getting Started

### Prerequisites

Before using CLAF++, ensure you have:

- ✅ Installed CLAF++ ([Installation Guide](INSTALLATION.md))
- ✅ (Optional) Set up Ollama for AI features ([LLM Setup Guide](LLM_SETUP.md))
- ✅ A binary executable to analyze

### Supported File Types

- **Windows**: `.exe`, `.dll`, `.sys` (PE format)
- **Linux**: ELF executables and shared libraries

---

## User Interface Overview

### Main Window Layout

┌─────────────────────────────────────────────────────────────┐
│ File  Tools  LLM  Options  Help                             │
├──────────────────┬──────────────────────────────────────────┤
│                  │                                          │
│  Assembly View   │      Decompiled Code View                │
│  (Left Panel)    │      (Right Panel)                       │
│                  │                                          │
│  • Address       │  • C-like pseudo code                    │
│  • Instruction   │  • Variable declarations                 │
│  • Operands      │  • Control flow structures               │
│                  │  • Function calls                        │
│                  │                                          │
├──────────────────┴──────────────────────────────────────────┤
│                                                              │
│           Function List (Bottom Panel)                       │
│           • Address  • Name  • Instruction Count             │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│ Status: Ready                                                │
└──────────────────────────────────────────────────────────────┘

### Menu Bar

**File Menu:**
- **Open** - Load binary executable
- **Save** - Save decompiled output
- **Export** - Export analysis results
- **Exit** - Close application

**Tools Menu:**
- **Imports Viewer** - Display imported functions
- **Exports Viewer** - Display exported functions
- **Strings** - Extract string constants

**LLM Menu:**
- **Generate Function Comment** - AI documentation
- **Suggest Variable Names** - AI naming suggestions
- **Analyze Complexity** - Cyclomatic complexity
- **Generate Line Comments** - Inline explanations
- **LLM Settings...** - Configure AI parameters

**Options Menu:**
- **Syntax Highlighting** - Toggle color coding
- **Dark Mode** - Toggle theme
- **Preferences** - General settings

---

## Basic Workflow

### Step 1: Load a Binary

1. Click **File → Open** (or press `Ctrl+O`)
2. Navigate to your binary file
3. Select the file and click **Open**

**Recommended test binaries:**

- **Windows**: `C:\Windows\System32\calc.exe`
- **Linux**: `/bin/ls` or `/usr/bin/grep`

**What happens:**
- Binary structure is parsed
- Executable sections identified
- Import table extracted
- Functions discovered

Status bar shows: `"File loaded successfully. Found X functions."`

---

### Step 2: View Disassembly

After loading, the **left panel** displays assembly code:
140001000:  PUSH RBP
140001001:  MOV RBP, RSP
140001004:  SUB RSP, 0x20
140001008:  MOV [RBP+0x10], RCX
14000100C:  CALL 0x140002000

**Understanding the display:**

- **Column 1**: Memory address (hexadecimal)
- **Column 2**: Instruction mnemonic (e.g., PUSH, MOV, CALL)
- **Column 3**: Operands (registers, memory locations)

---

### Step 3: Select a Function

In the **bottom panel**, you'll see a function list:

| Address | Calling Convention | Name | Instructions |
|---------|-------------------|------|-------------|
| 140001000 | __cdecl | func1000 | 45 |
| 140001470 | __fastcall | func1470 | 13 |
| 1400014C0 | __cdecl | func14C0 | 157 |

**Click any function** to analyze it.

---

### Step 4: View Decompiled Code

The **right panel** shows C-like pseudo code:

```cpp
int __cdecl func14C0()
{
    long long var74;
    int var98;
    long long var78;
    
    var74 = GetStartupInfoW(&var78);
    
    if (var98 == 0)
    {
        return 0;
    }
    
    exit(var98);
    return var98;
}
```

---

### Step 5: Analyze and Save

**Save decompiled output:**

1. Click **File → Save**
2. Choose output format (`.c` or `.txt`)
3. Select location
4. Click **Save**

---

## AI-Enhanced Workflow

### Prerequisites

- Ollama must be running (`ollama serve`)
- At least one model downloaded (`ollama pull deepseek-coder:6.7b`)
- LLM configured in CLAF++ (**LLM → Settings**)

---

### Feature 1: Generate Function Comment

**Purpose:** Automatically create comprehensive function documentation.

**Steps:**

1. Load a binary and select a function
2. Click **LLM → Generate Function Comment**
3. Wait 20-30 seconds (progress dialog shown)
4. Documentation comment appears at top of code

**Before:**
```cpp
int __cdecl func14C0()
{
    long long var74;
    // ... code
}
```

**After:**
```cpp
/**
 * Function performs CRT initialization including startup info 
 * gathering, memory management, and exit code handling.
 * Uses GetStartupInfoW and various exit handlers.
 */
int __cdecl func14C0()
{
    long long var74;
    // ... code
}
```

**When to use:**
- Understanding unfamiliar binaries
- Documenting malware behavior
- Creating analysis reports
- Legacy code reverse engineering

---

### Feature 2: Suggest Variable Names

**Purpose:** Replace generic names with semantically meaningful identifiers.

**Steps:**

1. Select a function
2. Click **LLM → Suggest Variable Names**
3. Wait 60-90 seconds
4. Suggestions appear in comment block

**Output format:**
```cpp
// --- Suggested variable renames ---
//   var74  ->  startup_info
//   var98  ->  exit_code
//   var78  ->  startup_info_buffer
//   var88  ->  memory_address
// ----------------------------------
```

**How to apply suggestions:**

Currently, suggestions are displayed as comments. You can:
1. Manually rename in your editor
2. Use find-and-replace (`Ctrl+H`)
3. Export and process with scripts

**Accuracy:** ~85% semantic correctness based on testing.

**When to use:**
- Complex functions with many variables
- Preparing code for presentations
- Making code more readable for team review

---

### Feature 3: Analyze Complexity

**Purpose:** Calculate cyclomatic complexity and identify complexity sources.

**Steps:**

1. Select a function
2. Click **LLM → Analyze Complexity**
3. Wait 15-25 seconds
4. Complexity report dialog appears

**Example report:**
Cyclomatic Complexity: 23 (Moderate-to-Complex)
Rating: Moderately Complex
Contributing factors:

Multiple conditional statements (if-else chains)
Nested loops increasing control flow paths
Indirect function calls (GetStartupInfoW, Sleep, exit)
Direct memory manipulation via RSP offsets
Complex error handling logic

Recommendation: Consider refactoring into smaller helper
functions for improved testability and maintenance.

**Understanding complexity scores:**

| Score | Rating | Meaning |
|-------|--------|---------|
| 1-10 | Low | Simple, easy to test |
| 11-20 | Moderate | Acceptable complexity |
| 21-50 | High | Consider refactoring |
| 50+ | Very High | Difficult to maintain |

**When to use:**
- Prioritizing code review efforts
- Estimating testing effort
- Identifying refactoring candidates
- Security audit planning

---

### Feature 4: Generate Line Comments

**Purpose:** Add explanatory comments to individual code lines.

**Steps:**

1. Select a function
2. Click **LLM → Generate Line Comments**
3. Wait 2-3 minutes for larger functions
4. Inline comments appear throughout code

**Before:**
```cpp
var74 = GetStartupInfoW(&var78);
if (var98 == 0)
{
    return 0;
}
```

**After:**
```cpp
var74 = GetStartupInfoW(&var78);  // Retrieve process startup parameters
if (var98 == 0)  // Check if exit code is zero
{
    return 0;  // Return success status
}
```

**Note:** Only non-trivial lines receive comments to avoid clutter.

**When to use:**
- Educational purposes (explaining code to students)
- Detailed malware analysis reports
- Documentation for complex algorithms

---

## Advanced Features

### Import Table Viewer

**Access:** **Tools → Imports**

Shows all external functions the binary imports:

| DLL | Function | Address |
|-----|----------|---------|
| kernel32.dll | GetStartupInfoW | 0x140003000 |
| kernel32.dll | ExitProcess | 0x140003008 |
| ntdll.dll | RtlExitUserThread | 0x140003010 |

**Use cases:**
- Identifying capabilities (networking, file I/O, registry)
- Detecting suspicious imports (e.g., keylogger APIs)
- Understanding dependencies

---

### String Extraction

**Access:** **Tools → Strings**

Extracts all readable strings from the binary:
"Windows Calculator"
"calc.exe"
"Failed to initialize"
"C:\Windows\System32"

**Use cases:**
- Finding error messages
- Locating URLs or IP addresses
- Identifying hardcoded credentials
- Malware C2 server detection

---

### Export Viewer

**Access:** **Tools → Exports**

Shows functions exported by DLLs:

| Ordinal | Name | Address |
|---------|------|---------|
| 1 | DllMain | 0x140001000 |
| 2 | ExportedFunction | 0x140001500 |

---

## Best Practices

### 1. Start with Small Binaries

For learning, begin with:
- Simple command-line tools
- Calculator applications
- "Hello World" programs

Avoid initially:
- Large GUI applications
- Heavily obfuscated malware
- Stripped binaries (no symbols)

---

### 2. Combine Manual + AI Analysis

**Workflow:**

1. **Manual review** - Understand overall structure
2. **AI documentation** - Get high-level description
3. **Manual verification** - Confirm AI findings
4. **AI variable naming** - Improve readability
5. **Final manual pass** - Verify correctness

**Never trust AI blindly!** Always verify suggestions.

---

### 3. Use Complexity Analysis to Prioritize

**Strategy:**

1. Run complexity analysis on all functions
2. Sort by complexity score
3. Focus on high-complexity functions first (security-critical)
4. Low-complexity = likely utility functions (less interesting)

---

### 4. Save Incremental Progress

**Best practice:**
Project/
├── binary_original.exe
├── analysis/
│   ├── func1000_decompiled.c
│   ├── func1470_decompiled.c
│   └── func14C0_decompiled.c
├── notes/
│   └── analysis_notes.md
└── reports/
└── final_report.pdf

Save each analyzed function separately.

---

### 5. Document Your Findings

Create an analysis report structure:

```markdown
# Binary Analysis Report

## Executive Summary
- Binary name: calc.exe
- File type: PE executable
- Architecture: x86-64
- Compiler: MSVC 2019

## Key Findings
1. Function func1000: Entry point initialization
2. Function func14C0: CRT setup (complexity: 23)
3. Suspicious: Calls to undocumented APIs

## Security Assessment
- No obvious malicious behavior detected
- Uses standard Windows APIs
- No network communication observed

## Recommendations
- Low risk for deployment
- No further analysis required
```

---

## Tips and Tricks

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open file |
| `Ctrl+S` | Save output |
| `Ctrl+F` | Find text |
| `F5` | Refresh view |
| `Ctrl+G` | Go to address |

---

### Performance Optimization

**For large binaries (>10 MB):**

1. **Disable real-time features** during initial load
2. **Analyze functions selectively** (not all at once)
3. **Use faster LLM models** for quick scans (`llama3.2:1b`)
4. **Close unused panels** to save RAM

---

### Handling Obfuscated Code

**Strategies:**

1. **Start with string analysis** - Often reveals purpose
2. **Check imports** - Indicates capabilities
3. **Use complexity analysis** - High complexity = possible obfuscation
4. **AI can sometimes help** - Pattern recognition in obfuscated loops

---

### Malware Analysis Safety

**Critical precautions:**

1. **Use a VM or sandbox** - Never analyze malware on host OS
2. **Disconnect network** - Prevent C2 communication
3. **Take snapshots** - Before loading suspicious files
4. **Use isolated environment** - Air-gapped analysis machine

**CLAF++ is static analysis only** - Safe for malware analysis as it doesn't execute code.

---

## Troubleshooting

### Issue: "Failed to load binary"

**Cause:** Unsupported file format or corrupted file.

**Solution:**

1. Verify file type:
```bash
   file suspicious.exe
```

2. Check if it's a valid PE/ELF:
```bash
   # Linux
   readelf -h binary_file
   
   # Windows
   dumpbin /headers binary_file.exe
```

3. Try a different binary to confirm CLAF++ works

---

### Issue: Decompiled code looks nonsensical

**Causes:**
- Compiler optimizations
- Obfuscation
- Anti-reversing techniques

**Solutions:**

1. **Compare with disassembly** - Verify logic matches
2. **Use AI analysis** - May identify patterns humans miss
3. **Focus on high-level behavior** - Don't get lost in details
4. **Check for packing** - Unpacking may be required

---

### Issue: LLM features not responding

See [LLM Setup Guide - Troubleshooting](LLM_SETUP.md#troubleshooting)

---

### Issue: Application crashes on large files

**Solution:**

1. **Increase system RAM** if possible
2. **Close other applications**
3. **Analyze in sections** - Focus on specific functions
4. **Report the issue** on GitHub with:
   - Binary size
   - System specs
   - Crash log

---

## Performance Benchmarks

Measured on Intel Core i5-10400, 16GB RAM:

### Core Decompilation

| Binary Size | Load Time | Function Count | Analysis Time |
|-------------|-----------|----------------|---------------|
| 100 KB | 1-2s | 50 | 5-10s |
| 1 MB | 3-5s | 200 | 15-30s |
| 10 MB | 10-15s | 800 | 60-120s |
| 50 MB+ | 30-60s | 2000+ | 5-10 min |

### AI Enhancement (per function)

| Model | Documentation | Variable Names | Complexity |
|-------|--------------|----------------|------------|
| deepseek-coder:6.7b | 20-30s | 60-90s | 15-25s |
| mistral:latest | 25-35s | 70-100s | 20-30s |
| llama3.2:1b | 5-10s | 20-30s | 5-8s |

---

## Accuracy Metrics

Based on testing with 20 known binaries:

| Feature | Accuracy | Notes |
|---------|----------|-------|
| Function Documentation | 90% | Occasionally generic on highly optimized code |
| Variable Naming | 85% | Very good for API-heavy code, fair for pure math |
| Complexity Analysis | 95% | ±2 from manual calculation |
| Inline Comments | 88% | Selective, avoids obvious statements |

---

## Next Steps

**After mastering the basics:**

1. **Try real-world analysis** - Analyze your own compiled programs
2. **Contribute to project** - Report bugs, suggest features
3. **Join community** - Discuss techniques and findings
4. **Explore advanced topics** - Symbolic execution, binary diffing

---

## Getting Help

**Resources:**

- [Installation Guide](INSTALLATION.md)
- [LLM Setup Guide](LLM_SETUP.md)
- [GitHub Issues](https://github.com/Nirzor-Chowdhury/clafpp-decompiler/issues)
- [Project Documentation](https://github.com/Nirzor-Chowdhury/clafpp-decompiler)

**Community:**

- Report bugs with detailed reproduction steps
- Request features with clear use cases
- Share interesting findings
- Help other users

---

**Happy reverse engineering!** 🔍
