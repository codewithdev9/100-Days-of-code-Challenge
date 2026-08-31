import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

import java.io.*;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.sql.*;
import java.util.*;
import java.util.regex.*;

/**
 * Student Record Management System - Backend
 * Pure Java (JDK HttpServer) + JDBC (MySQL) + hand-rolled JSON. No external
 * frameworks required beyond the MySQL Connector/J driver jar at runtime.
 *
 * Compile:  javac -d out src/Main.java
 * Run:      java -cp out:mysql-connector-j-9.0.0.jar Main      (Linux/Mac)
 *           java -cp "out;mysql-connector-j-9.0.0.jar" Main    (Windows)
 */
public class Main {

    // ---- Update these to match your local MySQL setup ----
    static final String DB_URL = "jdbc:mysql://localhost:3306/student_records";
    static final String DB_USER = "root";
    static final String DB_PASS = "";                  // <-- put your MySQL password here
    static final int PORT = 8080;

    public static void main(String[] args) throws Exception {
        try {
            Class.forName("com.mysql.cj.jdbc.Driver");
        } catch (ClassNotFoundException e) {
            System.err.println("MySQL JDBC driver not found on classpath. See README for setup.");
        }

        HttpServer server = HttpServer.create(new InetSocketAddress(PORT), 0);
        server.createContext("/api/students", new StudentsHandler());
        server.createContext("/api/courses", new CoursesHandler());
        server.createContext("/api/marks", new MarksHandler());
        server.createContext("/", new StaticFileHandler());
        server.setExecutor(null);
        server.start();
        System.out.println("Server running at http://localhost:" + PORT);
    }

    static Connection connect() throws SQLException {
        return DriverManager.getConnection(DB_URL, DB_USER, DB_PASS);
    }

    // ===================== Students =====================
    static class StudentsHandler implements HttpHandler {
        public void handle(HttpExchange ex) throws IOException {
            try {
                String path = ex.getRequestURI().getPath();
                String method = ex.getRequestMethod();
                String[] parts = path.split("/");
                Integer id = (parts.length > 3) ? Integer.parseInt(parts[3]) : null;

                if ("GET".equals(method) && id == null) {
                    listStudents(ex);
                } else if ("POST".equals(method) && id == null) {
                    createStudent(ex);
                } else if ("DELETE".equals(method) && id != null) {
                    deleteStudent(ex, id);
                } else if ("PUT".equals(method) && id != null) {
                    updateStudent(ex, id);
                } else {
                    Http.send(ex, 404, Json.error("Not found"));
                }
            } catch (Exception e) {
                Http.send(ex, 500, Json.error(e.getMessage()));
            }
        }

        void listStudents(HttpExchange ex) throws Exception {
            String query = Http.queryParam(ex, "search");
            String sql = "SELECT * FROM students";
            if (query != null && !query.isEmpty()) {
                sql += " WHERE name LIKE ? OR roll_no LIKE ? OR department LIKE ?";
            }
            sql += " ORDER BY name";

            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement(sql)) {
                if (query != null && !query.isEmpty()) {
                    String term = "%" + query + "%";
                    ps.setString(1, term); ps.setString(2, term); ps.setString(3, term);
                }
                ResultSet rs = ps.executeQuery();
                List<String> rows = new ArrayList<>();
                while (rs.next()) rows.add(studentJson(rs));
                Http.send(ex, 200, Json.array(rows));
            }
        }

        void createStudent(HttpExchange ex) throws Exception {
            Map<String, String> b = Json.parseObject(Http.body(ex));
            if (Json.isBlank(b.get("name")) || Json.isBlank(b.get("roll_no"))) {
                Http.send(ex, 400, Json.error("name and roll_no are required")); return;
            }
            String sql = "INSERT INTO students (name, roll_no, department, year, email, phone) VALUES (?,?,?,?,?,?)";
            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
                ps.setString(1, b.get("name"));
                ps.setString(2, b.get("roll_no"));
                ps.setString(3, b.getOrDefault("department", ""));
                ps.setInt(4, Json.toInt(b.get("year"), 1));
                ps.setString(5, b.getOrDefault("email", ""));
                ps.setString(6, b.getOrDefault("phone", ""));
                ps.executeUpdate();
                ResultSet keys = ps.getGeneratedKeys();
                keys.next();
                Http.send(ex, 201, "{\"id\":" + keys.getInt(1) + "}");
            }
        }

        void updateStudent(HttpExchange ex, int id) throws Exception {
            Map<String, String> b = Json.parseObject(Http.body(ex));
            String sql = "UPDATE students SET name=?, roll_no=?, department=?, year=?, email=?, phone=? WHERE id=?";
            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement(sql)) {
                ps.setString(1, b.get("name"));
                ps.setString(2, b.get("roll_no"));
                ps.setString(3, b.getOrDefault("department", ""));
                ps.setInt(4, Json.toInt(b.get("year"), 1));
                ps.setString(5, b.getOrDefault("email", ""));
                ps.setString(6, b.getOrDefault("phone", ""));
                ps.setInt(7, id);
                int rows = ps.executeUpdate();
                if (rows == 0) { Http.send(ex, 404, Json.error("Student not found")); return; }
                Http.send(ex, 200, "{\"message\":\"Student updated\"}");
            }
        }

        void deleteStudent(HttpExchange ex, int id) throws Exception {
            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement("DELETE FROM students WHERE id=?")) {
                ps.setInt(1, id);
                int rows = ps.executeUpdate();
                if (rows == 0) { Http.send(ex, 404, Json.error("Student not found")); return; }
                Http.send(ex, 200, "{\"message\":\"Student deleted\"}");
            }
        }

        String studentJson(ResultSet rs) throws SQLException {
            return "{" +
                Json.field("id", rs.getInt("id")) + "," +
                Json.field("name", rs.getString("name")) + "," +
                Json.field("roll_no", rs.getString("roll_no")) + "," +
                Json.field("department", rs.getString("department")) + "," +
                Json.field("year", rs.getInt("year")) + "," +
                Json.field("email", rs.getString("email")) + "," +
                Json.field("phone", rs.getString("phone")) +
            "}";
        }
    }

    // ===================== Courses =====================
    static class CoursesHandler implements HttpHandler {
        public void handle(HttpExchange ex) throws IOException {
            try {
                String path = ex.getRequestURI().getPath();
                String method = ex.getRequestMethod();
                String[] parts = path.split("/");
                Integer id = (parts.length > 3) ? Integer.parseInt(parts[3]) : null;

                if ("GET".equals(method) && id == null) {
                    listCourses(ex);
                } else if ("POST".equals(method) && id == null) {
                    createCourse(ex);
                } else if ("DELETE".equals(method) && id != null) {
                    deleteCourse(ex, id);
                } else {
                    Http.send(ex, 404, Json.error("Not found"));
                }
            } catch (Exception e) {
                Http.send(ex, 500, Json.error(e.getMessage()));
            }
        }

        void listCourses(HttpExchange ex) throws Exception {
            try (Connection c = connect(); Statement st = c.createStatement()) {
                ResultSet rs = st.executeQuery("SELECT * FROM courses ORDER BY code");
                List<String> rows = new ArrayList<>();
                while (rs.next()) rows.add(courseJson(rs));
                Http.send(ex, 200, Json.array(rows));
            }
        }

        void createCourse(HttpExchange ex) throws Exception {
            Map<String, String> b = Json.parseObject(Http.body(ex));
            if (Json.isBlank(b.get("code")) || Json.isBlank(b.get("name"))) {
                Http.send(ex, 400, Json.error("code and name are required")); return;
            }
            String sql = "INSERT INTO courses (code, name, credits) VALUES (?,?,?)";
            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
                ps.setString(1, b.get("code"));
                ps.setString(2, b.get("name"));
                ps.setInt(3, Json.toInt(b.get("credits"), 3));
                ps.executeUpdate();
                ResultSet keys = ps.getGeneratedKeys();
                keys.next();
                Http.send(ex, 201, "{\"id\":" + keys.getInt(1) + "}");
            }
        }

        void deleteCourse(HttpExchange ex, int id) throws Exception {
            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement("DELETE FROM courses WHERE id=?")) {
                ps.setInt(1, id);
                int rows = ps.executeUpdate();
                if (rows == 0) { Http.send(ex, 404, Json.error("Course not found")); return; }
                Http.send(ex, 200, "{\"message\":\"Course deleted\"}");
            }
        }

        String courseJson(ResultSet rs) throws SQLException {
            return "{" +
                Json.field("id", rs.getInt("id")) + "," +
                Json.field("code", rs.getString("code")) + "," +
                Json.field("name", rs.getString("name")) + "," +
                Json.field("credits", rs.getInt("credits")) +
            "}";
        }
    }

    // ===================== Marks =====================
    static class MarksHandler implements HttpHandler {
        public void handle(HttpExchange ex) throws IOException {
            try {
                String path = ex.getRequestURI().getPath();
                String method = ex.getRequestMethod();
                String[] parts = path.split("/");
                Integer id = (parts.length > 3) ? Integer.parseInt(parts[3]) : null;

                if ("GET".equals(method) && id == null) {
                    listMarks(ex);
                } else if ("POST".equals(method) && id == null) {
                    createMark(ex);
                } else if ("DELETE".equals(method) && id != null) {
                    deleteMark(ex, id);
                } else {
                    Http.send(ex, 404, Json.error("Not found"));
                }
            } catch (Exception e) {
                Http.send(ex, 500, Json.error(e.getMessage()));
            }
        }

        void listMarks(HttpExchange ex) throws Exception {
            String studentId = Http.queryParam(ex, "student_id");
            String sql = "SELECT marks.*, students.name AS student_name, students.roll_no, " +
                         "courses.code AS course_code, courses.name AS course_name " +
                         "FROM marks " +
                         "JOIN students ON marks.student_id = students.id " +
                         "JOIN courses ON marks.course_id = courses.id";
            if (studentId != null && !studentId.isEmpty()) sql += " WHERE marks.student_id = ?";
            sql += " ORDER BY marks.exam_date DESC";

            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement(sql)) {
                if (studentId != null && !studentId.isEmpty()) ps.setInt(1, Integer.parseInt(studentId));
                ResultSet rs = ps.executeQuery();
                List<String> rows = new ArrayList<>();
                while (rs.next()) rows.add(markJson(rs));
                Http.send(ex, 200, Json.array(rows));
            }
        }

        void createMark(HttpExchange ex) throws Exception {
            Map<String, String> b = Json.parseObject(Http.body(ex));
            if (Json.isBlank(b.get("student_id")) || Json.isBlank(b.get("course_id")) || Json.isBlank(b.get("marks_obtained"))) {
                Http.send(ex, 400, Json.error("student_id, course_id and marks_obtained are required")); return;
            }
            String sql = "INSERT INTO marks (student_id, course_id, marks_obtained) VALUES (?,?,?)";
            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
                ps.setInt(1, Json.toInt(b.get("student_id"), 0));
                ps.setInt(2, Json.toInt(b.get("course_id"), 0));
                ps.setDouble(3, Double.parseDouble(b.get("marks_obtained")));
                ps.executeUpdate();
                ResultSet keys = ps.getGeneratedKeys();
                keys.next();
                Http.send(ex, 201, "{\"id\":" + keys.getInt(1) + "}");
            }
        }

        void deleteMark(HttpExchange ex, int id) throws Exception {
            try (Connection c = connect(); PreparedStatement ps = c.prepareStatement("DELETE FROM marks WHERE id=?")) {
                ps.setInt(1, id);
                int rows = ps.executeUpdate();
                if (rows == 0) { Http.send(ex, 404, Json.error("Mark record not found")); return; }
                Http.send(ex, 200, "{\"message\":\"Record deleted\"}");
            }
        }

        String markJson(ResultSet rs) throws SQLException {
            double marks = rs.getDouble("marks_obtained");
            return "{" +
                Json.field("id", rs.getInt("id")) + "," +
                Json.field("student_id", rs.getInt("student_id")) + "," +
                Json.field("course_id", rs.getInt("course_id")) + "," +
                Json.field("student_name", rs.getString("student_name")) + "," +
                Json.field("roll_no", rs.getString("roll_no")) + "," +
                Json.field("course_code", rs.getString("course_code")) + "," +
                Json.field("course_name", rs.getString("course_name")) + "," +
                Json.field("marks_obtained", marks) + "," +
                Json.field("grade", grade(marks)) + "," +
                Json.field("exam_date", String.valueOf(rs.getDate("exam_date"))) +
            "}";
        }

        String grade(double m) {
            if (m >= 90) return "A+";
            if (m >= 80) return "A";
            if (m >= 70) return "B";
            if (m >= 60) return "C";
            if (m >= 50) return "D";
            return "F";
        }
    }

    // ===================== Static file serving =====================
    static class StaticFileHandler implements HttpHandler {
        public void handle(HttpExchange ex) throws IOException {
            String path = ex.getRequestURI().getPath();
            if (path.equals("/")) path = "/index.html";
            Path file = Paths.get("public" + path).normalize();
            if (!file.startsWith(Paths.get("public")) || !Files.exists(file) || Files.isDirectory(file)) {
                Http.send(ex, 404, "Not found");
                return;
            }
            byte[] data = Files.readAllBytes(file);
            String contentType = path.endsWith(".html") ? "text/html" :
                                  path.endsWith(".css") ? "text/css" :
                                  path.endsWith(".js") ? "application/javascript" : "application/octet-stream";
            ex.getResponseHeaders().set("Content-Type", contentType + "; charset=utf-8");
            ex.sendResponseHeaders(200, data.length);
            try (OutputStream os = ex.getResponseBody()) { os.write(data); }
        }
    }

    // ===================== HTTP helpers =====================
    static class Http {
        static String body(HttpExchange ex) throws IOException {
            try (InputStream is = ex.getRequestBody(); ByteArrayOutputStream bos = new ByteArrayOutputStream()) {
                byte[] buf = new byte[1024];
                int n;
                while ((n = is.read(buf)) != -1) bos.write(buf, 0, n);
                return bos.toString(StandardCharsets.UTF_8);
            }
        }

        static String queryParam(HttpExchange ex, String name) {
            String rawQuery = ex.getRequestURI().getRawQuery();
            if (rawQuery == null) return null;
            for (String pair : rawQuery.split("&")) {
                String[] kv = pair.split("=", 2);
                if (kv.length == 2 && kv[0].equals(name)) {
                    try {
                        return java.net.URLDecoder.decode(kv[1], "UTF-8");
                    } catch (Exception e) {
                        return kv[1];
                    }
                }
            }
            return null;
        }

        static void send(HttpExchange ex, int status, String json) throws IOException {
            byte[] data = json.getBytes(StandardCharsets.UTF_8);
            ex.getResponseHeaders().set("Content-Type", "application/json; charset=utf-8");
            ex.getResponseHeaders().set("Access-Control-Allow-Origin", "*");
            ex.sendResponseHeaders(status, data.length);
            try (OutputStream os = ex.getResponseBody()) { os.write(data); }
        }
    }

    // ===================== Minimal JSON utilities =====================
    static class Json {
        static final Pattern PAIR = Pattern.compile(
            "\"(\\w+)\"\\s*:\\s*(\"(?:[^\"\\\\]|\\\\.)*\"|-?\\d+(?:\\.\\d+)?|true|false|null)");

        static Map<String, String> parseObject(String json) {
            Map<String, String> map = new LinkedHashMap<>();
            if (json == null) return map;
            Matcher m = PAIR.matcher(json);
            while (m.find()) {
                String key = m.group(1);
                String val = m.group(2);
                if (val.startsWith("\"")) {
                    val = val.substring(1, val.length() - 1)
                             .replace("\\\"", "\"")
                             .replace("\\\\", "\\");
                }
                map.put(key, val);
            }
            return map;
        }

        static boolean isBlank(String s) { return s == null || s.trim().isEmpty(); }

        static int toInt(String s, int def) {
            try { return Integer.parseInt(s); } catch (Exception e) { return def; }
        }

        static String field(String key, String value) {
            return "\"" + key + "\":\"" + escape(value == null ? "" : value) + "\"";
        }

        static String field(String key, int value) { return "\"" + key + "\":" + value; }

        static String field(String key, double value) { return "\"" + key + "\":" + value; }

        static String escape(String s) {
            return s.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n");
        }

        static String array(List<String> jsonObjects) {
            return "[" + String.join(",", jsonObjects) + "]";
        }

        static String error(String message) {
            return "{\"error\":\"" + escape(message == null ? "Unknown error" : message) + "\"}";
        }
    }
}
