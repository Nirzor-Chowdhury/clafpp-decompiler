### Key Design Principles

- **Modularity**: Each phase operates independently for easy debugging and extension
- **Graceful Degradation**: Core functionality works without AI subsystem
- **Offline-First**: No cloud APIs or internet requirements for base operation
- **Extensibility**: Plugin-ready architecture for future enhancements

---

## Performance

Measured on Intel Core i5 with 16GB RAM:

| Operation | Time (seconds) | Model |
|-----------|---------------|-------|
| Function Documentation | 20-30 | deepseek-coder:6.7b |
| Variable Suggestions | 60-90 | deepseek-coder:6.7b |
| Complexity Analysis | 15-25 | deepseek-coder:6.7b |
| Line Comments (50 lines) | 120-180 | deepseek-coder:6.7b |

**Accuracy Metrics (20 functions tested):**
- Function documentation relevance: 90%
- Variable naming semantic correctness: 85%
- Complexity calculation accuracy: 95% (±2)
- Inline comment usefulness: 88%

---

## Documentation

- [Installation Guide](docs/INSTALLATION.md)
- [User Manual](docs/USER_GUIDE.md)
- [API Documentation](docs/API.md)
- [LLM Integration Guide](docs/LLM_SETUP.md)
- [Development Guide](docs/DEVELOPMENT.md)

---

## Research & Publications

This project is part of academic research in binary analysis and AI-assisted reverse engineering. If you use CLAF++ in your research, please cite:

```bibtex
@misc{clafpp2026,
  title={CLAF++: AI-Enhanced Binary Decompilation Framework},
  author={Nirzor Chowdhury and Abhijeet},
  year={2026},
  institution={Sikkim Manipal Institute of Technology}
}
```

---

## Roadmap

### Current Focus (v1.0)
- [x] Core decompilation pipeline (6 phases)
- [x] Local LLM integration via Ollama
- [x] wxWidgets GUI implementation
- [x] Windows PE support
- [x] Linux ELF support
- [x] AI-powered semantic enhancement

### Planned Features (v2.0)
- [ ] Full x86/x64 instruction coverage
- [ ] ARM architecture support
- [ ] SSA (Static Single Assignment) transformation
- [ ] Advanced type inference engine
- [ ] Interactive CFG/DFG visualization
- [ ] Binary diffing capabilities
- [ ] Plugin architecture
- [ ] Symbolic execution integration
- [ ] Malware detection heuristics

---

## Contributing

We welcome contributions from the community! Here's how you can help:

### Development Setup

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Contribution Areas

- 🐛 Bug fixes and issue reports
- ✨ New features and enhancements
- 📝 Documentation improvements
- 🧪 Test coverage expansion
- 🌐 Localization and translations
- 🎨 UI/UX improvements

### Code Style

- Follow existing code formatting
- Include comments for complex logic
- Add unit tests for new features
- Update documentation accordingly

---

## Known Limitations

- **Information Loss**: Original variable names and comments are not recoverable
- **Type Inference**: Complex data structures may show generic types
- **Obfuscation**: Heavily obfuscated code remains challenging
- **Instruction Coverage**: Some rare x86 instructions not yet supported
- **LLM Dependency**: AI features require local Ollama installation

---

## Troubleshooting

### LLM Connection Failed
```bash
# Check if Ollama is running
curl http://localhost:11434/api/tags

# If not, start the service
ollama serve
```

### Build Errors (Windows)
- Ensure vcpkg is integrated: `vcpkg integrate install`
- Verify Visual Studio C++ workload is installed
- Check wxWidgets paths in project properties

### Build Errors (Linux)
- Update wxWidgets: `sudo apt install libwxgtk3.2-dev`
- Install missing dependencies: `sudo apt install build-essential`

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.