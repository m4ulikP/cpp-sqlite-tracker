# ADR 0001: Three-Layer C++20 Backend Architecture with SQLite3 and HTTP/JSON Interface

- **Status:** Accepted
- **Date:** 2026-11-23
- **Author:** Antigravity Engineering

## Context
For a college C++ course, a full-stack application is required featuring a backend service, persistent database storage, and a user frontend connected end-to-end. The chosen domain is **"Student Expense & Task Tracker"**, designed to track academic tasks, deadlines, and student living/college expenses with complete CRUD and summary analytics.

The system must run smoothly on a local developer machine, cleanly decouple responsibilities, enforce data integrity with parameterized SQL queries, and present an intuitive HTTP/JSON interface for frontend consumption.

## Architectural Decision

We adopt a strict **Three-Layer Architecture**:

```
+-------------------------------------------------------------+
|                       Web Frontend                          |
|    Responsive UI (HTML5, Tailwind/Modern CSS, Vanilla JS)   |
|          Communicates via Fetch API over HTTP/JSON          |
+------------------------------+------------------------------+
                               |
                        HTTP / JSON (REST)
                               |
+------------------------------v------------------------------+
|                     Backend (C++20)                         |
|                                                             |
|   1. HTTP Route Handlers (Controllers)                      |
|      - Parses requests, validates headers/parameters        |
|      - Dispatches to Services                               |
|      - Formats JSON responses using standard envelope       |
|                                                             |
|   2. Service Layer (Business Logic)                         |
|      - Enforces domain validation rules (title, category)   |
|      - Computes aggregate metrics and dashboard statistics  |
|      - Coordinates repository transactions                  |
|                                                             |
|   3. Repository Layer (Data Access)                         |
|      - Interacts with SQLite3 engine                        |
|      - Exclusively uses parameterized prepared statements   |
|      - Prevents SQL injection vulnerabilities               |
+------------------------------+------------------------------+
                               |
                        Direct C API (sqlite3)
                               |
+------------------------------v------------------------------+
|                     Database Engine                         |
|       SQLite3 local storage file (`data/tracker.db`)        |
+-------------------------------------------------------------+
```

### 1. Technology Choices & Libraries
- **Language Standard:** C++20 (`-std=c++20`).
- **HTTP Server Library:** `cpp-httplib` (single header `httplib.h`), enabling non-blocking/multithreaded local HTTP server on port 8080 with CORS support for frontend clients.
- **JSON Serialization:** `nlohmann/json` (`json.hpp`), industry standard modern C++ JSON manipulation.
- **Database Engine:** `sqlite3` local database engine (`data/tracker.db`). All queries use `sqlite3_prepare_v2`, `sqlite3_bind_*`, and `sqlite3_step`.
- **Frontend Stack:** Responsive HTML5/CSS3/JavaScript SPA or React, cleanly consuming `/api/*` endpoints without hardcoded assumptions.

### 2. Error Handling & Contract Enforcement
- Centralized error response contract (`success`, `data`, `error`).
- HTTP status codes accurately reflect semantics:
  - `200 OK`: Successful retrieval/update/delete
  - `201 Created`: Successful resource creation
  - `400 Bad Request`: Input validation errors
  - `404 Not Found`: Resource non-existent
  - `500 Internal Server Error`: Unhandled database or operational faults

## Consequences
- **Positive:**
  - Clear separation of concerns simplifies testing and grading evaluations.
  - No bloated dependencies or external database daemons (like MySQL/PostgreSQL) required to run the demo.
  - Parameterized queries eliminate SQL injection risks.
  - Clean API contract enables decoupled frontend development.
- **Trade-offs:**
  - SQLite is single-writer, which is completely sufficient for local single-user student desktop/laptop usage.
