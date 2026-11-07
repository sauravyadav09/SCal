/*
 * ============================================================
 *   Scientific Calculator — Pure C Console Application
 *   Author : sauravyadav09
 *   Build  : gcc calculator.c -o calculator -lm
 *            (Windows MSVC: cl calculator.c)
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* ──────────── Colour helpers (ANSI escape codes) ──────────── */
#ifdef _WIN32
#include <windows.h>
static void enable_ansi(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | 0x0004 /*ENABLE_VIRTUAL_TERMINAL_PROCESSING*/);
}
#else
static void enable_ansi(void) { /* ANSI supported natively */ }
#endif

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"
#define MAGENTA "\033[35m"
#define WHITE   "\033[97m"
#define BG_DARK "\033[48;5;235m"

/* ──────────── Constants ──────────── */
#define PI          3.14159265358979323846
#define EULER       2.71828182845904523536
#define MAX_HIST    20
#define EXPR_LEN    256

/* ──────────── History ──────────── */
typedef struct {
    char description[128];
    double result;
} HistoryEntry;

static HistoryEntry history[MAX_HIST];
static int hist_count = 0;

static void add_history(const char *desc, double result) {
    if (hist_count < MAX_HIST) {
        strncpy(history[hist_count].description, desc, 127);
        history[hist_count].description[127] = '\0';
        history[hist_count].result = result;
        hist_count++;
    } else {
        /* Shift left */
        for (int i = 1; i < MAX_HIST; i++) history[i - 1] = history[i];
        strncpy(history[MAX_HIST - 1].description, desc, 127);
        history[MAX_HIST - 1].description[127] = '\0';
        history[MAX_HIST - 1].result = result;
    }
}

/* ──────────── Utility ──────────── */
static int angle_mode = 0; /* 0 = DEG, 1 = RAD */

static double to_rad(double x) {
    return angle_mode ? x : x * PI / 180.0;
}
static double to_angle(double rad) {
    return angle_mode ? rad : rad * 180.0 / PI;
}

static double factorial(int n) {
    if (n < 0 || n > 170) return -1;
    double r = 1;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
}

static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void pause_screen(void) {
    printf("\n" DIM "  Press Enter to continue..." RESET);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static double read_number(const char *prompt) {
    double val;
    printf(CYAN "  %s" RESET, prompt);
    while (scanf("%lf", &val) != 1) {
        while (getchar() != '\n');
        printf(RED "  Invalid input. Try again: " RESET);
    }
    while (getchar() != '\n');
    return val;
}

/* ──────────── Expression parser (supports +, -, *, /, ^, (), unary -) ──────────── */
/* Recursive descent parser */
static const char *expr_ptr;
static int expr_error;

static double parse_expr(void);

static void skip_spaces(void) {
    while (*expr_ptr == ' ') expr_ptr++;
}

static double parse_number(void) {
    skip_spaces();
    /* Handle pi and e constants */
    if (strncmp(expr_ptr, "pi", 2) == 0 && !isalpha((unsigned char)expr_ptr[2])) {
        expr_ptr += 2;
        return PI;
    }
    if (*expr_ptr == 'e' && !isalpha((unsigned char)expr_ptr[1]) && !isdigit((unsigned char)expr_ptr[1]) && expr_ptr[1] != '.') {
        expr_ptr++;
        return EULER;
    }
    double val;
    char *end;
    val = strtod(expr_ptr, &end);
    if (end == expr_ptr) {
        expr_error = 1;
        return 0;
    }
    expr_ptr = end;
    return val;
}

static double parse_factor(void) {
    skip_spaces();
    /* Unary minus */
    if (*expr_ptr == '-') {
        expr_ptr++;
        return -parse_factor();
    }
    /* Unary plus */
    if (*expr_ptr == '+') {
        expr_ptr++;
        return parse_factor();
    }
    /* Parentheses */
    if (*expr_ptr == '(') {
        expr_ptr++;
        double val = parse_expr();
        skip_spaces();
        if (*expr_ptr == ')') expr_ptr++;
        else expr_error = 1;
        return val;
    }
    return parse_number();
}

static double parse_power(void) {
    double base = parse_factor();
    skip_spaces();
    if (*expr_ptr == '^') {
        expr_ptr++;
        double exp = parse_factor(); /* right-associative */
        base = pow(base, exp);
    }
    return base;
}

static double parse_term(void) {
    double val = parse_power();
    skip_spaces();
    while (*expr_ptr == '*' || *expr_ptr == '/' || *expr_ptr == '%') {
        char op = *expr_ptr++;
        double right = parse_power();
        if (op == '*') val *= right;
        else if (op == '/') {
            if (right == 0) { expr_error = 1; return 0; }
            val /= right;
        } else {
            if (right == 0) { expr_error = 1; return 0; }
            val = fmod(val, right);
        }
        skip_spaces();
    }
    return val;
}

static double parse_expr(void) {
    double val = parse_term();
    skip_spaces();
    while (*expr_ptr == '+' || *expr_ptr == '-') {
        char op = *expr_ptr++;
        double right = parse_term();
        if (op == '+') val += right;
        else val -= right;
        skip_spaces();
    }
    return val;
}

static int evaluate_expression(const char *input, double *result) {
    expr_ptr = input;
    expr_error = 0;
    *result = parse_expr();
    skip_spaces();
    if (*expr_ptr != '\0') expr_error = 1;
    return !expr_error && isfinite(*result);
}

/* ──────────── UI Drawing ──────────── */
static void draw_header(void) {
    printf("\n");
    printf(BOLD CYAN "  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║         ∑  SCIENTIFIC CALCULATOR  ∑              ║\n");
    printf("  ║            Pure C Application                    ║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n" RESET);
    printf(DIM "  Mode: %s" RESET "\n\n", angle_mode ? "RAD" : "DEG");
}

static void draw_main_menu(void) {
    printf(BOLD WHITE "  ┌──────────────────────────────────────────────────┐\n");
    printf("  │                   MAIN MENU                      │\n");
    printf("  ├──────────────────────────────────────────────────┤\n" RESET);
    printf(GREEN  "  │  [1] " WHITE "Basic Arithmetic  (+, -, ×, ÷, %%, ^)     " GREEN "│\n");
    printf(GREEN  "  │  [2] " WHITE "Trigonometric Functions                    " GREEN "│\n");
    printf(GREEN  "  │  [3] " WHITE "Inverse Trigonometric Functions            " GREEN "│\n");
    printf(GREEN  "  │  [4] " WHITE "Hyperbolic Functions                      " GREEN "│\n");
    printf(GREEN  "  │  [5] " WHITE "Logarithmic & Exponential                 " GREEN "│\n");
    printf(GREEN  "  │  [6] " WHITE "Power & Root Functions                    " GREEN "│\n");
    printf(GREEN  "  │  [7] " WHITE "Other (factorial, abs, reciprocal, %%)     " GREEN "│\n");
    printf(GREEN  "  │  [8] " WHITE "Expression Evaluator  (type any expression)" GREEN "│\n");
    printf(YELLOW "  │  [9] " WHITE "View History                              " YELLOW "│\n");
    printf(MAGENTA"  │  [0] " WHITE "Toggle DEG/RAD                            " MAGENTA"│\n");
    printf(RED    "  │  [Q] " WHITE "Quit                                      " RED    "│\n");
    printf(BOLD WHITE "  └──────────────────────────────────────────────────┘\n" RESET);
}

static void print_result(const char *label, double result) {
    char desc[128];
    snprintf(desc, sizeof(desc), "%s", label);
    add_history(desc, result);
    printf("\n" BOLD GREEN "  ✓ Result: " WHITE "%.12g" RESET "\n", result);
}

static void print_error(const char *msg) {
    printf("\n" BOLD RED "  ✗ Error: %s" RESET "\n", msg);
}

/* ──────────── Menu handlers ──────────── */
static void menu_basic(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Basic Arithmetic ──\n\n" RESET);
    printf("  [1] Add        [2] Subtract\n");
    printf("  [3] Multiply   [4] Divide\n");
    printf("  [5] Modulo     [6] Power (a^b)\n");
    printf("  [0] Back\n\n");

    printf(CYAN "  Choice: " RESET);
    char ch;
    scanf(" %c", &ch);
    while (getchar() != '\n');

    if (ch == '0') return;
    if (ch < '1' || ch > '6') { print_error("Invalid choice"); pause_screen(); return; }

    double a = read_number("Enter first number: ");
    double b = read_number("Enter second number: ");
    double result;
    char label[128];

    switch (ch) {
        case '1': result = a + b; snprintf(label, 128, "%.6g + %.6g", a, b); break;
        case '2': result = a - b; snprintf(label, 128, "%.6g - %.6g", a, b); break;
        case '3': result = a * b; snprintf(label, 128, "%.6g × %.6g", a, b); break;
        case '4':
            if (b == 0) { print_error("Division by zero"); pause_screen(); return; }
            result = a / b; snprintf(label, 128, "%.6g ÷ %.6g", a, b); break;
        case '5':
            if (b == 0) { print_error("Division by zero"); pause_screen(); return; }
            result = fmod(a, b); snprintf(label, 128, "%.6g %% %.6g", a, b); break;
        case '6': result = pow(a, b); snprintf(label, 128, "%.6g ^ %.6g", a, b); break;
        default: return;
    }
    if (!isfinite(result)) { print_error("Math error"); pause_screen(); return; }
    print_result(label, result);
    pause_screen();
}

static void menu_trig(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Trigonometric Functions ──\n\n" RESET);
    printf("  [1] sin(x)     [2] cos(x)     [3] tan(x)\n");
    printf("  [0] Back\n\n");

    printf(CYAN "  Choice: " RESET);
    char ch;
    scanf(" %c", &ch);
    while (getchar() != '\n');
    if (ch == '0') return;
    if (ch < '1' || ch > '3') { print_error("Invalid choice"); pause_screen(); return; }

    double x = read_number("Enter angle: ");
    double r = to_rad(x);
    double result;
    char label[128];

    switch (ch) {
        case '1': result = sin(r); snprintf(label, 128, "sin(%.6g)", x); break;
        case '2': result = cos(r); snprintf(label, 128, "cos(%.6g)", x); break;
        case '3':
            if (fabs(cos(r)) < 1e-12) { print_error("tan undefined at this angle"); pause_screen(); return; }
            result = tan(r); snprintf(label, 128, "tan(%.6g)", x); break;
        default: return;
    }
    print_result(label, result);
    pause_screen();
}

static void menu_inv_trig(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Inverse Trigonometric Functions ──\n\n" RESET);
    printf("  [1] asin(x)    [2] acos(x)    [3] atan(x)\n");
    printf("  [0] Back\n\n");

    printf(CYAN "  Choice: " RESET);
    char ch;
    scanf(" %c", &ch);
    while (getchar() != '\n');
    if (ch == '0') return;
    if (ch < '1' || ch > '3') { print_error("Invalid choice"); pause_screen(); return; }

    double x = read_number("Enter value: ");
    double result;
    char label[128];

    switch (ch) {
        case '1':
            if (x < -1 || x > 1) { print_error("Domain error: input must be [-1, 1]"); pause_screen(); return; }
            result = to_angle(asin(x)); snprintf(label, 128, "asin(%.6g)", x); break;
        case '2':
            if (x < -1 || x > 1) { print_error("Domain error: input must be [-1, 1]"); pause_screen(); return; }
            result = to_angle(acos(x)); snprintf(label, 128, "acos(%.6g)", x); break;
        case '3':
            result = to_angle(atan(x)); snprintf(label, 128, "atan(%.6g)", x); break;
        default: return;
    }
    print_result(label, result);
    pause_screen();
}

static void menu_hyp(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Hyperbolic Functions ──\n\n" RESET);
    printf("  [1] sinh(x)    [2] cosh(x)    [3] tanh(x)\n");
    printf("  [0] Back\n\n");

    printf(CYAN "  Choice: " RESET);
    char ch;
    scanf(" %c", &ch);
    while (getchar() != '\n');
    if (ch == '0') return;
    if (ch < '1' || ch > '3') { print_error("Invalid choice"); pause_screen(); return; }

    double x = read_number("Enter value: ");
    double result;
    char label[128];

    switch (ch) {
        case '1': result = sinh(x); snprintf(label, 128, "sinh(%.6g)", x); break;
        case '2': result = cosh(x); snprintf(label, 128, "cosh(%.6g)", x); break;
        case '3': result = tanh(x); snprintf(label, 128, "tanh(%.6g)", x); break;
        default: return;
    }
    if (!isfinite(result)) { print_error("Overflow"); pause_screen(); return; }
    print_result(label, result);
    pause_screen();
}

static void menu_log_exp(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Logarithmic & Exponential ──\n\n" RESET);
    printf("  [1] log10(x)   [2] ln(x)      [3] e^x\n");
    printf("  [0] Back\n\n");

    printf(CYAN "  Choice: " RESET);
    char ch;
    scanf(" %c", &ch);
    while (getchar() != '\n');
    if (ch == '0') return;
    if (ch < '1' || ch > '3') { print_error("Invalid choice"); pause_screen(); return; }

    double x = read_number("Enter value: ");
    double result;
    char label[128];

    switch (ch) {
        case '1':
            if (x <= 0) { print_error("log requires positive input"); pause_screen(); return; }
            result = log10(x); snprintf(label, 128, "log10(%.6g)", x); break;
        case '2':
            if (x <= 0) { print_error("ln requires positive input"); pause_screen(); return; }
            result = log(x); snprintf(label, 128, "ln(%.6g)", x); break;
        case '3':
            result = exp(x); snprintf(label, 128, "e^%.6g", x);
            if (!isfinite(result)) { print_error("Overflow"); pause_screen(); return; }
            break;
        default: return;
    }
    print_result(label, result);
    pause_screen();
}

static void menu_power_root(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Power & Root Functions ──\n\n" RESET);
    printf("  [1] x²         [2] x³         [3] √x\n");
    printf("  [4] ∛x         [5] x^n        [6] ⁿ√x\n");
    printf("  [0] Back\n\n");

    printf(CYAN "  Choice: " RESET);
    char ch;
    scanf(" %c", &ch);
    while (getchar() != '\n');
    if (ch == '0') return;
    if (ch < '1' || ch > '6') { print_error("Invalid choice"); pause_screen(); return; }

    double x = read_number("Enter value: ");
    double result;
    char label[128];

    switch (ch) {
        case '1': result = x * x; snprintf(label, 128, "(%.6g)²", x); break;
        case '2': result = x * x * x; snprintf(label, 128, "(%.6g)³", x); break;
        case '3':
            if (x < 0) { print_error("Cannot take square root of negative number"); pause_screen(); return; }
            result = sqrt(x); snprintf(label, 128, "√%.6g", x); break;
        case '4': result = cbrt(x); snprintf(label, 128, "∛%.6g", x); break;
        case '5': {
            double n = read_number("Enter exponent: ");
            result = pow(x, n); snprintf(label, 128, "%.6g ^ %.6g", x, n); break;
        }
        case '6': {
            double n = read_number("Enter root degree: ");
            if (n == 0) { print_error("Root degree cannot be zero"); pause_screen(); return; }
            result = pow(x, 1.0 / n); snprintf(label, 128, "%.6g ^ (1/%.6g)", x, n); break;
        }
        default: return;
    }
    if (!isfinite(result)) { print_error("Math error"); pause_screen(); return; }
    print_result(label, result);
    pause_screen();
}

static void menu_other(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Other Functions ──\n\n" RESET);
    printf("  [1] n! (factorial)    [2] |x| (absolute)\n");
    printf("  [3] 1/x (reciprocal) [4] %% (percent)\n");
    printf("  [5] π (pi value)     [6] e (Euler's number)\n");
    printf("  [0] Back\n\n");

    printf(CYAN "  Choice: " RESET);
    char ch;
    scanf(" %c", &ch);
    while (getchar() != '\n');
    if (ch == '0') return;

    double result;
    char label[128];

    switch (ch) {
        case '1': {
            double x = read_number("Enter non-negative integer: ");
            if (x < 0 || x > 170 || floor(x) != x) { print_error("Factorial requires integer 0–170"); pause_screen(); return; }
            result = factorial((int)x); snprintf(label, 128, "%.0f!", x); break;
        }
        case '2': {
            double x = read_number("Enter value: ");
            result = fabs(x); snprintf(label, 128, "|%.6g|", x); break;
        }
        case '3': {
            double x = read_number("Enter value: ");
            if (x == 0) { print_error("Division by zero"); pause_screen(); return; }
            result = 1.0 / x; snprintf(label, 128, "1 / %.6g", x); break;
        }
        case '4': {
            double x = read_number("Enter value: ");
            result = x / 100.0; snprintf(label, 128, "%.6g%%", x); break;
        }
        case '5':
            result = PI; snprintf(label, 128, "π"); break;
        case '6':
            result = EULER; snprintf(label, 128, "e"); break;
        default: print_error("Invalid choice"); pause_screen(); return;
    }
    print_result(label, result);
    pause_screen();
}

static void menu_expression(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Expression Evaluator ──\n\n" RESET);
    printf(DIM "  Supports: + - * / %% ^ ( ) and numbers\n");
    printf("  Constants: pi, e\n");
    printf("  Example: 3 + 5 * (2 - 8) ^ 2\n\n" RESET);

    char input[EXPR_LEN];
    printf(CYAN "  Enter expression: " RESET);
    if (!fgets(input, sizeof(input), stdin)) return;
    /* Remove trailing newline */
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') input[len - 1] = '\0';
    if (strlen(input) == 0) return;

    double result;
    if (evaluate_expression(input, &result)) {
        char label[128];
        snprintf(label, sizeof(label), "%s", input);
        print_result(label, result);
    } else {
        print_error("Invalid expression");
    }
    pause_screen();
}

static void menu_history(void) {
    clear_screen();
    draw_header();
    printf(BOLD CYAN "  ── Calculation History ──\n\n" RESET);

    if (hist_count == 0) {
        printf(DIM "  No calculations yet.\n" RESET);
    } else {
        printf(BOLD WHITE "  ┌─────┬────────────────────────────────┬──────────────────┐\n");
        printf("  │  #  │  Expression                    │  Result          │\n");
        printf("  ├─────┼────────────────────────────────┼──────────────────┤\n" RESET);
        for (int i = 0; i < hist_count; i++) {
            printf("  │ %3d │  %-28.28s│  %-16.12g│\n", i + 1, history[i].description, history[i].result);
        }
        printf(BOLD WHITE "  └─────┴────────────────────────────────┴──────────────────┘\n" RESET);
    }
    pause_screen();
}

/* ──────────── Main Loop ──────────── */
int main(void) {
    enable_ansi();

    while (1) {
        clear_screen();
        draw_header();
        draw_main_menu();
        printf("\n" CYAN "  Choice: " RESET);

        char choice;
        scanf(" %c", &choice);
        while (getchar() != '\n');

        switch (choice) {
            case '1': menu_basic();      break;
            case '2': menu_trig();       break;
            case '3': menu_inv_trig();   break;
            case '4': menu_hyp();        break;
            case '5': menu_log_exp();    break;
            case '6': menu_power_root(); break;
            case '7': menu_other();      break;
            case '8': menu_expression(); break;
            case '9': menu_history();    break;
            case '0':
                angle_mode = !angle_mode;
                printf(GREEN "\n  ✓ Switched to %s mode\n" RESET, angle_mode ? "RAD" : "DEG");
                pause_screen();
                break;
            case 'q': case 'Q':
                clear_screen();
                printf(BOLD CYAN "\n  Thank you for using Scientific Calculator!\n\n" RESET);
                return 0;
            default:
                print_error("Invalid choice");
                pause_screen();
                break;
        }
    }
}
