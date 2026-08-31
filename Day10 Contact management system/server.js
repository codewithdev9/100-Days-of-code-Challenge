// Contact Management System - Backend (Node.js + Express + MySQL)

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
// Update these credentials to match your local MySQL setup
const db = mysql.createPool({
    host: 'localhost',
    user: 'root',
    password: '',            // <-- put your MySQL password here
    database: 'contact_management'
});

// ---- Routes ----

// Get all contacts (supports ?search=)
app.get('/api/contacts', (req, res) => {
    const search = req.query.search;
    let sql = 'SELECT * FROM contacts';
    let params = [];

    if (search) {
        sql += ' WHERE name LIKE ? OR phone LIKE ? OR email LIKE ?';
        const term = `%${search}%`;
        params = [term, term, term];
    }
    sql += ' ORDER BY name ASC';

    db.query(sql, params, (err, results) => {
        if (err) return res.status(500).json({ error: err.message });
        res.json(results);
    });
});

// Get single contact
app.get('/api/contacts/:id', (req, res) => {
    db.query('SELECT * FROM contacts WHERE id = ?', [req.params.id], (err, results) => {
        if (err) return res.status(500).json({ error: err.message });
        if (results.length === 0) return res.status(404).json({ error: 'Contact not found' });
        res.json(results[0]);
    });
});

// Create contact
app.post('/api/contacts', (req, res) => {
    const { name, phone, email, address, category } = req.body;
    if (!name || !phone) {
        return res.status(400).json({ error: 'Name and phone are required' });
    }
    const sql = 'INSERT INTO contacts (name, phone, email, address, category) VALUES (?, ?, ?, ?, ?)';
    db.query(sql, [name, phone, email, address, category || 'General'], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        res.status(201).json({ id: result.insertId, name, phone, email, address, category });
    });
});

// Update contact
app.put('/api/contacts/:id', (req, res) => {
    const { name, phone, email, address, category } = req.body;
    const sql = 'UPDATE contacts SET name=?, phone=?, email=?, address=?, category=? WHERE id=?';
    db.query(sql, [name, phone, email, address, category, req.params.id], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        if (result.affectedRows === 0) return res.status(404).json({ error: 'Contact not found' });
        res.json({ message: 'Contact updated successfully' });
    });
});

// Delete contact
app.delete('/api/contacts/:id', (req, res) => {
    db.query('DELETE FROM contacts WHERE id = ?', [req.params.id], (err, result) => {
        if (err) return res.status(500).json({ error: err.message });
        if (result.affectedRows === 0) return res.status(404).json({ error: 'Contact not found' });
        res.json({ message: 'Contact deleted successfully' });
    });
});

app.listen(PORT, () => {
    console.log(`Server running at http://localhost:${PORT}`);
});
