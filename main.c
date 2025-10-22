#include <gtk/gtk.h>
#include <math.h>

static void activate(GtkApplication *app, gpointer user_data) {
  

    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Calculator");
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

        {"Sin","Cos","Tan","",""},
        {"EE","Ln","Log","xLog()","",""},
        {"7", "8", "9", "/","",""},
        {"4", "5", "6", "*","",""},
        {"1", "2", "3", "-","",""},
        {"0", ".", "=", "+","penar",""}

    };

  
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            GtkWidget *button = gtk_button_new_with_label(buttons[i][j]);
            gtk_widget_set_hexpand(button, TRUE);
            gtk_widget_set_vexpand(button, TRUE);
            gtk_grid_attach(GTK_GRID(grid), button, j, i, 1, 1);
        }
    }

    gtk_widget_show(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
      gtk_box_set_homogeneous (GTK_BOX (main_box), TRUE);
    gtk_container_add (GTK_CONTAINER (window), main_box);
    gtk_container_add (GTK_CONTAINER (main_box), button_01);
    gtk_container_add (GTK_CONTAINER (main_box), label0);
    gtk_container_add (GTK_CONTAINER (main_box), label1);
    gtk_container_add (GTK_CONTAINER (main_box), label2);
    gtk_container_add (GTK_CONTAINER (main_box), button_02);
    app = gtk_application_new("com.example.calculator", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

