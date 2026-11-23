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

## Getting Started (Phase 1)

1. **Clone repository:**
   ```bash
   git clone https://github.com/m4ulikP/cpp-sqlite-tracker.git
   cd cpp-sqlite-tracker
   ```

2. **Download third-party header libraries (one-time setup):**
   ```powershell
   .\scripts\setup_dependencies.ps1
   ```

3. **Compile Backend:**
   - **Using CMake:**
     ```bash
     cmake -B build -S .
     cmake --build build --config Release
     ```
   - **Using PowerShell with UCRT64 GCC:**
     ```powershell
     .\scripts\build.ps1
     ```
