# Student Expense & Task Tracker

A full-stack, three-layer college project built with **Modern C++20**, **SQLite3**, and a responsive **Web Frontend**. Designed to help students monitor both academic assignments/milestones and living/college expenses with complete CRUD workflows and summary metrics.

---

## Architecture Overview

The system strictly follows a clean three-layer architecture:

```
[ Frontend: Web Interface (HTML5 / Modern CSS / JavaScript) ]
                           │
                 HTTP / JSON (REST API)
                           │
┌──────────────────────────▼──────────────────────────┐
│              Backend Layer (C++20)                  │
│                                                     │
│   ├── Controllers: HTTP routes & JSON envelopes     │
│   ├── Services: Validation & business logic         │
│   └── Repository: SQLite3 parameterized queries     │
└──────────────────────────┬──────────────────────────┘
                           │
                 Native C API (sqlite3)
                           │
┌──────────────────────────▼──────────────────────────┐
│      Persistent Database Storage (SQLite3)          │
│               `data/tracker.db`                     │
└─────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
├── .gitignore                    # C++ binaries, SQLite DBs, node_modules, IDE temp
├── CMakeLists.txt                # Cross-platform CMake configuration (C++20)
├── README.md                     # Project overview and setup documentation
├── backend/
│   ├── include/
│   │   ├── controllers/          # Route handlers & request validation
│   │   ├── db/                   # SQLite connection & schema initialization
│   │   ├── models/               # Domain structs & JSON serialization
│   │   ├── repository/           # Parameterized SQLite data access
│   │   └── services/             # Business logic & summary calculations
│   ├── src/
│   │   ├── controllers/
│   │   ├── db/
│   │   ├── repository/
│   │   ├── services/
│   │   └── main.cpp              # HTTP server entry point (port 8080)
│   └── third_party/              # Header-only dependencies (httplib, json, sqlite3)
├── data/
│   ├── .gitkeep
│   └── schema.sql                # SQLite table DDL, triggers, and seed data
├── docs/
│   ├── adr/
│   │   └── 0001-three-layer-architecture.md  # Architecture Decision Record
│   ├── api-contract.md           # REST JSON API specification
│   └── setup-guide.md            # Dependency wiring and build guide
├── frontend/
│   ├── assets/                   # Images, icons, and fonts
│   ├── css/                      # Responsive layout styling
│   └── js/                       # Client-side API interactions
└── scripts/
    ├── build.ps1                 # Turnkey Windows C++20 compilation script
    └── setup_dependencies.ps1    # Automated vendor header downloader
```

---

## API Summary Contract

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/api/health` | Server health check and SQLite connection status |
| `GET` | `/api/items` | List expenses and tasks with filter options |
| `POST` | `/api/items` | Create a new task or expense item |
| `PUT` | `/api/items/{id}` | Update existing item by ID |
| `DELETE` | `/api/items/{id}` | Delete item by ID |
| `GET` | `/api/items/summary` | Aggregate dashboard statistics (expenses & tasks) |

See [docs/api-contract.md](file:///c:/Users/mauli/OneDrive/Documents/cpp project/docs/api-contract.md) for complete payload schemas and error codes.

---

## Running the Project End-to-End

1. **Clone repository:**
   ```bash
   git clone https://github.com/m4ulikP/cpp-sqlite-tracker.git
   cd cpp-sqlite-tracker
   ```

2. **Download third-party header libraries (one-time setup):**
   ```powershell
   .\scripts\setup_dependencies.ps1
   ```

3. **Compile Backend (C++20):**
   - **Using Turnkey PowerShell Script:**
     ```powershell
     .\scripts\build.ps1
     ```
   - **Or using CMake:**
     ```bash
     cmake -B build -S .
     cmake --build build --config Release
     ```

4. **Launch Server:**
   ```powershell
   .\build\bin\tracker_server.exe
   ```

5. **Open Frontend in Browser:**
   - **Direct via C++ Backend:** Open [http://localhost:8080](http://localhost:8080)
   - **Or serve via Python:**
     ```powershell
     python -m http.server 3000 --directory frontend
     ```
     Then open [http://localhost:3000](http://localhost:3000) (CORS enabled).

6. **Run Automated Test Suites:**
   - **Unit / Database Smoke Tests:** `.\build\bin\tracker_server.exe --test`
   - **Live REST API Tests:** `powershell -File .\scripts\test_api.ps1`
   - **Full End-to-End Tests:** `powershell -File .\scripts\verify_e2e.ps1`

---

## Technical Highlights for College Presentation / Viva

- **Modern C++20 Standard:** Utilizes `<filesystem>`, structured bindings, `std::optional`, and designated initializers.
- **Strict RAII (Resource Acquisition Is Initialization):**
  - `DatabaseManager`: Ensures `sqlite3_close_v2` is guaranteed on destruction. Copying is deleted; move semantics transfer ownership.
  - `ScopedStatement`: RAII wrapper around `sqlite3_stmt*` to guarantee `sqlite3_finalize` is always invoked, eliminating statement/memory leaks.
- **SQL Injection Prevention:**
  - Zero SQL string concatenation. All dynamic filtering in `findAll` uses positional parameter placeholders (`?`) and `sqlite3_bind_*`.
- **Domain Validation & Result Pattern:**
  - `ItemService` validates inputs before any database query is executed, returning strongly-typed `ServiceResult<T>` with distinct error codes (`INVALID_INPUT`, `NOT_FOUND`, `DB_ERROR`).
- **REST Envelopes & CORS:**
  - Clean JSON envelope `{ "success": true/false, "data": ..., "error": ... }` matching production standards. Full preflight `OPTIONS` and CORS headers support.
- **Dual Serving:**
  - C++ `cpp-httplib` server directly mounts `./frontend`, serving the single-page application and the REST API simultaneously on port 8080.
