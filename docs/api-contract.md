# Student Expense & Task Tracker - API Endpoint Contract

**Base URL:** `http://localhost:8080`  
**Content-Type:** `application/json`  
**Standard Response Envelope:**
```json
{
  "success": true,
  "data": { ... },
  "error": null
}
```
Error Response Envelope:
```json
{
  "success": false,
  "data": null,
  "error": {
    "code": "INVALID_INPUT",
    "message": "Title is required and cannot be empty."
  }
}
```

---

## 1. Health Check
- **Endpoint:** `GET /api/health`
- **Description:** Verifies server responsiveness and SQLite database connectivity.
- **Success Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "status": "healthy",
    "database": "connected",
    "version": "1.0.0",
    "timestamp": "2026-11-23T21:40:00Z"
  },
  "error": null
}
```

---

## 2. List Items
- **Endpoint:** `GET /api/items`
- **Query Parameters (Optional):**
  - `type` (`string`, optional): Filter by `expense` or `task`.
  - `status` (`string`, optional): Filter by `pending`, `in_progress`, `completed`, `paid`, `cancelled`.
  - `category` (`string`, optional): Filter by category name (e.g. `Books`, `Project`).
  - `search` (`string`, optional): Search in `title` and `description`.
- **Success Response (200 OK):**
```json
{
  "success": true,
  "data": [
    {
      "id": 1,
      "title": "C++ Semester Textbook & Lab Manual",
      "type": "expense",
      "category": "Books",
      "amount": 64.50,
      "status": "paid",
      "priority": "high",
      "due_date": "2026-11-20",
      "description": "Required textbook for CS201 course",
      "created_at": "2026-11-23T16:00:00Z",
      "updated_at": "2026-11-23T16:00:00Z"
    }
  ],
  "error": null
}
```

---

## 3. Create Item
- **Endpoint:** `POST /api/items`
- **Request Headers:** `Content-Type: application/json`
- **Request Body:**
```json
{
  "title": "Discrete Math Quiz 3",
  "type": "task",
  "category": "Exam",
  "amount": 0.0,
  "status": "pending",
  "priority": "high",
  "due_date": "2026-12-02",
  "description": "Sets, Relations and Graph Theory"
}
```
- **Validation Rules:**
  - `title`: String, 1–120 characters, required.
  - `type`: Must be `"expense"` or `"task"`, required.
  - `category`: Must be one of `['Tuition', 'Books', 'Food', 'Housing', 'Travel', 'Assignment', 'Exam', 'Project', 'Personal', 'Other']`, required.
  - `amount`: Number, >= 0.0 (defaults to 0.0).
  - `status`: One of `['pending', 'in_progress', 'completed', 'paid', 'cancelled']` (defaults to `"pending"`).
  - `priority`: One of `['low', 'medium', 'high']` (defaults to `"medium"`).
  - `due_date`: String format `YYYY-MM-DD` or empty.
- **Success Response (201 Created):**
```json
{
  "success": true,
  "data": {
    "id": 7,
    "title": "Discrete Math Quiz 3",
    "type": "task",
    "category": "Exam",
    "amount": 0.0,
    "status": "pending",
    "priority": "high",
    "due_date": "2026-12-02",
    "description": "Sets, Relations and Graph Theory",
    "created_at": "2026-11-23T21:40:00Z",
    "updated_at": "2026-11-23T21:40:00Z"
  },
  "error": null
}
```
- **Error Response (400 Bad Request):**
```json
{
  "success": false,
  "data": null,
  "error": {
    "code": "VALIDATION_FAILED",
    "message": "Field 'title' cannot be empty."
  }
}
```

---

## 4. Update Item
- **Endpoint:** `PUT /api/items/{id}`
- **Path Parameter:** `id` (integer)
- **Request Body:**
```json
{
  "title": "Discrete Math Quiz 3 - Prep Finished",
  "type": "task",
  "category": "Exam",
  "amount": 0.0,
  "status": "completed",
  "priority": "medium",
  "due_date": "2026-12-02",
  "description": "Completed all practice sets"
}
```
- **Success Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "id": 7,
    "title": "Discrete Math Quiz 3 - Prep Finished",
    "type": "task",
    "category": "Exam",
    "amount": 0.0,
    "status": "completed",
    "priority": "medium",
    "due_date": "2026-12-02",
    "description": "Completed all practice sets",
    "created_at": "2026-11-23T21:40:00Z",
    "updated_at": "2026-11-23T21:45:00Z"
  },
  "error": null
}
```
- **Error Response (404 Not Found):**
```json
{
  "success": false,
  "data": null,
  "error": {
    "code": "NOT_FOUND",
    "message": "Item with id 7 was not found."
  }
}
```

---

## 5. Delete Item
- **Endpoint:** `DELETE /api/items/{id}`
- **Path Parameter:** `id` (integer)
- **Success Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "id": 7,
    "deleted": true
  },
  "error": null
}
```
- **Error Response (404 Not Found):**
```json
{
  "success": false,
  "data": null,
  "error": {
    "code": "NOT_FOUND",
    "message": "Item with id 7 was not found."
  }
}
```

---

## 6. Summary Stats
- **Endpoint:** `GET /api/items/summary`
- **Description:** Returns aggregate metrics for student dashboard widgets (total expense, pending expenses, total tasks, completed tasks, tasks due soon).
- **Success Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "expenses": {
      "total_count": 3,
      "total_amount": 209.50,
      "paid_amount": 184.50,
      "pending_amount": 25.00
    },
    "tasks": {
      "total_count": 3,
      "completed_count": 1,
      "in_progress_count": 1,
      "pending_count": 1,
      "completion_percentage": 33.3
    }
  },
  "error": null
}
```
