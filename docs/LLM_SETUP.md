### AI Enhancement Setup (Optional)

For AI-powered features, install Ollama:

**Windows/Linux:**
```bash
# Install Ollama
curl -fsSL https://ollama.com/install.sh | sh

# Download a code-specialized model
ollama pull deepseek-coder:6.7b

# Start the service (required for AI features)
ollama serve
```

**Recommended Models:**
- `deepseek-coder:6.7b` - Best for code analysis (recommended)
- `mistral:latest` - General purpose alternative
- `llama3.2:1b` - Fastest for testing

---

## Usage

### Basic Workflow

1. **Launch CLAF++** and select a binary executable (File → Open)
2. **View Disassembly** in the left panel showing raw assembly instructions
3. **Select a Function** from the function list to analyze
4. **Review Decompiled Code** in the right panel showing pseudo-C output

### AI-Enhanced Workflow

1. Open an analyzed binary with decompiled functions visible
2. Select a function to enhance
3. Choose enhancement type from **LLM** menu:
   - **Generate Function Comment**: Creates comprehensive documentation
   - **Suggest Variable Names**: Proposes semantic identifiers
   - **Analyze Complexity**: Calculates cyclomatic complexity
   - **Generate Line Comments**: Adds inline explanations

4. Review AI-generated suggestions and apply as needed

### Configuration

Access **LLM → Settings** to configure:
- **Endpoint**: Ollama service URL (default: `http://localhost:11434`)
- **Model**: Select from available local models
- **Temperature**: Control randomness (0.7 recommended for code)
- **Max Tokens**: Response length limit (800-1000 for detailed output)
- **Timeout**: Maximum wait time (300-400s for complex functions)
