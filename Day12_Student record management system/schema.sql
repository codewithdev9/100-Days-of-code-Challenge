-- Student Record Management System - Database Schema (MySQL)

CREATE DATABASE IF NOT EXISTS student_records;
USE student_records;

CREATE TABLE IF NOT EXISTS students (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    roll_no VARCHAR(30) NOT NULL UNIQUE,
    department VARCHAR(60) NOT NULL,
    year INT NOT NULL,
    email VARCHAR(100),
    phone VARCHAR(20),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS courses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    code VARCHAR(20) NOT NULL UNIQUE,
    name VARCHAR(120) NOT NULL,
    credits INT NOT NULL DEFAULT 3
);

CREATE TABLE IF NOT EXISTS marks (
    id INT AUTO_INCREMENT PRIMARY KEY,
    student_id INT NOT NULL,
    course_id INT NOT NULL,
    marks_obtained DECIMAL(5,2) NOT NULL,
    exam_date DATE DEFAULT (CURRENT_DATE),
    FOREIGN KEY (student_id) REFERENCES students(id) ON DELETE CASCADE,
    FOREIGN KEY (course_id) REFERENCES courses(id) ON DELETE CASCADE
);

-- Sample data
INSERT INTO students (name, roll_no, department, year, email, phone) VALUES
('Rahul Sharma', 'CS101', 'Computer Science', 2, 'rahul@example.com', '9876543210'),
('Priya Verma', 'EC102', 'Electronics', 3, 'priya@example.com', '9123456780'),
('Amit Singh', 'CS103', 'Computer Science', 1, 'amit@example.com', '9988776655');

INSERT INTO courses (code, name, credits) VALUES
('CS201', 'Data Structures', 4),
('CS202', 'Database Systems', 4),
('EC301', 'Digital Electronics', 3);

INSERT INTO marks (student_id, course_id, marks_obtained) VALUES
(1, 1, 88.00),
(1, 2, 76.50),
(2, 3, 91.00);
