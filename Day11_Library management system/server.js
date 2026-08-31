// Library Management System - Backend (Node.js + Express + MySQL)

const express = require('express');
const mysql = require('mysql2');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// ---- Database connection ----
const db = mysql.createPool({
    host: 'localhost',
    user: 'root',
    password: '',            // <-- put your MySQL password here
    database: 'library_management'
});

// ================= BOOKS =================

app.get('/api/books', (req, res) => {
    const search = req.query.search;
    let sql = 'SELECT * FROM books';
    let params = [];
    if (search) {
        sql += ' WHERE title LIKE ? OR author LIKE ? OR category LIKE ?';
        const term = `%${search}%`;
        params = [term, term, term];
    }
    sql += ' ORDER BY title ASC';
    db.query(sql, params, (err, results) => {
        if (err) return res.status(500).json({ error: err.message });
        res.json(results);
    });
});

app.post('/api/books', (req, res) => {
    const { title, author, isbn, category, total_copies } = req.body;
    if (!title || !author) return res.status(400).json({ error: 'Title and author are required' });
    const copies = total_copies || 1;
    const sql = 'INSERT INTO books (title, author, isbn, category, total_copies, available_copies) VALUES (?, ?, ?, ?, ?, ?)';
    db.query(sql, [title, author, isbn, category || 'General', copies, copies], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        res.status(201).json({ id: result.insertId });
    });
});

app.put('/api/books/:id', (req, res) => {
    const { title, author, isbn, category, total_copies } = req.body;
    const sql = 'UPDATE books SET title=?, author=?, isbn=?, category=?, total_copies=? WHERE id=?';
    db.query(sql, [title, author, isbn, category, total_copies, req.params.id], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        if (result.affectedRows === 0) return res.status(404).json({ error: 'Book not found' });
        res.json({ message: 'Book updated' });
    });
});

app.delete('/api/books/:id', (req, res) => {
    db.query('DELETE FROM books WHERE id = ?', [req.params.id], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        if (result.affectedRows === 0) return res.status(404).json({ error: 'Book not found' });
        res.json({ message: 'Book deleted' });
    });
});

// ================= MEMBERS =================

app.get('/api/members', (req, res) => {
    const search = req.query.search;
    let sql = 'SELECT * FROM members';
    let params = [];
    if (search) {
        sql += ' WHERE name LIKE ? OR email LIKE ?';
        const term = `%${search}%`;
        params = [term, term];
    }
    sql += ' ORDER BY name ASC';
    db.query(sql, params, (err, results) => {
        if (err) return res.status(500).json({ error: err.message });
        res.json(results);
    });
});

app.post('/api/members', (req, res) => {
    const { name, email, phone } = req.body;
    if (!name) return res.status(400).json({ error: 'Name is required' });
    db.query('INSERT INTO members (name, email, phone) VALUES (?, ?, ?)', [name, email, phone], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        res.status(201).json({ id: result.insertId });
    });
});

app.delete('/api/members/:id', (req, res) => {
    db.query('DELETE FROM members WHERE id = ?', [req.params.id], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        if (result.affectedRows === 0) return res.status(404).json({ error: 'Member not found' });
        res.json({ message: 'Member deleted' });
    });
});

// ================= ISSUES (Issue / Return) =================

// List issues (with book + member info), optional status filter
app.get('/api/issues', (req, res) => {
    const status = req.query.status;
    let sql = `
        SELECT issues.*, books.title AS book_title, members.name AS member_name
        FROM issues
        JOIN books ON issues.book_id = books.id
        JOIN members ON issues.member_id = members.id
    `;
    let params = [];
    if (status) {
        sql += ' WHERE issues.status = ?';
        params.push(status);
    }
    sql += ' ORDER BY issues.issue_date DESC';
    db.query(sql, params, (err, results) => {
        if (err) return res.status(500).json({ error: err.message });
        res.json(results);
    });
});

// Issue a book to a member
app.post('/api/issues', (req, res) => {
    const { book_id, member_id, issue_date, due_date } = req.body;
    if (!book_id || !member_id || !issue_date || !due_date) {
        return res.status(400).json({ error: 'book_id, member_id, issue_date and due_date are required' });
    }

    db.query('SELECT available_copies FROM books WHERE id = ?', [book_id], (err, results) => {
        if (err) return res.status(500).json({ error: err.message });
        if (results.length === 0) return res.status(404).json({ error: 'Book not found' });
        if (results[0].available_copies < 1) return res.status(400).json({ error: 'No copies available' });

        db.query(
            'INSERT INTO issues (book_id, member_id, issue_date, due_date, status) VALUES (?, ?, ?, ?, "Issued")',
            [book_id, member_id, issue_date, due_date],
            (err, result) => {
                if (err) return res.status(500).json({ error: err.message });
                db.query('UPDATE books SET available_copies = available_copies - 1 WHERE id = ?', [book_id]);
                res.status(201).json({ id: result.insertId });
            }
        );
    });
});

// Return a book
app.put('/api/issues/:id/return', (req, res) => {
    db.query('SELECT * FROM issues WHERE id = ?', [req.params.id], (err, results) => {
        if (err) return res.status(500).json({ error: err.message });
        if (results.length === 0) return res.status(404).json({ error: 'Issue record not found' });
        if (results[0].status === 'Returned') return res.status(400).json({ error: 'Already returned' });

        const bookId = results[0].book_id;
        db.query(
            'UPDATE issues SET status = "Returned", return_date = CURDATE() WHERE id = ?',
            [req.params.id],
            (err) => {
                if (err) return res.status(500).json({ error: err.message });
                db.query('UPDATE books SET available_copies = available_copies + 1 WHERE id = ?', [bookId]);
                res.json({ message: 'Book returned successfully' });
            }
        );
    });
});

app.listen(PORT, () => {
    console.log(`Server running at http://localhost:${PORT}`);
});
