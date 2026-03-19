#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>

#define MAX 100
#define PI 3.141592653589793
#define HISTORY_FILE "history.txt"
#define HISTORY_SIZE 10

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif



double cube(double x) { return x * x * x; }
double cbrt_func(double x) { return cbrt(x); }
double log10_func(double x) { return log10(x); }
double ln(double x) { return log(x); }
double log2_func(double x) { return log2(x); }
double abs_val(double x) { return fabs(x); }


double absolute(double x) { return fabs(x); }
double floor_val(double x) { return floor(x); }
double ceil_val(double x) { return ceil(x); }

double sinh_val(double x) { return sinh(x); }
double cosh_val(double x) { return cosh(x); }
double tanh_val(double x) { return tanh(x); }

double asin_val(double x) { return asin(x); }
double acos_val(double x) { return acos(x); }
double atan_val(double x) { return atan(x); }

double log_base(double x, double base) {
    return log(x) / log(base);
}

double power10(double x) { return pow(10, x); }
double exp_val(double x) { return exp(x); }

int signum(double x) {
    if (x > 0) return 1;
    else if (x < 0) return -1;
    return 0;
}

double sum_n(int n) {
    return (n * (n + 1)) / 2.0;
}

// ================= STACK =================
typedef struct {
    double data[MAX];
    int top;
} Stack;

void push(Stack *s, double val) {
    if (s->top >= MAX - 1) {
        printf("Stack Overflow!\n");
        return;
    }
    s->data[++s->top] = val;
}

double pop(Stack *s) {
    if (s->top < 0) {
        printf("Stack Underflow!\n");
        return 0;
    }
    return s->data[s->top--];
}

// ================= MEMORY =================
double memory = 0, ans = 0;

//number theory module 
void print_divisors(int n) {
    printf("Divisors: ");
    for(int i=1;i<=n;i++)
        if(n%i==0) printf("%d ", i);
}

void prime_factors(int n) {
    printf("Prime factors: ");
    for(int i=2;i<=n;i++) {
        while(n%i==0) {
            printf("%d ", i);
            n/=i;
        }
    }
}

int fibonacci(int n) {
    if(n<=1) return n;
    return fibonacci(n-1)+fibonacci(n-2);
}

int isPerfect(int n) {
    int sum=0;
    for(int i=1;i<n;i++)
        if(n%i==0) sum+=i;
    return sum==n;
}

int isArmstrong(int n) {
    int temp=n, sum=0, digits=0;
    while(temp){ digits++; temp/=10; }

    temp=n;
    while(temp){
        int r=temp%10;
        sum+=pow(r,digits);
        temp/=10;
    }
    return sum==n;
}

int isPalindrome(int n) {
    int rev=0, temp=n;
    while(temp){
        rev=rev*10 + temp%10;
        temp/=10;
    }
    return rev==n;
}
// ================= FILE HISTORY =================
void saveHistory(const char *expr, double result) {
    FILE *fp = fopen(HISTORY_FILE, "a");
    if (fp) {
        // for exact time 
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char timeStr[26];

        // Format: Date Month Year Hour:Minute:Second
        strftime(timeStr, 26, "%d-%m-%Y %H:%M:%S", tm_info);

        // File mein [Date Time] Expression = Result ke format mein save ho
        fprintf(fp, "[%s] %s = %.5lf\n", timeStr, expr, result);
        fclose(fp);
    }
}

void showHistory() {
    FILE *fp = fopen(HISTORY_FILE, "r");
    char line[200];
    if (!fp) {
        printf("No history found!\n");
        return;
    }
    while (fgets(line, sizeof(line), fp))
        printf("%s", line);
    fclose(fp);
}

// ================= LOGIN SYSTEM =================
void login() {
    char user[20], pass[20], ch;
    int i = 0;

    printf("Username: ");
    scanf("%s", user);

    printf("Password: ");
    while ((ch = getch()) != '\r') {
        pass[i++] = ch;
        printf("*");
    }
    pass[i] = '\0';

    if (strcmp(user, "codewithdev") == 0 && strcmp(pass, "1234") == 0)
        printf("\nLogin Successful!\n");
    else {
        printf("\nAccess Denied!\n");
        exit(0);
    }
}

// ================= OPERATOR =================
int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    return 0;
}

double applyOp(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b == 0) ? NAN : a / b;
        case '^': return pow(a, b);
    }
    return 0;
}

// ================= INFIX → POSTFIX =================
void infixToPostfix(char *exp, char *output) {
    char stack[MAX];
    int top = -1, k = 0;

    for (int i = 0; exp[i]; i++) {
        if (isdigit(exp[i]) || exp[i] == '.') {
            while (isdigit(exp[i]) || exp[i] == '.')
                output[k++] = exp[i++];
            output[k++] = ' ';
            i--;
        } else if (exp[i] == '(')
            stack[++top] = exp[i];
        else if (exp[i] == ')') {
            while (top >= 0 && stack[top] != '(')
                output[k++] = stack[top--], output[k++] = ' ';
            top--;
        } else {
            while (top >= 0 && precedence(stack[top]) >= precedence(exp[i]))
                output[k++] = stack[top--], output[k++] = ' ';
            stack[++top] = exp[i];
        }
    }
    while (top >= 0)
        output[k++] = stack[top--], output[k++] = ' ';
    output[k] = '\0';
}

// ================= POSTFIX EVAL =================
double evalPostfix(char *exp) {
    Stack s;
    s.top = -1;

    for (int i = 0; exp[i]; i++) {
        if (isdigit(exp[i])) {
            double num = 0;
            while (isdigit(exp[i])) {
                num = num * 10 + (exp[i++] - '0');
            }
            push(&s, num);
        } else if (exp[i] == ' ') continue;
        else {
            double b = pop(&s);
            double a = pop(&s);
            push(&s, applyOp(a, b, exp[i]));
        }
    }
    return pop(&s);
}

// ================= SCIENTIFIC =================
double nthRoot(double x, double n) {
    return pow(x, 1.0 / n);
}

// ================= MATRIX =================
void matrixAdd() {
    int i,j;
    double A[3][3], B[3][3];
    printf("Enter 3x3 Matrix A:\n");
    for(i=0;i<3;i++)
        for(j=0;j<3;j++)
            scanf("%lf",&A[i][j]);

    printf("Enter 3x3 Matrix B:\n");
    for(i=0;i<3;i++)
        for(j=0;j<3;j++)
            scanf("%lf",&B[i][j]);

    printf("Result:\n");
    for(i=0;i<3;i++){
        for(j=0;j<3;j++)
            printf("%.2lf ", A[i][j]+B[i][j]);
        printf("\n");
    }
}

void matrix_subtract(double A[3][3], double B[3][3]) {
    double C[3][3];
    for(int i=0;i<3;i++)
        for(int j=0;j<3;j++)
            C[i][j]=A[i][j]-B[i][j];

    printf("Result:\n");
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++)
            printf("%.2lf ",C[i][j]);
        printf("\n");
    }
}

void transpose(double A[3][3]) {
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++)
            printf("%.2lf ",A[j][i]);
        printf("\n");
    }
}

double det2(double A[2][2]) {
    return A[0][0]*A[1][1] - A[0][1]*A[1][0];
}

double det3(double A[3][3]) {
    return A[0][0]*(A[1][1]*A[2][2]-A[1][2]*A[2][1])
         - A[0][1]*(A[1][0]*A[2][2]-A[1][2]*A[2][0])
         + A[0][2]*(A[1][0]*A[2][1]-A[1][1]*A[2][0]);
}

void matrix_multiply(double A[3][3], double B[3][3]) {
    double C[3][3]={0};
    for(int i=0;i<3;i++)
        for(int j=0;j<3;j++)
            for(int k=0;k<3;k++)
                C[i][j]+=A[i][k]*B[k][j];

    printf("Result:\n");
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++)
            printf("%.2lf ",C[i][j]);
        printf("\n");
    }
}


// ================= ADVANCED NUMBER THEORY =================
int isPrime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i <= sqrt(n); i++)
        if (n % i == 0) return 0;
    return 1;
}




// ================= FACTORIAL, P & C =================
long double factorial(int n) {
    if (n < 0 || n > 20) {
        printf("Limit: 0-20 only!\n");
        return 0;
    }
    long double f = 1;
    for (int i = 1; i <= n; i++) f *= i;
    return f;
}

long double nPr(int n, int r) {
    if (r > n) return 0;
    return factorial(n) / factorial(n - r);
}

long double nCr(int n, int r) {
    if (r > n) return 0;
    return factorial(n) / (factorial(r) * factorial(n - r));
}



// ================= EQUATION SOLVER =================
void solveQuadratic(double a, double b, double c) {
    double d = b * b - 4 * a * c;
    if (d > 0) {
        double r1 = (-b + sqrt(d)) / (2 * a);
        double r2 = (-b - sqrt(d)) / (2 * a);
        printf("Roots: %.2lf and %.2lf\n", r1, r2);
    } else if (d == 0) {
        printf("Equal Root: %.2lf\n", -b / (2 * a));
    } else {
        printf("Complex Roots (No Real Solution)\n");
    }
}
//COMPUTER SCIENCE UTILITIES
void dec_to_binary(int n) {
    int bin[32], i=0;
    while(n>0){
        bin[i++]=n%2;
        n/=2;
    }
    for(int j=i-1;j>=0;j--)
        printf("%d",bin[j]);
}

void dec_to_hex(int n) {
    printf("%X", n);
}

void dec_to_octal(int n) {
    printf("%o", n);
}

void ascii_value(char c) {
    printf("ASCII: %d", c);
}

int left_shift(int x, int shift) {
    return x << shift;
}

int right_shift(int x, int shift) {
    return x >> shift;
}

void generate_password(int length) {
    char charset[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int size=strlen(charset);

    srand(time(NULL));

    for(int i=0;i<length;i++)
        printf("%c", charset[rand()%size]);
}

// ================= UNIT CONVERSIONS (EXTRA) =================
double c_to_f(double c) { return (c * 9 / 5) + 32; }
double f_to_c(double f) { return (f - 32) * 5 / 9; }
double km_to_m(double km) { return km * 1000; }
double usd_to_inr(double usd) { return usd * 83.0; }
double kg_to_pound(double kg) { return kg * 2.20462; }
double gram_to_ounce(double g) { return g * 0.035274; }
double mb_to_gb(double mb) { return mb / 1024.0; }
double bits_to_bytes(double b) { return b / 8.0; }
double kmh_to_ms(double kmh) { return kmh * 5.0/18.0; }

// ================= UTILS =================
int randomRange(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// ================= STATISTICS =================
double mean(double arr[], int n){
    double s=0;
    for(int i=0;i<n;i++) s+=arr[i];
    return s/n;
}

double variance(double arr[], int n){
    double m=mean(arr,n), s=0;
    for(int i=0;i<n;i++)
        s+=pow(arr[i]-m,2);
    return s/n;
}

void sort(double arr[], int n) {
    for(int i=0;i<n-1;i++)
        for(int j=i+1;j<n;j++)
            if(arr[i]>arr[j]) {
                double t=arr[i]; arr[i]=arr[j]; arr[j]=t;
            }
}

double median(double arr[], int n) {
    sort(arr,n);
    if(n%2==0)
        return (arr[n/2-1]+arr[n/2])/2;
    return arr[n/2];
}

double mode(double arr[], int n) {
    int maxCount=0;
    double mode=arr[0];

    for(int i=0;i<n;i++){
        int count=0;
        for(int j=0;j<n;j++)
            if(arr[j]==arr[i]) count++;

        if(count>maxCount){
            maxCount=count;
            mode=arr[i];
        }
    }
    return mode;
}

double stddev(double arr[], int n) {
    double mean=0, var=0;
    for(int i=0;i<n;i++) mean+=arr[i];
    mean/=n;

    for(int i=0;i<n;i++)
        var += pow(arr[i]-mean,2);

    return sqrt(var/n);
}

double range(double arr[], int n) {
    sort(arr,n);
    return arr[n-1]-arr[0];
}

// ================= NUMBER THEORY =================
int gcd(int a,int b){
    while(b){int t=b;b=a%b;a=t;}
    return a;
}

// ================= UNIT =================
double bytesToKB(double b){return b/1024;}
double jouleToCal(double j){return j/4.184;}

// ================= MAIN MENU =================
void menu(){
    printf("\n==== UNIVERSAL CALCULATOR ====\n");
    printf("1.  Expression Solver (e.g. 2+3*5)\n");
    printf("2.  Scientific (Cube, Log, nCr, Fact)\n");
    printf("3.  Matrix Operations (Add, Sub, Mul, Det)\n");
    printf("4.  Statistics (Mean, Median, StdDev, Range)\n");
    printf("5.  Number Theory (GCD, LCM, Prime)\n");
    printf("6.  Unit Converter (Bytes, Temp, Distance)\n");
    printf("7.  Trigonometry (Sin, Cos, Tan, etc.)\n");
    printf("8.  Bitwise Operations (AND, OR, XOR)\n");
    printf("9.  Quadratic Equation Solver\n");
    printf("10. Computer Science (Binary, Hex, Password Gen)\n");
    printf("11. Advanced Math (Fibonacci, Sum of N)\n");
    printf("12. View History\n");
    printf("0.  Exit\n");
    printf("Enter choice: ");
}



// TRIGONOMETRY (cal.c se)
double degToRad(double d) { return d * PI / 180.0; }
double sin_deg(double x) { return sin(degToRad(x)); }
double cos_deg(double x) { return cos(degToRad(x)); }
double tan_deg(double x) { return tan(degToRad(x)); }
double cot_deg(double x) { return 1 / tan(degToRad(x)); }
double sec_deg(double x) { return 1 / cos(degToRad(x)); }
double cosec_deg(double x) { return 1 / sin(degToRad(x)); }



// BITWISE (cal.c se)
int bit_and(int a, int b) { return a & b; }
int bit_or(int a, int b) { return a | b; }
int bit_xor(int a, int b) { return a ^ b; }

// NUMBER THEORY (Missing LCM)
int lcm(int a, int b) { return (a * b) / gcd(a, b); }

// ================= MAIN =================
int main(){
    system("color 0A");
    char expr[100], postfix[100];
    int ch;

    login();

    while(1){
        
        menu();
        
        // Input validation logic
        if (scanf("%d", &ch) != 1) { 
            // Agar user ne alphabet ya koi wrong character input hua to
            printf("\n[Error] only numbers (0-12) allow!\n");
            
            // Clear the input buffer (important to stop infinite loop)
            while (getchar() != '\n'); 
            
            continue; 
        }
        
        getchar();

        if(ch == 0) break;

        switch(ch){
            case 1: { 
                printf("Enter Expression: ");
                fgets(expr, 100, stdin);
                expr[strcspn(expr, "\n")] = 0; 
                infixToPostfix(expr, postfix);
                ans = evalPostfix(postfix);
                printf("Result = %.5lf\n", ans);
                saveHistory(expr, ans);
                break;
            }
            // Scientific Functions

            case 2: {
               printf("1.Cube 2.Cbrt 3.Log10 4.Ln 5.Fact 6.nPr 7.nCr: ");
               int s_ch; scanf("%d", &s_ch);
               double v1, v2;
               if(s_ch >= 6) { printf("Enter n and r: "); scanf("%lf %lf", &v1, &v2); }
               else { printf("Enter value: "); scanf("%lf", &v1); }

               if(s_ch==1) ans = cube(v1);
               else if(s_ch==2) ans = cbrt_func(v1);
               else if(s_ch==3) ans = log10_func(v1);
               else if(s_ch==4) ans = ln(v1);
               else if(s_ch==5) ans = (double)factorial((int)v1);
               else if(s_ch==6) ans = (double)nPr((int)v1, (int)v2);
               else if(s_ch==7) ans = (double)nCr((int)v1, (int)v2);

               printf("Result: %.5lf\n", ans);
               char histExpr[50];
               sprintf(histExpr, "Scientific_Op(%.2lf)", v1); 
               saveHistory(histExpr, ans);
               break;
}

           //matrix logic
            case 3: { 
                int m_ch;
                double A[3][3], B[3][3];
                printf("1.Add 2.Sub 3.Mul 4.Transpose 5.Det: ");
                scanf("%d", &m_ch);
    
                printf("Enter Matrix A (3x3):\n");
                for(int i=0; i<3; i++) for(int j=0; j<3; j++) scanf("%lf", &A[i][j]);

                if(m_ch <= 3) { // Matrix B only for Add, Sub, Mul
                    printf("Enter Matrix B (3x3):\n");
                    for(int i=0; i<3; i++) for(int j=0; j<3; j++) scanf("%lf", &B[i][j]);
                }

                if(m_ch==1) { // Simple loop for addition
                    printf("Result:\n");
                    for(int i=0;i<3;i++){
                        for(int j=0;j<3;j++) printf("%.2lf ", A[i][j]+B[i][j]);
                        printf("\n");
                    }
                }
                else if(m_ch==2) matrix_subtract(A, B);
                else if(m_ch==3) matrix_multiply(A, B);
                else if(m_ch==4) transpose(A);
                else if(m_ch==5) printf("Determinant: %.2lf\n", det3(A));
                char histExpr[50];
                strcpy(histExpr, "Matrix Operation"); 
                saveHistory(histExpr, 0);
                break;
            }

                // Statistics
            case 4: {
                int n, st_ch;
                printf("Enter n: "); scanf("%d",&n);
                double arr[n];
                printf("Enter %d numbers: ", n);
                for(int i=0; i<n; i++) scanf("%lf", &arr[i]);
                printf("1.Mean/Var 2.Median 3.Mode 4.StdDev 5.Range: ");
                scanf("%d", &st_ch);
                if(st_ch==1) printf("Mean=%.2lf Var=%.2lf\n", mean(arr,n), variance(arr,n));
                else if(st_ch==2) printf("Median=%.2lf\n", median(arr,n));
                else if(st_ch==3) printf("Mode=%.2lf\n", mode(arr,n));
                else if(st_ch==4) printf("StdDev=%.2lf\n", stddev(arr,n));
                else if(st_ch==5) printf("Range=%.2lf\n", range(arr,n));
                char histExpr[50];
                sprintf(histExpr, "Stats_n(%d)", n); 
                saveHistory(histExpr, (double)n);
                break;
                }
            // Number Theory (GCD, LCM, Prime)
            case 5: { 
                int a, b, nt;
                printf("1.GCD 2.LCM 3.Prime: "); scanf("%d", &nt);
                if(nt==3){ printf("Num: "); scanf("%d",&a); printf(isPrime(a)?"Prime\n":"Not\n"); }
                else { printf("a b: "); scanf("%d %d",&a,&b); printf("Res: %d\n", (nt==1)?gcd(a,b):lcm(a,b)); }
                char histExpr[50];
                sprintf(histExpr, "NumTheory_n(%d)", a); 
                saveHistory(histExpr, (double)a);
                break;
            }


            // Unit Converter
            case 6: { 
                printf("1.Bytes->KB 2.C->F 3.KM->M: ");
                int u_ch; double uv; scanf("%d", &u_ch);
                printf("Value: "); scanf("%lf", &uv);
                if(u_ch==1) printf("%.2lf KB\n", bytesToKB(uv));
                else if(u_ch==2) printf("%.2lf F\n", c_to_f(uv));
                else if(u_ch==3) printf("%.2lf M\n", km_to_m(uv));
                char histExpr[50];
                sprintf(histExpr, "Unit_Conv(%.2lf)", uv); 
                saveHistory(histExpr, (double)uv);
                break;
            }
            // Trigonometry
            case 7: {
                printf("Enter degree: "); 
                double deg; 
                scanf("%lf", &deg);
                printf("Sin: %.2lf | Cos: %.2lf | Tan: %.2lf\n", sin_deg(deg), cos_deg(deg), tan_deg(deg));
                printf("Cot: %.2lf | Sec: %.2lf | Cosec: %.2lf\n", cot_deg(deg), sec_deg(deg), cosec_deg(deg));
                char histExpr[50];
                sprintf(histExpr, "Trig_Op(%.2lf deg)", deg); 
                saveHistory(histExpr, sin_deg(deg));
                break;
               }
            // Bitwise
            case 8: {
                int b1, b2, bw_ch;
                printf("Enter two integers: "); 
                if(scanf("%d %d", &b1, &b2) != 2) {
                while(getchar() != '\n');
                
                printf("Invalid input!\n");

                break;
                }
    
               printf("1.AND 2.OR 3.XOR: ");
               scanf("%d", &bw_ch);

               char opName[10];
            if(bw_ch == 1) {
               ans = (double)(b1 & b2);
               strcpy(opName, "AND");
               } else if(bw_ch == 2) {
                ans = (double)(b1 | b2);
                strcpy(opName, "OR");
                } else {
                    ans = (double)(b1 ^ b2);
               strcpy(opName, "XOR");
               }

            printf("%s Result: %.0lf\n", opName, ans);

               char histExpr[50];
                sprintf(histExpr, "Bitwise_%s(%d, %d)", opName, b1, b2); 
                saveHistory(histExpr, ans); 
                break;
}
            // Quadratic Equation Solver

            case 9: {
               double qa, qb, qc; 
               printf("Enter coefficients a, b, and c (ax^2 + bx + c = 0): "); 
               if(scanf("%lf %lf %lf", &qa, &qb, &qc) != 3) {
               while(getchar() != '\n');
               printf("Invalid input! Please enter numbers.\n");
               break;
               }

            if(qa == 0) {
                printf("Not a quadratic equation (a cannot be 0).\n");
                break;
                      }
                solveQuadratic(qa, qb, qc);
                char histExpr[60];
                sprintf(histExpr, "Quadratic_Eq: %.1lfx^2 + %.1lfx + %.1lf", qa, qb, qc); 
                saveHistory(histExpr, 0); 
                break;
                    }

            case 10: { // Computer Science Utilities
                int cs_ch, n;
                printf("1.Binary 2.Hex 3.Octal 4.Password Gen: ");
                scanf("%d", &cs_ch);
                if(cs_ch == 4) { 
                printf("Length: "); scanf("%d", &n); 
                printf("Password: "); generate_password(n); printf("\n");
                } else { 
                printf("Number: "); scanf("%d", &n);
                if(cs_ch==1) dec_to_binary(n);
                else if(cs_ch==2) dec_to_hex(n);
                else dec_to_octal(n);
                printf("\n");
                }
                char histExpr[50];
                sprintf(histExpr, "CS_Utility_Op(%d)", n);
                saveHistory(histExpr, (double)n);
                break;
           }
            case 11: { 
               int m_ch, n;
               printf("1.Fibonacci 2.Sum of N: ");
               scanf("%d", &m_ch);
                printf("Enter n: "); scanf("%d", &n);
    
               if(m_ch==1) {
                     ans = (double)fibonacci(n);
                     printf("Fibonacci term: %.0lf\n", ans);
                     saveHistory("Fibonacci", ans);
              } else {
                    ans = sum_n(n);
                    printf("Sum: %.0lf\n", ans);
                    saveHistory("Sum of N", ans);
                }
                break;
}
            
            // History
            case 12:{
                showHistory();
                break;    
            }
            
        default:
            printf("Invalid!\n");
        }
    }

    return 0;
}