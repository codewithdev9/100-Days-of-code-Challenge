# Library Management System

Stack: **HTML/CSS/JS** (frontend) + **Node.js/Express** (backend) + **MySQL** (DBMS)

## Features
- **Books** — add, delete, search, track total vs available copies
- **Members** — add, delete, search
- **Issue / Return** — issue a book to a member (auto due date), mark returned, filter by status
- Available copies auto-update when a book is issued or returned

## Folder structure
```
library-management-system/
├── schema.sql        # MySQL: books, members, issues tables + sample data
├── server.js         # Express backend + REST API
├── package.json
└── public/
    └── index.html     # Frontend UI (tabs: Books / Members / Issue-Return)
```

## Setup

### 1. Create the database
```bash
mysql -u root -p < schema.sql
```
Creates `library_management` DB with `books`, `members`, `issues` tables (foreign keys linked), plus sample rows.

### 2. Configure the backend
In `server.js`, set your MySQL password:
```js
const db = mysql.createPool({
    host: 'localhost',
    user: 'root',
    password: 'YOUR_MYSQL_PASSWORD',
    database: 'library_management'
});
```

### 3. Install & run
```bash
npm install
npm start
```
Open **http://localhost:3000**

## API Reference

| Method | Endpoint                  | Description                          |
|--------|----------------------------|----------------------------------------|
| GET    | /api/books                 | List books (supports `?search=`)      |
| POST   | /api/books                 | Add a book                            |
| PUT    | /api/books/:id             | Update a book                         |
| DELETE | /api/books/:id             | Delete a book                         |
| GET    | /api/members                | List members (supports `?search=`)   |
| POST   | /api/members                | Add a member                         |
| DELETE | /api/members/:id            | Delete a member                      |
| GET    | /api/issues                 | List issue records (supports `?status=Issued/Returned`) |
| POST   | /api/issues                 | Issue a book to a member (checks availability) |
| PUT    | /api/issues/:id/return      | Mark a book as returned              |

## How issuing works (DBMS logic)
- `books.available_copies` decreases by 1 when a book is issued, and increases by 1 when returned.
- Issuing is blocked if `available_copies` is 0.
- `issues` table uses foreign keys to `books` and `members` — this is the core relational design of the system.

## Notes
- Want this in **pure Java** instead (JDBC + Servlets, or a Swing desktop app)? Just ask — same DB schema can be reused.
