#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/* function prototype (forward declaration) */
static void on_button_clicked(GtkButton *button, gpointer user_data);

//Whole function setups the calculator window and its components
static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data; /* silence unused parameter warning */

    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Scientific Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

  
    entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_margin_top(entry, 10);
    gtk_widget_set_margin_bottom(entry, 10);
    gtk_widget_set_margin_start(entry, 10);
    gtk_widget_set_margin_end(entry, 10);
    gtk_box_append(GTK_BOX(vbox), entry);


    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    gtk_box_append(GTK_BOX(vbox), grid);

    
    const char *buttons[6][6] = {

        {"Sin","Cos","Tan","clear",""},
        {"EE","Ln","Log","xLog()","",""},
        {"7", "8", "9", "/","",""},
        {"4", "5", "6", "*","",""},
        {"1", "2", "3", "-","penar⁻¹",""},
        {"0", ".", "=", "+","penar",""}

    };

  
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            const char *label = buttons[i][j];
            if (label == NULL || label[0] == '\0')
                continue;
            GtkWidget *button = gtk_button_new_with_label(label);
            gtk_widget_set_hexpand(button, TRUE);
            gtk_widget_set_vexpand(button, TRUE);
            gtk_grid_attach(GTK_GRID(grid), button, j, i, 1, 1);
            /* connect all buttons to the same callback; pass the entry as user_data */
            g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), entry);
        }
    }

        /* Use gtk_window_present (recommended in GTK4) to show the window */
        gtk_window_present(GTK_WINDOW(window));
}


typedef enum {TOK_NONE, TOK_NUMBER, TOK_OP, TOK_LPAREN, TOK_RPAREN, TOK_FUNC, TOK_COMMA} TokenType;

typedef struct {
    TokenType type;
    double value; /* for numbers */
    char op;      /* for operators */
    char name[8]; /* for functions (like sin, cos, xlog) */
} Token;

/* Forward declarations */
static int tokenize(const char *s, Token *out, int max);
static int shunting_yard(Token *in, int in_len, Token *out, int max);
static int eval_rpn(Token *rpn, int len, double *result);

static int evaluate_expression(const char *expr, double *result) {
    Token toks[512];
    Token rpn[512];
    int n = tokenize(expr, toks, 512);
    if (n < 0) return -1;
    int m = shunting_yard(toks, n, rpn, 512);
    if (m < 0) return -1;
    if (eval_rpn(rpn, m, result) != 0) return -1;
    return 0;
}

/* Very small tokenizer supporting numbers (with E), operators + - * / ^, parentheses, commas, and functions (letters) */
static int tokenize(const char *s, Token *out, int max) {
    int i = 0; const char *p = s;
    while (*p && i < max) {
        if (isspace((unsigned char)*p)) { p++; continue; }
        if (isdigit((unsigned char)*p) || (*p=='.')) {
            char *end;
            double v = strtod(p, &end);
            out[i].type = TOK_NUMBER; out[i].value = v; i++;
            p = end; continue;
        }
        if (isalpha((unsigned char)*p)) {
            /* read identifier */
            int j = 0;
            while (isalpha((unsigned char)*p) && j < (int)sizeof(out[i].name)-1) out[i].name[j++] = tolower((unsigned char)*p++);
            out[i].name[j] = '\0';
            out[i].type = TOK_FUNC; i++; continue;
        }
        if (*p == ',') { out[i].type = TOK_COMMA; i++; p++; continue; }
        if (*p == '(') { out[i].type = TOK_LPAREN; i++; p++; continue; }
        if (*p == ')') { out[i].type = TOK_RPAREN; i++; p++; continue; }
        /* operators */
        if (strchr("+-*/^", *p)) { out[i].type = TOK_OP; out[i].op = *p; i++; p++; continue; }
        /* unknown character */
        return -1;
    }
    return i;
}

/* Operator precedence */
static int prec(char op) {
    switch (op) {
        case '+': case '-': return 1;
        case '*': case '/': return 2;
        case '^': return 3;
    }
    return 0;
}

static int shunting_yard(Token *in, int in_len, Token *out, int max) {
    Token stack[512]; int sp = 0;
    int out_n = 0;
    for (int i = 0; i < in_len; ++i) {
        Token t = in[i];
        if (t.type == TOK_NUMBER) {
            if (out_n >= max) return -1;
            out[out_n++] = t;
        } else if (t.type == TOK_FUNC) {
            if (sp >= 512) return -1;
            stack[sp++] = t;
        } else if (t.type == TOK_COMMA) {
            while (sp>0 && stack[sp-1].type != TOK_LPAREN) {
                if (out_n>=max) return -1;
                out[out_n++]=stack[--sp];
            }
            if (sp==0) return -1; /* mismatched */
        } else if (t.type == TOK_OP) {
            while (sp>0 && stack[sp-1].type==TOK_OP && ((prec(stack[sp-1].op) > prec(t.op)) || (prec(stack[sp-1].op)==prec(t.op) && t.op != '^'))) {
                if (out_n>=max) return -1;
                out[out_n++]=stack[--sp];
            }
            if (sp>=512) return -1;
            stack[sp++]=t;
        } else if (t.type == TOK_LPAREN) {
            if (sp>=512) return -1;
            stack[sp++]=t;
        } else if (t.type == TOK_RPAREN) {
            while (sp>0 && stack[sp-1].type!=TOK_LPAREN) {
                if (out_n>=max) return -1;
                out[out_n++]=stack[--sp];
            }
            if (sp==0) return -1; /* mismatched */
            sp--; /* pop LPAREN */
            if (sp>0 && stack[sp-1].type==TOK_FUNC) {
                if (out_n>=max) return -1;
                out[out_n++]=stack[--sp];
            }
        }
    }
    while (sp>0) {
        if (stack[sp-1].type==TOK_LPAREN || stack[sp-1].type==TOK_RPAREN) return -1;
        if (out_n>=max) return -1;
        out[out_n++]=stack[--sp];
    }
    return out_n;
}


static int eval_rpn(Token *rpn, int len, double *result) {
    double stack[512]; int sp = 0;
    for (int i = 0; i < len; ++i) {
        Token t = rpn[i];
        if (t.type == TOK_NUMBER) { stack[sp++] = t.value; }
        else if (t.type == TOK_OP) {
            if (sp < 2) return -1;
            double b = stack[--sp]; double a = stack[--sp]; double r = 0;
            switch (t.op) {
                case '+': r = a + b; break;
                case '-': r = a - b; break;
                case '*': r = a * b; break;
                case '/': if (b == 0) return -1; r = a / b; break;
                case '^': r = pow(a, b); break;
                default: return -1;
            }
            stack[sp++] = r;
        } else if (t.type == TOK_FUNC) {
            if (strcmp(t.name, "xlog") == 0) {
                /* xlog(base, value) */
                if (sp < 2) return -1;
                double val = stack[--sp]; double base = stack[--sp];
                if (base <= 0 || val <= 0) return -1;
                stack[sp++] = log(val) / log(base);
            } else {
                if (sp < 1) return -1;
                double v = stack[--sp]; double r = 0;
                if (strcmp(t.name, "sin")==0) r = sin(v);
                else if (strcmp(t.name, "cos")==0) r = cos(v);
                else if (strcmp(t.name, "tan")==0) r = tan(v);
                else if (strcmp(t.name, "ln")==0) { if (v <= 0) return -1; r = log(v); }
                else if (strcmp(t.name, "log")==0) { if (v <= 0) return -1; r = log10(v); }
                else return -1;
                stack[sp++] = r;
            }
        } else return -1;
    }
    if (sp != 1) return -1;
    *result = stack[0];
    return 0;
}


static void on_button_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);
    const char *label = gtk_button_get_label(button);
    if (!label) return;
    const char *l = label;
    if (strcmp(l, "=") == 0) {
        gchar *txt = NULL;
        g_object_get(entry, "text", &txt, NULL);
        double res;
        if (txt && evaluate_expression(txt, &res) == 0) {
            char buf[128]; snprintf(buf, sizeof(buf), "%g", res);
            g_object_set(entry, "text", buf, NULL);
        } else {
            g_object_set(entry, "text", "Error", NULL);
        }
        if (txt) g_free(txt);
        return;
    }
    /* special buttons */
    if (strcmp(l, "Sin") == 0) { /* wrap as function */
        gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL);
        double v; if (txt && evaluate_expression(txt, &v) == 0) { double r = sin(v); char buf[128]; snprintf(buf,sizeof(buf),"%g", r); g_object_set(entry, "text", buf, NULL); } else g_object_set(entry, "text", "Error", NULL);
        if (txt) g_free(txt);
        return;
    }
    if (strcmp(l, "Cos") == 0) { gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL); double v; if (txt && evaluate_expression(txt, &v) == 0) { char buf[128]; snprintf(buf,sizeof(buf),"%g", cos(v)); g_object_set(entry, "text", buf, NULL); } else g_object_set(entry, "text", "Error", NULL); if (txt) g_free(txt); return; }
    if (strcmp(l, "Tan") == 0) { gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL); double v; if (txt && evaluate_expression(txt, &v) == 0) { char buf[128]; snprintf(buf,sizeof(buf),"%g", tan(v)); g_object_set(entry, "text", buf, NULL); } else g_object_set(entry, "text", "Error", NULL); if (txt) g_free(txt); return; }
    if (strcmp(l, "Ln") == 0) { gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL); double v; if (txt && evaluate_expression(txt, &v) == 0 && v>0) { char buf[128]; snprintf(buf,sizeof(buf),"%g", log(v)); g_object_set(entry, "text", buf, NULL); } else g_object_set(entry, "text", "Error", NULL); if (txt) g_free(txt); return; }
    if (strcmp(l, "Log") == 0) { gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL); double v; if (txt && evaluate_expression(txt, &v) == 0 && v>0) { char buf[128]; snprintf(buf,sizeof(buf),"%g", log10(v)); g_object_set(entry, "text", buf, NULL); } else g_object_set(entry, "text", "Error", NULL); if (txt) g_free(txt); return; }
    if (strcmp(l, "xLog()") == 0) { /* insert xlog( to allow xlog(base, value) */
        gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL);
        char buf[512]; snprintf(buf, sizeof(buf), "%s%s", txt ? txt : "", "xlog("); g_object_set(entry, "text", buf, NULL); if (txt) g_free(txt); return;
    }
    if (strcmp(l, "EE") == 0) { 
        gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL);
        char buf[512]; snprintf(buf, sizeof(buf), "%sE", txt ? txt : ""); g_object_set(entry, "text", buf, NULL); if (txt) g_free(txt); return;
    }
    if (strcmp(l, "penar") == 0) { gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL); char buf[512]; snprintf(buf,sizeof(buf), "%s(", txt ? txt : ""); g_object_set(entry, "text", buf, NULL); if (txt) g_free(txt); return; }
    if (strcmp(l, "penar⁻¹") == 0) { gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL); char buf[512]; snprintf(buf,sizeof(buf), "%s)", txt ? txt : ""); g_object_set(entry, "text", buf, NULL); if (txt) g_free(txt); return; }
    
    if (strcmp(l, "clear") == 0) {
        /* Clear the entry text */
        g_object_set(entry, "text", "", NULL);
        return;
    }

    gchar *txt = NULL; g_object_get(entry, "text", &txt, NULL);
    char buf[512]; snprintf(buf, sizeof(buf), "%s%s", txt ? txt : "", l);
    g_object_set(entry, "text", buf, NULL);
    if (txt) g_free(txt);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
        //Create the instance of the app for the calculator
    app = gtk_application_new("com.example.calculator", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

