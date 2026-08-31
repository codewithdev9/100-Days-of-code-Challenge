-- Contact Management System - Database Schema (MySQL)

CREATE DATABASE IF NOT EXISTS contact_management;
USE contact_management;

CREATE TABLE IF NOT EXISTS contacts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    phone VARCHAR(20) NOT NULL,
    email VARCHAR(100),
    address VARCHAR(255),
    category VARCHAR(50) DEFAULT 'General',   -- e.g. Family, Friend, Work
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- Sample data (optional)
INSERT INTO contacts (name, phone, email, address, category) VALUES
('Rahul Sharma', '9876543210', 'rahul@example.com', 'Aligarh, UP', 'Friend'),
('Priya Verma', '9123456780', 'priya@example.com', 'Delhi', 'Work'),
('Amit Singh', '9988776655', 'amit@example.com', 'Lucknow, UP', 'Family');
