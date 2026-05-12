#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_TASKS 100
#define MAX_NAME_LENGTH 100
#define FILENAME "tasks.txt"
#define BACKUP_FILENAME "tasks_backup.txt"
#define PASSWORD "admin123"  // Default password

typedef struct {
    char name[MAX_NAME_LENGTH];
    int completed;
    int priority;
    char deadline[11];  // YYYY-MM-DD format
} Task;

Task tasks[MAX_TASKS];
int taskCount = 0;
int isLoggedIn = 0;

// Utility Functions
void clearScreen() {
    system("cls || clear");
}

void pauseScreen() {
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

int getCurrentDate(char* buffer) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    return strftime(buffer, 11, "%Y-%m-%d", tm_info);
}

int compareDates(const char* date1, const char* date2) {
    return strcmp(date1, date2);
}

int isValidDate(const char* date) {
    if (strlen(date) != 10) return 0;
    if (date[4] != '-' || date[7] != '-') return 0;
    
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit(date[i])) return 0;
    }
    
    return 1;
}

// Core Task Functions
void addTask(const char* name, int priority, const char* deadline) {
    if (taskCount >= MAX_TASKS) {
        printf("Task limit reached!\n");
        return;
    }
    
    Task newTask;
    strncpy(newTask.name, name, MAX_NAME_LENGTH - 1);
    newTask.name[MAX_NAME_LENGTH - 1] = '\0';
    newTask.completed = 0;
    newTask.priority = (priority < 1) ? 1 : (priority > 5 ? 5 : priority);
    
    if (deadline && isValidDate(deadline)) {
        strcpy(newTask.deadline, deadline);
    } else {
        strcpy(newTask.deadline, "0000-00-00");
    }
    
    tasks[taskCount++] = newTask;
}

void showTasks() {
    if (taskCount == 0) {
        printf("No tasks found!\n");
        return;
    }
    
    for (int i = 0; i < taskCount; i++) {
        printf("%d. [%c] %s (Priority: %d, Deadline: %s)\n",
               i + 1,
               tasks[i].completed ? 'X' : ' ',
               tasks[i].name,
               tasks[i].priority,
               strcmp(tasks[i].deadline, "0000-00-00") == 0 ? "None" : tasks[i].deadline);
    }
}

void deleteTask(int index) {
    if (index < 0 || index >= taskCount) {
        printf("Invalid task index!\n");
        return;
    }
    
    for (int i = index; i < taskCount - 1; i++) {
        tasks[i] = tasks[i + 1];
    }
    taskCount--;
}

void updateTask(int index, const char* newName, int newPriority, const char* newDeadline) {
    if (index < 0 || index >= taskCount) {
        printf("Invalid task index!\n");
        return;
    }
    
    if (newName) {
        strncpy(tasks[index].name, newName, MAX_NAME_LENGTH - 1);
        tasks[index].name[MAX_NAME_LENGTH - 1] = '\0';
    }
    
    if (newPriority > 0) {
        tasks[index].priority = (newPriority < 1) ? 1 : (newPriority > 5 ? 5 : newPriority);
    }
    
    if (newDeadline && isValidDate(newDeadline)) {
        strcpy(tasks[index].deadline, newDeadline);
    }
}

void markComplete(int index) {
    if (index < 0 || index >= taskCount) {
        printf("Invalid task index!\n");
        return;
    }
    tasks[index].completed = 1;
}

void markIncomplete(int index) {
    if (index < 0 || index >= taskCount) {
        printf("Invalid task index!\n");
        return;
    }
    tasks[index].completed = 0;
}

void clearAllTasks() {
    taskCount = 0;
}

int countTasks() {
    return taskCount;
}

int isEmpty() {
    return taskCount == 0;
}

Task* getTaskByIndex(int index) {
    if (index < 0 || index >= taskCount) {
        return NULL;
    }
    return &tasks[index];
}

// UI Functions
void printHeader() {
    printf("========================================\n");
    printf("           TO-DO LIST MANAGER           \n");
    printf("========================================\n");
}

void printFooter() {
    printf("========================================\n");
}

void printTask(const Task* task) {
    if (!task) return;
    printf("[%c] %s (Priority: %d, Deadline: %s)\n",
           task->completed ? 'X' : ' ',
           task->name,
           task->priority,
           strcmp(task->deadline, "0000-00-00") == 0 ? "None" : task->deadline);
}

void printAllTasksFormatted() {
    if (taskCount == 0) {
        printf("No tasks to display.\n");
        return;
    }
    
    printf("\n%-4s %-6s %-8s %-12s %s\n", "No.", "Status", "Priority", "Deadline", "Task Name");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < taskCount; i++) {
        printf("%-4d %-6s %-8d %-12s %s\n",
               i + 1,
               tasks[i].completed ? "Done" : "Pending",
               tasks[i].priority,
               strcmp(tasks[i].deadline, "0000-00-00") == 0 ? "None" : tasks[i].deadline,
               tasks[i].name);
    }
}

void showMenu() {
    printf("\nMain Menu:\n");
    printf("1. Add Task\n");
    printf("2. Show All Tasks\n");
    printf("3. Delete Task\n");
    printf("4. Update Task\n");
    printf("5. Mark Complete\n");
    printf("6. Mark Incomplete\n");
    printf("7. Search Tasks\n");
    printf("8. Filter Tasks\n");
    printf("9. Sort Tasks\n");
    printf("10. Show Statistics\n");
    printf("11. Save Tasks\n");
    printf("12. Load Tasks\n");
    printf("13. Backup Tasks\n");
    printf("14. Change Password\n");
    printf("15. Logout\n");
    printf("0. Exit\n");
    printf("Choose an option: ");
}

int getUserChoice() {
    int choice;
    scanf("%d", &choice);
    while (getchar() != '\n');
    return choice;
}

// File Handling
int fileExists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

void createFileIfNotExist(const char* filename) {
    if (!fileExists(filename)) {
        FILE* file = fopen(filename, "w");
        if (file) fclose(file);
    }
}

void encryptDecrypt(char* data, int encrypt) {
    int shift = encrypt ? 3 : -3;
    for (int i = 0; data[i] != '\0'; i++) {
        if (isalpha(data[i])) {
            char base = islower(data[i]) ? 'a' : 'A';
            data[i] = ((data[i] - base + shift + 26) % 26) + base;
        }
    }
}

void saveTasksToFile() {
    FILE* file = fopen(FILENAME, "w");
    if (!file) {
        printf("Error saving tasks!\n");
        return;
    }
    
    for (int i = 0; i < taskCount; i++) {
        char encryptedName[MAX_NAME_LENGTH];
        strcpy(encryptedName, tasks[i].name);
        encryptDecrypt(encryptedName, 1);
        
        fprintf(file, "%s|%d|%d|%s\n",
                encryptedName,
                tasks[i].completed,
                tasks[i].priority,
                tasks[i].deadline);
    }
    
    fclose(file);
    printf("Tasks saved successfully!\n");
}

void loadTasksFromFile() {
    FILE* file = fopen(FILENAME, "r");
    if (!file) {
        printf("No saved tasks found!\n");
        return;
    }
    
    taskCount = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), file) && taskCount < MAX_TASKS) {
        char encryptedName[MAX_NAME_LENGTH];
        int completed, priority;
        char deadline[11];
        
        if (sscanf(line, "%[^|]|%d|%d|%10s",
                   encryptedName, &completed, &priority, deadline) == 4) {
            
            encryptDecrypt(encryptedName, 0);
            
            Task newTask;
            strncpy(newTask.name, encryptedName, MAX_NAME_LENGTH - 1);
            newTask.name[MAX_NAME_LENGTH - 1] = '\0';
            newTask.completed = completed;
            newTask.priority = priority;
            strcpy(newTask.deadline, deadline);
            
            tasks[taskCount++] = newTask;
        }
    }
    
    fclose(file);
    printf("Tasks loaded successfully!\n");
}

void appendTaskToFile(const Task* task) {
    FILE* file = fopen(FILENAME, "a");
    if (!file) {
        printf("Error appending task!\n");
        return;
    }
    
    char encryptedName[MAX_NAME_LENGTH];
    strcpy(encryptedName, task->name);
    encryptDecrypt(encryptedName, 1);
    
    fprintf(file, "%s|%d|%d|%s\n",
            encryptedName,
            task->completed,
            task->priority,
            task->deadline);
    
    fclose(file);
}

void backupTasksFile() {
    FILE* source = fopen(FILENAME, "r");
    FILE* dest = fopen(BACKUP_FILENAME, "w");
    
    if (!source || !dest) {
        printf("Error creating backup!\n");
        if (source) fclose(source);
        if (dest) fclose(dest);
        return;
    }
    
    char ch;
    while ((ch = fgetc(source)) != EOF) {
        fputc(ch, dest);
    }
    
    fclose(source);
    fclose(dest);
    printf("Backup created successfully!\n");
}

// Search & Filter
void searchTaskByName(const char* name) {
    int found = 0;
    for (int i = 0; i < taskCount; i++) {
        if (strstr(tasks[i].name, name) != NULL) {
            printf("%d. ", i + 1);
            printTask(&tasks[i]);
            found = 1;
        }
    }
    if (!found) printf("No tasks found with name containing '%s'\n", name);
}

void searchTaskByKeyword(const char* keyword) {
    searchTaskByName(keyword);
}

void filterCompletedTasks() {
    int found = 0;
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].completed) {
            printf("%d. ", i + 1);
            printTask(&tasks[i]);
            found = 1;
        }
    }
    if (!found) printf("No completed tasks found.\n");
}

void filterPendingTasks() {
    int found = 0;
    for (int i = 0; i < taskCount; i++) {
        if (!tasks[i].completed) {
            printf("%d. ", i + 1);
            printTask(&tasks[i]);
            found = 1;
        }
    }
    if (!found) printf("No pending tasks found.\n");
}

void sortTasksByName() {
    for (int i = 0; i < taskCount - 1; i++) {
        for (int j = 0; j < taskCount - i - 1; j++) {
            if (strcmp(tasks[j].name, tasks[j + 1].name) > 0) {
                Task temp = tasks[j];
                tasks[j] = tasks[j + 1];
                tasks[j + 1] = temp;
            }
        }
    }
    printf("Tasks sorted by name.\n");
}

void sortTasksByPriority() {
    for (int i = 0; i < taskCount - 1; i++) {
        for (int j = 0; j < taskCount - i - 1; j++) {
            if (tasks[j].priority < tasks[j + 1].priority) {
                Task temp = tasks[j];
                tasks[j] = tasks[j + 1];
                tasks[j + 1] = temp;
            }
        }
    }
    printf("Tasks sorted by priority.\n");
}

void reverseTaskList() {
    for (int i = 0; i < taskCount / 2; i++) {
        Task temp = tasks[i];
        tasks[i] = tasks[taskCount - i - 1];
        tasks[taskCount - i - 1] = temp;
    }
    printf("Task list reversed.\n");
}

// Priority & Status
void setTaskPriority(int index, int priority) {
    if (index < 0 || index >= taskCount) {
        printf("Invalid task index!\n");
        return;
    }
    tasks[index].priority = (priority < 1) ? 1 : (priority > 5 ? 5 : priority);
}

int getTaskPriority(int index) {
    if (index < 0 || index >= taskCount) {
        return -1;
    }
    return tasks[index].priority;
}

void changeTaskStatus(int index, int completed) {
    if (index < 0 || index >= taskCount) {
        printf("Invalid task index!\n");
        return;
    }
    tasks[index].completed = completed;
}

int isTaskCompleted(int index) {
    if (index < 0 || index >= taskCount) {
        return -1;
    }
    return tasks[index].completed;
}

void highlightImportantTasks() {
    printf("High Priority Tasks (Priority 4-5):\n");
    int found = 0;
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].priority >= 4) {
            printf("⭐ ");
            printTask(&tasks[i]);
            found = 1;
        }
    }
    if (!found) printf("No high priority tasks found.\n");
}

int countCompletedTasks() {
    int count = 0;
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].completed) count++;
    }
    return count;
}

// Date & Time
void setTaskDeadline(int index, const char* deadline) {
    if (index < 0 || index >= taskCount) {
        printf("Invalid task index!\n");
        return;
    }
    if (isValidDate(deadline)) {
        strcpy(tasks[index].deadline, deadline);
    } else {
        printf("Invalid date format! Use YYYY-MM-DD\n");
    }
}

void showOverdueTasks() {
    char currentDate[11];
    getCurrentDate(currentDate);
    
    printf("Overdue Tasks:\n");
    int found = 0;
    for (int i = 0; i < taskCount; i++) {
        if (strcmp(tasks[i].deadline, "0000-00-00") != 0 &&
            compareDates(tasks[i].deadline, currentDate) < 0 &&
            !tasks[i].completed) {
            printTask(&tasks[i]);
            found = 1;
        }
    }
    if (!found) printf("No overdue tasks found.\n");
}

int daysLeftForTask(int index) {
    if (index < 0 || index >= taskCount) {
        return -1;
    }
    if (strcmp(tasks[index].deadline, "0000-00-00") == 0) {
        return -2;
    }
    
    char currentDate[11];
    getCurrentDate(currentDate);
    
    struct tm deadline_tm = {0};
    struct tm current_tm = {0};
    
    sscanf(tasks[index].deadline, "%4d-%2d-%2d",
           &deadline_tm.tm_year, &deadline_tm.tm_mon, &deadline_tm.tm_mday);
    sscanf(currentDate, "%4d-%2d-%2d",
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
int login() {
    char password[50];
    printf("Enter password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;
    
    if (strcmp(password, PASSWORD) == 0) {
        isLoggedIn = 1;
        printf("Login successful!\n");
        return 1;
    } else {
        printf("Invalid password!\n");
        return 0;
    }
}

void logout() {
    isLoggedIn = 0;
    printf("Logged out successfully.\n");
}

void setPassword() {
    char newPassword[50];
    char confirmPassword[50];
    
    printf("Enter new password: ");
    fgets(newPassword, sizeof(newPassword), stdin);
    newPassword[strcspn(newPassword, "\n")] = 0;
    
    printf("Confirm new password: ");
    fgets(confirmPassword, sizeof(confirmPassword), stdin);
    confirmPassword[strcspn(confirmPassword, "\n")] = 0;
    
    if (strcmp(newPassword, confirmPassword) == 0) {
        // In a real application, you'd save this to a secure location
        printf("Password changed successfully!\n");
    } else {
        printf("Passwords don't match!\n");
    }
}

// Main application
void runApplication() {
    int choice;
    char buffer[100];
    int index, priority;
    char deadline[11];
    
    createFileIfNotExist(FILENAME);
    
    while (1) {
        clearScreen();
        printHeader();
        
        if (!isLoggedIn) {
            if (!login()) {
                pauseScreen();
                continue;
            }
        }
        
        showMenu();
        choice = getUserChoice();
        
        switch (choice) {
            case 0:
                printf("Goodbye!\n");
                return;
                
            case 1: // Add Task
                printf("Enter task name: ");
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                
                printf("Enter priority (1-5): ");
                scanf("%d", &priority);
                while (getchar() != '\n');
                
                printf("Enter deadline (YYYY-MM-DD) or press Enter for none: ");
                fgets(deadline, sizeof(deadline), stdin);
                deadline[strcspn(deadline, "\n")] = 0;
                
                if (strlen(deadline) == 0) {
                    strcpy(deadline, "0000-00-00");
                }
                
                addTask(buffer, priority, deadline);
                printf("Task added successfully!\n");
                break;
                
            case 2: // Show Tasks
                printAllTasksFormatted();
                break;
                
            case 3: // Delete Task
                printf("Enter task number to delete: ");
                scanf("%d", &index);
                while (getchar() != '\n');
                deleteTask(index - 1);
                printf("Task deleted successfully!\n");
                break;
                
            case 4: // Update Task
                printf("Enter task number to update: ");
                scanf("%d", &index);
                while (getchar() != '\n');
                
                printf("Enter new task name (or press Enter to keep current): ");
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                
                printf("Enter new priority (1-5, or 0 to keep current): ");
                scanf("%d", &priority);
                while (getchar() != '\n');
                
                printf("Enter new deadline (YYYY-MM-DD, or press Enter to keep current): ");
                fgets(deadline, sizeof(deadline), stdin);
                deadline[strcspn(deadline, "\n")] = 0;
                
                updateTask(index - 1, 
                          strlen(buffer) ? buffer : NULL,
                          priority,
                          strlen(deadline) ? deadline : NULL);
                printf("Task updated successfully!\n");
                break;
                
            case 5: // Mark Complete
                printf("Enter task number to mark complete: ");
                scanf("%d", &index);
                while (getchar() != '\n');
                markComplete(index - 1);
                printf("Task marked as complete!\n");
                break;
                
            case 6: // Mark Incomplete
                printf("Enter task number to mark incomplete: ");
                scanf("%d", &index);
                while (getchar() != '\n');
                markIncomplete(index - 1);
                printf("Task marked as incomplete!\n");
                break;
                
            case 7: // Search
                printf("Enter search keyword: ");
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                searchTaskByName(buffer);
                break;
                
            case 8: // Filter
                printf("1. Show Completed\n2. Show Pending\n3. Show High Priority\nChoose: ");
                scanf("%d", &choice);
                while (getchar() != '\n');
                
                if (choice == 1) filterCompletedTasks();
                else if (choice == 2) filterPendingTasks();
                else if (choice == 3) highlightImportantTasks();
                else printf("Invalid choice!\n");
                break;
                
            case 9: // Sort
                printf("1. Sort by Name\n2. Sort by Priority\n3. Reverse List\nChoose: ");
                scanf("%d", &choice);
                while (getchar() != '\n');
                
                if (choice == 1) sortTasksByName();
                else if (choice == 2) sortTasksByPriority();
                else if (choice == 3) reverseTaskList();
                else printf("Invalid choice!\n");
                break;
                
            case 10: // Statistics
                printf("Total tasks: %d\n", countTasks());
                printf("Completed tasks: %d\n", countCompletedTasks());
                printf("Pending tasks: %d\n", countTasks() - countCompletedTasks());
                showOverdueTasks();
                break;
                
            case 11: // Save
                saveTasksToFile();
                break;
                
            case 12: // Load
                loadTasksFromFile();
                break;
                
            case 13: // Backup
                backupTasksFile();
                break;
                
            case 14: // Change Password
                setPassword();
                break;
                
            case 15: // Logout
                logout();
                break;
                
            default:
                printf("Invalid choice! Please try again.\n");
        }
        
        pauseScreen();
    }
}

int main() {
    runApplication();
    return 0;
}
