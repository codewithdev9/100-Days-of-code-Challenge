# Contact Management System

Stack: **HTML/CSS/JS** (frontend) + **Node.js/Express** (backend) + **MySQL** (DBMS)

## Features
- Add, view, edit, delete contacts (full CRUD)
- Search by name, phone, or email
- Categorize contacts (Family / Friend / Work / General)

## Folder structure
```
contact-management-system/
├── schema.sql        # MySQL database + table + sample data
├── server.js         # Express backend + REST API
├── package.json
└── public/
    └── index.html     # Frontend UI (HTML+CSS+JS, no framework)
```

## Setup

### 1. Create the database
Make sure MySQL is installed and running, then:
```bash
mysql -u root -p < schema.sql
```
This creates the `contact_management` database with a `contacts` table and 3 sample rows.

### 2. Configure the backend
Open `server.js` and set your MySQL password in the `db` config block:
```js
const db = mysql.createPool({
    host: 'localhost',
    user: 'root',
    password: 'YOUR_MYSQL_PASSWORD',
    database: 'contact_management'
});
```

### 3. Install dependencies & run
```bash
npm install
npm start
```
Server starts at **http://localhost:3000** — open that URL in your browser.

## API Reference

| Method | Endpoint              | Description              |
|--------|------------------------|---------------------------|
| GET    | /api/contacts           | List all contacts (supports `?search=`) |
| GET    | /api/contacts/:id       | Get one contact          |
| POST   | /api/contacts           | Create a contact         |
| PUT    | /api/contacts/:id       | Update a contact         |
| DELETE | /api/contacts/:id       | Delete a contact         |

## Notes
- If you'd rather use **pure Java** (JDBC + Servlets, or a Swing desktop app) instead of Node.js, this can be rebuilt that way too — just ask.
- To deploy, you can host the MySQL DB anywhere (e.g. PlanetScale, Railway) and the Node server on Render/Railway; just update the `db` config.
