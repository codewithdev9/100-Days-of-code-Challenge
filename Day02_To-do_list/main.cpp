#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cctype>
#include <iomanip>

using namespace std;

const string FILENAME = "tasks.txt";
const string BACKUP_FILENAME = "tasks_backup.txt";
const string PASSWORD = "admin123";

class Task {
private:
    string name;
    bool completed;
    int priority;
    string deadline;

public:
    Task(const string& n = "", int p = 1, const string& d = "0000-00-00")
        : name(n), completed(false), priority(max(1, min(5, p))), deadline(d) {}

    // Getters
    string getName() const { return name; }
    bool isCompleted() const { return completed; }
    int getPriority() const { return priority; }
    string getDeadline() const { return deadline; }

    // Setters
    void setName(const string& n) { name = n; }
    void setCompleted(bool c) { completed = c; }
    void setPriority(int p) { priority = max(1, min(5, p)); }
    void setDeadline(const string& d) { deadline = d; }

    void markComplete() { completed = true; }
    void markIncomplete() { completed = false; }

    void display() const {
        cout << "[" << (completed ? 'X' : ' ') << "] " << name
             << " (Priority: " << priority << ", Deadline: "
             << (deadline == "0000-00-00" ? "None" : deadline) << ")" << endl;
    }
};

class TodoManager {
private:
    vector<Task> tasks;
    bool loggedIn = false;

    // Utility functions
    void clearScreen() const {
        system("cls || clear");
    }

    void pauseScreen() const {
        cout << "\nPress Enter to continue...";
        cin.ignore();
        cin.get();
    }

    string getCurrentDate() const {
        time_t t = time(nullptr);
        tm* now = localtime(&t);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);
        return string(buffer);
    }

    bool isValidDate(const string& date) const {
        if (date.length() != 10) return false;
        if (date[4] != '-' || date[7] != '-') return false;
        
        for (int i = 0; i < 10; i++) {
            if (i == 4 || i == 7) continue;
            if (!isdigit(date[i])) return false;
        }
        return true;
    }

    void encryptDecrypt(string& data, bool encrypt) const {
        int shift = encrypt ? 3 : -3;
        for (char& c : data) {
            if (isalpha(c)) {
                char base = islower(c) ? 'a' : 'A';
                c = ((c - base + shift + 26) % 26) + base;
            }
        }
    }

public:
    // Core Task Functions
    void addTask(const string& name, int priority, const string& deadline) {
        tasks.emplace_back(name, priority, isValidDate(deadline) ? deadline : "0000-00-00");
    }

    void showTasks() const {
        if (tasks.empty()) {
            cout << "No tasks found!" << endl;
            return;
        }
        
        for (size_t i = 0; i < tasks.size(); i++) {
            cout << i + 1 << ". ";
            tasks[i].display();
        }
    }

    void deleteTask(int index) {
        if (index < 0 || index >= (int)tasks.size()) {
            cout << "Invalid task index!" << endl;
            return;
        }
        tasks.erase(tasks.begin() + index);
    }

    void updateTask(int index, const string& newName, int newPriority, const string& newDeadline) {
        if (index < 0 || index >= (int)tasks.size()) {
            cout << "Invalid task index!" << endl;
            return;
        }
        
        if (!newName.empty()) tasks[index].setName(newName);
        if (newPriority > 0) tasks[index].setPriority(newPriority);
        if (!newDeadline.empty() && isValidDate(newDeadline)) tasks[index].setDeadline(newDeadline);
    }

    void markComplete(int index) {
        if (index < 0 || index >= (int)tasks.size()) {
            cout << "Invalid task index!" << endl;
            return;
        }
        tasks[index].markComplete();
    }

    void markIncomplete(int index) {
        if (index < 0 || index >= (int)tasks.size()) {
            cout << "Invalid task index!" << endl;
            return;
        }
        tasks[index].markIncomplete();
    }

    void clearAllTasks() {
        tasks.clear();
    }

    int countTasks() const {
        return tasks.size();
    }

    bool isEmpty() const {
        return tasks.empty();
    }

    Task* getTaskByIndex(int index) {
        if (index < 0 || index >= (int)tasks.size()) return nullptr;
        return &tasks[index];
    }

    // UI Functions
    void printHeader() const {
        cout << "========================================" << endl;
        cout << "           TO-DO LIST MANAGER           " << endl;
        cout << "========================================" << endl;
    }

    void printFooter() const {
        cout << "========================================" << endl;
    }

    void printTask(const Task& task) const {
        task.display();
    }

    void printAllTasksFormatted() const {
        if (tasks.empty()) {
            cout << "No tasks to display." << endl;
            return;
        }
        
        cout << "\n" << left << setw(4) << "No." << setw(6) << "Status" 
             << setw(8) << "Priority" << setw(12) << "Deadline" << "Task Name" << endl;
        cout << "------------------------------------------------------------" << endl;
        
        for (size_t i = 0; i < tasks.size(); i++) {
            cout << left << setw(4) << i + 1
                 << setw(6) << (tasks[i].isCompleted() ? "Done" : "Pending")
                 << setw(8) << tasks[i].getPriority()
                 << setw(12) << (tasks[i].getDeadline() == "0000-00-00" ? "None" : tasks[i].getDeadline())
                 << tasks[i].getName() << endl;
        }
    }

    void showMenu() const {
        cout << "\nMain Menu:" << endl;
        cout << "1. Add Task" << endl;
        cout << "2. Show All Tasks" << endl;
        cout << "3. Delete Task" << endl;
        cout << "4. Update Task" << endl;
        cout << "5. Mark Complete" << endl;
        cout << "6. Mark Incomplete" << endl;
        cout << "7. Search Tasks" << endl;
        cout << "8. Filter Tasks" << endl;
        cout << "9. Sort Tasks" << endl;
        cout << "10. Show Statistics" << endl;
        cout << "11. Save Tasks" << endl;
        cout << "12. Load Tasks" << endl;
        cout << "13. Backup Tasks" << endl;
        cout << "14. Change Password" << endl;
        cout << "15. Logout" << endl;
        cout << "0. Exit" << endl;
        cout << "Choose an option: ";
    }

    int getUserChoice() const {
        int choice;
        cin >> choice;
        cin.ignore();
        return choice;
    }

    // File Handling
    bool fileExists(const string& filename) const {
        ifstream file(filename);
        return file.good();
    }

    void createFileIfNotExist(const string& filename) const {
        if (!fileExists(filename)) {
            ofstream file(filename);
        }
    }

    void saveTasksToFile() {
        ofstream file(FILENAME);
        if (!file) {
            cout << "Error saving tasks!" << endl;
            return;
        }
        
        for (const auto& task : tasks) {
            string encryptedName = task.getName();
            encryptDecrypt(encryptedName, true);
            
            file << encryptedName << "|" << task.isCompleted() << "|"
                 << task.getPriority() << "|" << task.getDeadline() << endl;
        }
        
        cout << "Tasks saved successfully!" << endl;
    }

    void loadTasksFromFile() {
        ifstream file(FILENAME);
        if (!file) {
            cout << "No saved tasks found!" << endl;
            return;
        }
        
        tasks.clear();
        string line;
        
        while (getline(file, line)) {
            size_t pos1 = line.find('|');
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);
            
            if (pos1 != string::npos && pos2 != string::npos && pos3 != string::npos) {
                string encryptedName = line.substr(0, pos1);
                bool completed = stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                int priority = stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
                string deadline = line.substr(pos3 + 1);
                
                encryptDecrypt(encryptedName, false);
                
                Task task(encryptedName, priority, deadline);
                task.setCompleted(completed);
                tasks.push_back(task);
            }
        }
        
        cout << "Tasks loaded successfully!" << endl;
    }

    void backupTasksFile() const {
        ifstream source(FILENAME, ios::binary);
        ofstream dest(BACKUP_FILENAME, ios::binary);
        
        if (!source || !dest) {
            cout << "Error creating backup!" << endl;
            return;
        }
        
        dest << source.rdbuf();
        cout << "Backup created successfully!" << endl;
    }

    // Search & Filter
    void searchTaskByName(const string& name) const {
        bool found = false;
        for (size_t i = 0; i < tasks.size(); i++) {
            if (tasks[i].getName().find(name) != string::npos) {
                cout << i + 1 << ". ";
                tasks[i].display();
                found = true;
            }
        }
        if (!found) cout << "No tasks found with name containing '" << name << "'" << endl;
    }

    void filterCompletedTasks() const {
        bool found = false;
        for (size_t i = 0; i < tasks.size(); i++) {
            if (tasks[i].isCompleted()) {
                cout << i + 1 << ". ";
                tasks[i].display();
                found = true;
            }
        }
        if (!found) cout << "No completed tasks found." << endl;
    }

    void filterPendingTasks() const {
        bool found = false;
        for (size_t i = 0; i < tasks.size(); i++) {
            if (!tasks[i].isCompleted()) {
                cout << i + 1 << ". ";
                tasks[i].display();
                found = true;
            }
        }
        if (!found) cout << "No pending tasks found." << endl;
    }

    void sortTasksByName() {
        for (size_t i = 0; i < tasks.size() - 1; i++) {
            for (size_t j = 0; j < tasks.size() - i - 1; j++) {
                if (tasks[j].getName() > tasks[j + 1].getName()) {
                    swap(tasks[j], tasks[j + 1]);
                }
            }
        }
        cout << "Tasks sorted by name." << endl;
    }

    void sortTasksByPriority() {
        for (size_t i = 0; i < tasks.size() - 1; i++) {
            for (size_t j = 0; j < tasks.size() - i - 1; j++) {
                if (tasks[j].getPriority() < tasks[j + 1].getPriority()) {
                    swap(tasks[j], tasks[j + 1]);
                }
            }
        }
        cout << "Tasks sorted by priority." << endl;
    }

    void reverseTaskList() {
        reverse(tasks.begin(), tasks.end());
        cout << "Task list reversed." << endl;
    }

    // Priority & Status
    void setTaskPriority(int index, int priority) {
        if (index < 0 || index >= (int)tasks.size()) {
            cout << "Invalid task index!" << endl;
            return;
        }
        tasks[index].setPriority(priority);
    }

    int getTaskPriority(int index) const {
        if (index < 0 || index >= (int)tasks.size()) return -1;
        return tasks[index].getPriority();
    }

    void changeTaskStatus(int index, bool completed) {
        if (index < 0 || index >= (int)tasks.size()) {
            cout << "Invalid task index!" << endl;
            return;
        }
        tasks[index].setCompleted(completed);
    }

    bool isTaskCompleted(int index) const {
        if (index < 0 || index >= (int)tasks.size()) return false;
        return tasks[index].isCompleted();
    }

    void highlightImportantTasks() const {
        cout << "High Priority Tasks (Priority 4-5):" << endl;
        bool found = false;
        for (size_t i = 0; i < tasks.size(); i++) {
            if (tasks[i].getPriority() >= 4) {
                cout << "⭐ ";
                tasks[i].display();
                found = true;
            }
        }
        if (!found) cout << "No high priority tasks found." << endl;
    }

    int countCompletedTasks() const {
        int count = 0;
        for (const auto& task : tasks) {
            if (task.isCompleted()) count++;
        }
        return count;
    }

    // Date & Time
    void setTaskDeadline(int index, const string& deadline) {
        if (index < 0 || index >= (int)tasks.size()) {
            cout << "Invalid task index!" << endl;
            return;
        }
        if (isValidDate(deadline)) {
            tasks[index].setDeadline(deadline);
        } else {
            cout << "Invalid date format! Use YYYY-MM-DD" << endl;
        }
    }

    void showOverdueTasks() const {
        string currentDate = getCurrentDate();
        cout << "Overdue Tasks:" << endl;
        bool found = false;
        
        for (const auto& task : tasks) {
            if (task.getDeadline() != "0000-00-00" &&
                task.getDeadline() < currentDate &&
                !task.isCompleted()) {
                task.display();
                found = true;
            }
        }
        if (!found) cout << "No overdue tasks found." << endl;
    }

    int daysLeftForTask(int index) const {
        if (index < 0 || index >= (int)tasks.size()) return -1;
        if (tasks[index].getDeadline() == "0000-00-00") return -2;
        
        string currentDate = getCurrentDate();
        
        tm deadline_tm = {}, current_tm = {};
        sscanf(tasks[index].getDeadline().c_str(), "%4d-%2d-%2d",
               &deadline_tm.tm_year, &deadline_tm.tm_mon, &deadline_tm.tm_mday);
        sscanf(currentDate.c_str(), "%4d-%2d-%2d",
               &current_tm.tm_year, &current_tm.tm_mon, &current_tm.tm_mday);
        
        deadline_tm.tm_year -= 1900;
        deadline_tm.tm_mon -= 1;
        current_tm.tm_year -= 1900;
        current_tm.tm_mon -= 1;
        
        time_t deadline_time = mktime(&deadline_tm);
        time_t current_time = mktime(&current_tm);
        
        double difference = difftime(deadline_time, current_time);
        return (int)(difference / (60 * 60 * 24));
    }

    // Security
    bool login() {
        string password;
        cout << "Enter password: ";
        getline(cin, password);
        
        if (password == PASSWORD) {
            loggedIn = true;
            cout << "Login successful!" << endl;
            return true;
        } else {
            cout << "Invalid password!" << endl;
            return false;
        }
    }

    void logout() {
        loggedIn = false;
        cout << "Logged out successfully." << endl;
    }

    void setPassword() {
        string newPassword, confirmPassword;
        cout << "Enter new password: ";
        getline(cin, newPassword);
        cout << "Confirm new password: ";
        getline(cin, confirmPassword);
        
        if (newPassword == confirmPassword) {
            cout << "Password changed successfully!" << endl;
        } else {
            cout << "Passwords don't match!" << endl;
        }
    }

    void run() {
        createFileIfNotExist(FILENAME);
        int choice;
        string input;
        int index, priority;
        string deadline;
        
        while (true) {
            clearScreen();
            printHeader();
            
            if (!loggedIn) {
                if (!login()) {
                    pauseScreen();
                    continue;
                }
            }
            
            showMenu();
            choice = getUserChoice();
            
            switch (choice) {
                case 0:
                    cout << "Goodbye!" << endl;
                    return;
                    
                case 1:
                    cout << "Enter task name: ";
                    getline(cin, input);
                    
                    cout << "Enter priority (1-5): ";
                    cin >> priority;
                    cin.ignore();
                    
                    cout << "Enter deadline (YYYY-MM-DD) or press Enter for none: ";
                    getline(cin, deadline);
                    
                    if (deadline.empty()) deadline = "0000-00-00";
                    
                    addTask(input, priority, deadline);
                    cout << "Task added successfully!" << endl;
                    break;
                    
                case 2:
                    printAllTasksFormatted();
                    break;
                    
                case 3:
                    cout << "Enter task number to delete: ";
                    cin >> index;
                    cin.ignore();
                    deleteTask(index - 1);
                    cout << "Task deleted successfully!" << endl;
                    break;
                    
                case 4:
                    cout << "Enter task number to update: ";
                    cin >> index;
                    cin.ignore();
                    
                    cout << "Enter new task name (or press Enter to keep current): ";
                    getline(cin, input);
                    
                    cout << "Enter new priority (1-5, or 0 to keep current): ";
                    cin >> priority;
                    cin.ignore();
                    
                    cout << "Enter new deadline (YYYY-MM-DD, or press Enter to keep current): ";
                    getline(cin, deadline);
                    
                    updateTask(index - 1, input, priority, deadline);
                    cout << "Task updated successfully!" << endl;
                    break;
                    
                case 5:
                    cout << "Enter task number to mark complete: ";
                    cin >> index;
                    cin.ignore();
                    markComplete(index - 1);
                    cout << "Task marked as complete!" << endl;
                    break;
                    
                case 6:
                    cout << "Enter task number to mark incomplete: ";
                    cin >> index;
                    cin.ignore();
                    markIncomplete(index - 1);
                    cout << "Task marked as incomplete!" << endl;
                    break;
                    
                case 7:
                    cout << "Enter search keyword: ";
                    getline(cin, input);
                    searchTaskByName(input);
                    break;
                    
                case 8:
                    cout << "1. Show Completed\n2. Show Pending\n3. Show High Priority\nChoose: ";
                    cin >> choice;
                    cin.ignore();
                    
                    if (choice == 1) filterCompletedTasks();
                    else if (choice == 2) filterPendingTasks();
                    else if (choice == 3) highlightImportantTasks();
                    else cout << "Invalid choice!" << endl;
                    break;
                    
                case 9:
                    cout << "1. Sort by Name\n2. Sort by Priority\n3. Reverse List\nChoose: ";
                    cin >> choice;
                    cin.ignore();
                    
                    if (choice == 1) sortTasksByName();
                    else if (choice == 2) sortTasksByPriority();
                    else if (choice == 3) reverseTaskList();
                    else cout << "Invalid choice!" << endl;
                    break;
                    
                case 10:
                    cout << "Total tasks: " << countTasks() << endl;
                    cout << "Completed tasks: " << countCompletedTasks() << endl;
                    cout << "Pending tasks: " << countTasks() - countCompletedTasks() << endl;
                    showOverdueTasks();
                    break;
                    
                case 11:
                    saveTasksToFile();
                    break;
                    
                case 12:
                    loadTasksFromFile();
                    break;
                    
                case 13:
                    backupTasksFile();
                    break;
                    
                case 14:
                    setPassword();
                    break;
                    
                case 15:
                    logout();
                    break;
                    
                default:
                    cout << "Invalid choice! Please try again." << endl;
            }
            
            pauseScreen();
        }
    }
};

int main() {
    TodoManager manager;
    manager.run();
    return 0;
}
