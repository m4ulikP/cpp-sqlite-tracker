-- ====================================================================
-- Student Expense & Task Tracker - SQLite Database Schema
-- File: data/schema.sql
-- Database: data/tracker.db
-- ====================================================================

-- 1. Main items table (Unified model for student expenses and tasks)
CREATE TABLE IF NOT EXISTS items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL CHECK(length(trim(title)) > 0),
    type TEXT NOT NULL CHECK(type IN ('expense', 'task')),
    category TEXT NOT NULL CHECK(category IN ('Tuition', 'Books', 'Food', 'Housing', 'Travel', 'Assignment', 'Exam', 'Project', 'Personal', 'Other')),
    amount REAL NOT NULL DEFAULT 0.0 CHECK(amount >= 0.0),
    status TEXT NOT NULL DEFAULT 'pending' CHECK(status IN ('pending', 'in_progress', 'completed', 'paid', 'cancelled')),
    priority TEXT NOT NULL DEFAULT 'medium' CHECK(priority IN ('low', 'medium', 'high')),
    due_date TEXT, -- Stored as ISO8601 string 'YYYY-MM-DD'
    description TEXT DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
    updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
);

-- 2. Performance indexes for filtering and sorting
CREATE INDEX IF NOT EXISTS idx_items_type ON items(type);
CREATE INDEX IF NOT EXISTS idx_items_status ON items(status);
CREATE INDEX IF NOT EXISTS idx_items_category ON items(category);
CREATE INDEX IF NOT EXISTS idx_items_due_date ON items(due_date);
CREATE INDEX IF NOT EXISTS idx_items_created_at ON items(created_at);

-- 3. Automatic timestamp updater trigger
CREATE TRIGGER IF NOT EXISTS trg_items_updated_at
AFTER UPDATE ON items
FOR EACH ROW
BEGIN
    UPDATE items
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now')
    WHERE id = OLD.id;
END;

-- 4. Initial seed data for college demo verification
INSERT INTO items (title, type, category, amount, status, priority, due_date, description)
VALUES 
    ('C++ Semester Textbook & Lab Manual', 'expense', 'Books', 64.50, 'paid', 'high', '2026-11-20', 'Required textbook for CS201 course'),
    ('Campus Cafeteria Monthly Meal Card', 'expense', 'Food', 120.00, 'paid', 'medium', '2026-11-25', 'Meal credits reload'),
    ('Submit C++ Project Architecture Proposal', 'task', 'Project', 0.0, 'completed', 'high', '2026-11-23', 'Document 3-layer architecture, schema and API specs'),
    ('Implement SQLite Parameterized Repository', 'task', 'Project', 0.0, 'in_progress', 'high', '2026-11-28', 'Write C++20 database access layer preventing SQL injection'),
    ('Final Exam Preparation: Data Structures', 'task', 'Exam', 0.0, 'pending', 'high', '2026-12-05', 'Chapters 4 to 8 review and practice questions'),
    ('Dormitory Internet Subscription Share', 'expense', 'Housing', 25.00, 'pending', 'medium', '2026-11-30', 'Monthly high-speed fiber share');
