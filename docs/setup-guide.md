# Build Setup & Dependency Integration Guide

This project is built with modern **C++20** and uses three lightweight, industry-standard C++ libraries without requiring complex packaging managers or system-level administrative installers:

1. **`cpp-httplib` (v0.18+)**: Multi-threaded, header-only HTTP/HTTPS server library.
2. **`nlohmann/json` (v3.11+)**: Header-only modern JSON library for C++.
3. **`sqlite3` (v3.45+)**: Embedded relational SQL engine (via single-file amalgamation or system library).

---

## Dependency Wiring Architecture

### 1. Header-Only Integration (`backend/third_party/`)
The two primary libraries are header-only and placed inside `backend/third_party/`:
- `backend/third_party/httplib.h` (from [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib))
- `backend/third_party/nlohmann/json.hpp` (from [nlohmann/json](https://github.com/nlohmann/json))

Both headers are exposed to all backend source files via `include_directories(backend/third_party)` in `CMakeLists.txt` or `-Ibackend/third_party` in compiler invocation scripts.

### 2. SQLite3 Database Engine Wiring
SQLite3 can be consumed in two flexible ways:
- **Option A (Bundled Amalgamation - Recommended on Windows):**  
  `backend/third_party/sqlite3.h` and `backend/third_party/sqlite3.c` are compiled directly into the binary or built as a static target (`sqlite3_bundled`). This requires zero host package installations and guarantees 100% platform portability.
- **Option B (System Package):**  
  If MSYS2 (`pacman -S mingw-w64-ucrt-x86_64-sqlite3`) or a system package is installed, `find_package(SQLite3)` links `SQLite::SQLite3`.

### 3. Windows Network Socket Dependencies
On Windows, `cpp-httplib` communicates over WinSock2. The build configuration explicitly links:
- `ws2_32.lib` (`-lws2_32`) for Windows Sockets API.
- `crypt32.lib` (`-lcrypt32`) for crypto/hash helpers.
- Threading (`-pthread` or `Threads::Threads`).

---

## Automated Setup Script

Run the automated dependency downloader in PowerShell to fetch the exact vendor headers:
```powershell
.\scripts\setup_dependencies.ps1
```
This script downloads `httplib.h`, `json.hpp`, `sqlite3.c`, and `sqlite3.h` directly into `backend/third_party/`.

---

## Build Commands

### Method A: CMake Build (Cross-platform)
```bash
cmake -B build -S .
cmake --build build --config Release
./build/bin/tracker_server
```

### Method B: Direct UCRT64 GCC Compilation (Turnkey Windows)
Using the MSYS2 UCRT64 GCC 16 toolchain (`C:\msys64\ucrt64\bin\g++.exe`):
```powershell
.\scripts\build.ps1
```
Produces `build/bin/tracker_server.exe` ready to run on `http://localhost:8080`.
