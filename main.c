#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/* --------------------------- Token Type Definitions --------------------------- */

typedef enum {
    TOK_NONE, TOK_NUMBER, TOK_OP, TOK_LPAREN, TOK_RPAREN, TOK_FUNC, TOK_COMMA
} TokenType;

typedef struct {
    TokenType type;
    double value;
    char op;
    char name[8];
} Token;

/* --------------------------- Function Prototypes --------------------------- */
static void on_button_clicked(GtkButton *button, gpointer user_data);
static void on_entry_activate(GtkEntry *entry);
static int evaluate_expression(const char *expr, double *result);
static int tokenize(const char *s, Token *out, int max);
static int shunting_yard(Token *in, int in_len, Token *out, int max);
static int eval_rpn(Token *rpn, int len, double *result);
static int operator_precedence(char op);
static void activate(GtkApplication *app, gpointer user_data);

/* ------------------------Runs when user presses enter --------------------------- */
static void on_entry_activate(GtkEntry *entry) {
    // Stores the text from the entry box into a txt var
    gchar *txt = NULL;
    g_object_get(entry, "text", &txt, NULL);
    //creates a result variable to store calculation
    double result;
    //edge case check for empty box or nonvalid math
    //Calls evaluate expression to do the math pointing to result
    if (txt && evaluate_expression(txt, &result) == 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%g", result);

        g_object_set(entry, "text", buf, NULL);
    } else {
        g_object_set(entry, "text", "Error", NULL);
    }

    if (txt) g_free(txt);
}

/* ---------------------Calculator Window and Buttons --------------------------- */
static void activate(GtkApplication *app, gpointer user_data) {
    //Create main window
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Scientific Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    //Initalizes the GTK Entry box
    GtkWidget *entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_widget_set_margin_top(entry, 10);
    gtk_widget_set_margin_bottom(entry, 10);
    gtk_widget_set_margin_start(entry, 10);
    gtk_widget_set_margin_end(entry, 10);
    gtk_box_append(GTK_BOX(vbox), entry);

    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);
    //create grid for holding buttons
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    gtk_box_append(GTK_BOX(vbox), grid);

    const char *buttons[6][6] = {
        {"Sin", "Cos", "Tan", "Clear", ""},
        {"EE", "Ln", "Log", "xLog()", "", ""},
        {"7", "8", "9", "/", "", ""},
        {"4", "5", "6", "*", "", ""},
        {"1", "2", "3", "-", ")", ""},
        {"0", ".", "=", "+", "(", ""}
    };
    //add each button to the grid and make them clickable
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            const char *label = buttons[i][j];
            if (label == NULL || label[0] == '\0') continue;
            //Use GTK button widget
            GtkWidget *button = gtk_button_new_with_label(label);
            gtk_widget_set_hexpand(button, TRUE);
            gtk_widget_set_vexpand(button, TRUE);
            gtk_grid_attach(GTK_GRID(grid), button, j, i, 1, 1);
            g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), entry);
        }
    }

    gtk_window_present(GTK_WINDOW(window));
}

/* ---------- Evaluates the expression based off the content inside the entry box- */
static int evaluate_expression(const char *expr, double *result) {
    Token toks[512], rpn[512];
    int n = tokenize(expr, toks, 512);
    if (n < 0) return -1;

    int m = shunting_yard(toks, n, rpn, 512);
    if (m < 0) return -1;

    if (eval_rpn(rpn, m, result) != 0) return -1;
    return 0;
}

/* ------------Tokenizer (Called in the evaluate_expression function)-------------- */
static int tokenize(const char *s, Token *out, int max) {
    int i = 0;
    const char *p = s;

    while (*p && i < max) {
        //Skips the spaces in the expression
        if (isspace((unsigned char)*p)) {
            p++;
            continue;
        }
        //checks to see if its number or decimal
        if (isdigit((unsigned char)*p) || (*p == '.')) {
            char *end;
            out[i].type = TOK_NUMBER;
            //string to double conversion
            out[i].value = strtod(p, &end);
            p = end;
            i++;
            continue;
        }
        //Checks to see if letter (sin, cos, log, etc.)
        //
        if (isalpha((unsigned char)*p)) {
            int j = 0;
            while (isalpha((unsigned char)*p) && j < (int)sizeof(out[i].name) - 1)
                out[i].name[j++] = tolower((unsigned char)*p++);
            out[i].name[j] = '\0';
            out[i].type = TOK_FUNC;
            i++;
            continue;
        }

        if (*p == ',') { out[i].type = TOK_COMMA; i++; p++; continue; }
        if (*p == '(') { out[i].type = TOK_LPAREN; i++; p++; continue; }
        if (*p == ')') { out[i].type = TOK_RPAREN; i++; p++; continue; }
        if (strchr("+-*/^", *p)) { out[i].type = TOK_OP; out[i].op = *p; i++; p++; continue; }

        return -1; // Invalid token
    }
    return i;
}

/* --------------------------- Operator Precedence --------------------------- */
static int operator_precedence(char op) {
    switch (op) {
        case '+':
        case '-': return 1;
        case '*':
        case '/': return 2;
        case '^': return 3;
        default: return 0;
    }
}

/* --------------------------- Shunting Yard Algorithm --------------------------- */
static int shunting_yard(Token *in, int in_len, Token *out, int max) {
    Token stack[512];
    int sp = 0, out_n = 0;

    for (int i = 0; i < in_len; ++i) {
        Token t = in[i];

        switch (t.type) {
            case TOK_NUMBER:
                out[out_n++] = t;
                break;

            case TOK_FUNC:
                stack[sp++] = t;
                break;

            case TOK_COMMA:
                while (sp > 0 && stack[sp - 1].type != TOK_LPAREN)
                    out[out_n++] = stack[--sp];
                if (sp == 0) return -1;
                break;

            case TOK_OP:
                while (sp > 0 && stack[sp - 1].type == TOK_OP &&
                       ((operator_precedence(stack[sp - 1].op) > operator_precedence(t.op)) ||
                        (operator_precedence(stack[sp - 1].op) == operator_precedence(t.op) && t.op != '^')))
                    out[out_n++] = stack[--sp];
                stack[sp++] = t;
                break;

            case TOK_LPAREN:
                stack[sp++] = t;
                break;

            case TOK_RPAREN:
                while (sp > 0 && stack[sp - 1].type != TOK_LPAREN)
                    out[out_n++] = stack[--sp];
                if (sp == 0) return -1;
                sp--;
                if (sp > 0 && stack[sp - 1].type == TOK_FUNC)
                    out[out_n++] = stack[--sp];
                break;

            default:
                break;
        }
    }

    while (sp > 0) {
        if (stack[sp - 1].type == TOK_LPAREN) return -1;
        out[out_n++] = stack[--sp];
    }

    return out_n;
}

/* --------------------------- Reverse Polish Notation Evaluator --------------------------- */
static int eval_rpn(Token *rpn, int len, double *result) {
    double stack[512];
    int sp = 0;

    for (int i = 0; i < len; ++i) {
        Token t = rpn[i];

        if (t.type == TOK_NUMBER) {
            stack[sp++] = t.value;
            continue;
        }

        if (t.type == TOK_OP) {
            if (sp < 2) return -1;
            double b = stack[--sp];
            double a = stack[--sp];
            double r = 0;

            switch (t.op) {
                case '+': r = a + b; break;
                case '-': r = a - b; break;
                case '*': r = a * b; break;
                case '/': if (b == 0) return -1; r = a / b; break;
                case '^': r = pow(a, b); break;
                default: return -1;
            }

            stack[sp++] = r;
            continue;
        }

        if (t.type == TOK_FUNC) {
            if (strcmp(t.name, "xlog") == 0) {
                if (sp < 2) return -1;
                double val = stack[--sp];
                double base = stack[--sp];
                if (base <= 0 || val <= 0) return -1;
                stack[sp++] = log(val) / log(base);
            } else {
                if (sp < 1) return -1;
                double v = stack[--sp];
                double r = 0;

                if (strcmp(t.name, "sin") == 0) r = sin(v);
                else if (strcmp(t.name, "cos") == 0) r = cos(v);
                else if (strcmp(t.name, "tan") == 0) r = tan(v);
                else if (strcmp(t.name, "ln") == 0) { if (v <= 0) return -1; r = log(v); }
                else if (strcmp(t.name, "log") == 0) { if (v <= 0) return -1; r = log10(v); }
                else return -1;

                stack[sp++] = r;
            }
        }
    }

    if (sp != 1) return -1;
    *result = stack[0];
    return 0;
}

/* --------------------------- Button Handler --------------------------- */
static void on_button_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);
    const char *label = gtk_button_get_label(button);
    if (!label) return;

    gchar *txt = NULL;
    g_object_get(entry, "text", &txt, NULL);

    if (strcmp(label, "=") == 0) {
        double res;
        if (txt && evaluate_expression(txt, &res) == 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%g", res);
            g_object_set(entry, "text", buf, NULL);
        } else {
            g_object_set(entry, "text", "Error", NULL);
        }
        if (txt) g_free(txt);
        return;
    }

    if (strcasecmp(label, "clear") == 0) {
        g_object_set(entry, "text", "", NULL);
        if (txt) g_free(txt);
        return;
    }

    const char *functions[] = {"Sin", "Cos", "Tan", "Ln", "Log", "xLog()"};
    const char *func_names[] = {"sin(", "cos(", "tan(", "ln(", "log(", "xlog("};

    for (int i = 0; i < 6; i++) {
        if (strcmp(label, functions[i]) == 0) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s%s", txt ? txt : "", func_names[i]);
            g_object_set(entry, "text", buf, NULL);
            if (txt) g_free(txt);
            return;
        }
    }

    // Default append behavior
    char buf[512];
    snprintf(buf, sizeof(buf), "%s%s", txt ? txt : "", label);
    g_object_set(entry, "text", buf, NULL);
    if (txt) g_free(txt);
}

/* --------------------------- Main --------------------------- */
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.example.calculator", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
