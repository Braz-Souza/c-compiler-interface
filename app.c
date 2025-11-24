#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void execute_c_code(const gchar *code) {
    FILE *source_file = fopen("/tmp/temp_code.c", "w");
    if (!source_file) {
        g_print("Erro: Não foi possível criar o arquivo temporário\n");
        return;
    }
    fprintf(source_file, "%s", code);
    fclose(source_file);
    
    int compile_result = system("gcc /tmp/temp_code.c -o /tmp/temp_code 2>&1");
    
    if (compile_result != 0) {
        system("gcc /tmp/temp_code.c -o /tmp/temp_code 2>&1");
        return;
    }
    
    FILE *output = popen("/tmp/temp_code 2>&1", "r");
    if (!output) {
        g_print("Erro: Não foi possível executar o programa\n");
        return;
    }
    
    char buffer[256];
    g_print("output:\n");
    while (fgets(buffer, sizeof(buffer), output) != NULL) {
        g_print("%s", buffer);
    }
    
    int exec_result = pclose(output);
    g_print("\nreturn: %d\n", WEXITSTATUS(exec_result));
    
    remove("/tmp/temp_code.c");
    remove("/tmp/temp_code");
}

void on_button_clicked(GtkWidget *widget, gpointer data) {
    GtkTextView *textview = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    execute_c_code(text);
    
    g_free(text);
}

void ui_config(int *argc, char ***argv) {
    gtk_init(argc, argv);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Client & Server juntos");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);

    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 6);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(buffer, "#include <stdio.h>\n\nint main()\n{\n    printf(\"Hello world!\\n\");\n    return 0;\n}", -1);

    gtk_container_add(GTK_CONTAINER(scrolled), textview);

    GtkWidget *button = gtk_button_new_with_label("Compilar");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), textview);

    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    gtk_widget_show_all(window);
    gtk_main();
} 

int main(int argc, char *argv[]) {
    ui_config(&argc, &argv);
    return 0;
}
