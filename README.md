<div align="center">

# 📚 Smart Librarian

### Intelligent Document Archival & PDF Pipeline

*A production-grade C++17 desktop application for OCR-based document archival,
custom PDF generation, and intelligent full-text search.*

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)

</div>

---

## 🎯 Overview

Smart Librarian is a cross-platform desktop application that:

- **Accepts scanned document images** via drag-and-drop
- **Extracts text** using a Tesseract OCR pipeline
- **Generates searchable PDFs** via a hand-written PDF engine (no external PDF libraries)
- **Indexes documents** using a custom inverted index and Trie for sub-millisecond search
- **Archives and manages** document metadata persistently

Built as a portfolio project demonstrating systems programming, binary file format internals,
data structures, and production-grade C++ architecture.

---

## 🏗️ Architecture
---

## 🔧 Technology Stack

| Component | Technology |
|-----------|-----------|
| Language | C++17 |
| Build System | CMake 3.20+ with Ninja |
| GUI | Qt 6 |
| OCR | Tesseract 5.x C++ API |
| PDF Engine | Custom-built (raw binary serialization) |
| Search | Custom Trie + Inverted Index |
| Package Manager | vcpkg |
| Testing | GoogleTest |
| CI | GitHub Actions |

---

## 🚀 Building from Source

### Prerequisites

- CMake 3.20+
- GCC 11+ or Clang 14+
- Ninja
- vcpkg

### Build

```bash
git clone git@github.com:YOUR_USERNAME/smart-librarian.git
cd smart-librarian
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bin/SmartLibrarian
```

---

## 📖 Development Phases

- [x] **Phase 0** — Environment setup, project skeleton, build pipeline
- [ ] **Phase 1** — Core architecture, logging, configuration
- [ ] **Phase 2** — Custom PDF engine (binary serialization)
- [ ] **Phase 3** — OCR pipeline (Tesseract integration)
- [ ] **Phase 4** — Search engine (Trie + Inverted Index)
- [ ] **Phase 5** — Qt6 Desktop GUI
- [ ] **Phase 6** — Archive management + persistence
- [ ] **Phase 7** — Testing, profiling, optimization

---

## 💡 Technical Highlights

### Custom PDF Engine
Hand-written binary PDF serializer. Implements PDF object model, cross-reference tables,
stream compression, and font embedding without any PDF library dependency.
Demonstrates: binary file format internals, buffer management, byte-level serialization.

### OCR Pipeline
Integrates Tesseract C++ API with custom preprocessing (grayscale conversion, binarization,
noise filtering). Demonstrates: external C library integration, image processing, string handling.

### Search Engine
Custom Trie for prefix search + inverted index for full-text retrieval.
O(m) lookup complexity where m = query length.
Demonstrates: data structures, algorithmic complexity, memory layout optimization.

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.