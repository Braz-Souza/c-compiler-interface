#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Variáveis globais para o socket
int sockfd;
char *hostname;
int portno;

typedef struct {
    GtkTextView *textview;
    GtkTextView *output_textview;
} AppWidgets; 

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void on_button_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *widgets = (AppWidgets *)data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(widgets->textview);
    GtkTextBuffer *output_buffer = gtk_text_view_get_buffer(widgets->output_textview);
    GtkTextIter start, end;
    
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *code = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    // Conectar ao servidor
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char response[256];
    int n;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        gtk_text_buffer_set_text(output_buffer, "ERROR: Could not open socket", -1);
        g_free(code);
        return;
    }
    
    server = gethostbyname(hostname);
    if (server == NULL) {
        gtk_text_buffer_set_text(output_buffer, "ERROR: No such host", -1);
        close(sockfd);
        g_free(code);
        return;
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
    serv_addr.sin_port = htons(portno);
    
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        gtk_text_buffer_set_text(output_buffer, "ERROR: Could not connect to server", -1);
        close(sockfd);
        g_free(code);
        return;
    }
    
    // Enviar código para o servidor
    n = write(sockfd, code, strlen(code));
    if (n < 0) {
        gtk_text_buffer_set_text(output_buffer, "ERROR: Could not write to socket", -1);
        close(sockfd);
        g_free(code);
        return;
    }
    
    // Receber resposta do servidor
    bzero(response, 256);
    n = read(sockfd, response, 255);
    if (n < 0) {
        gtk_text_buffer_set_text(output_buffer, "ERROR: Could not read from socket", -1);
    } else {
        gtk_text_buffer_set_text(output_buffer, response, -1);
    }
    
    close(sockfd);
    g_free(code);
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    
    hostname = argv[1];
    portno = atoi(argv[2]);
    
    gtk_init(&argc, &argv);
    
    // Criar janela principal
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "C Compiler Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    
    // Container principal
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    // Label para área de código
    GtkWidget *code_label = gtk_label_new("Código C:");
    gtk_widget_set_halign(code_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), code_label, FALSE, FALSE, 0);
    
    // ScrolledWindow para o código
    GtkWidget *scrolled_code = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_code), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_code, TRUE);
    gtk_widget_set_hexpand(scrolled_code, TRUE);
    
    // TextView para o código
    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 6);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(buffer, "#include <stdio.h>\n\nint main()\n{\n    printf(\"Hello world!\\n\");\n    return 0;\n}", -1);
    
    gtk_container_add(GTK_CONTAINER(scrolled_code), textview);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_code, TRUE, TRUE, 0);
    
    // Label para área de saída
    GtkWidget *output_label = gtk_label_new("Resposta do Servidor:");
    gtk_widget_set_halign(output_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), output_label, FALSE, FALSE, 0);
    
    // ScrolledWindow para a saída
    GtkWidget *scrolled_output = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_output), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_output, TRUE);
    gtk_widget_set_hexpand(scrolled_output, TRUE);
    
    // TextView para a saída
    GtkWidget *output_textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output_textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(output_textview), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(output_textview), 6);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_textview), FALSE);
    
    gtk_container_add(GTK_CONTAINER(scrolled_output), output_textview);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_output, TRUE, TRUE, 0);
    
    // Botão para compilar
    GtkWidget *button = gtk_button_new_with_label("Compilar e Executar");
    
    // Estrutura para passar os widgets
    AppWidgets *widgets = g_malloc(sizeof(AppWidgets));
    widgets->textview = GTK_TEXT_VIEW(textview);
    widgets->output_textview = GTK_TEXT_VIEW(output_textview);
    
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    gtk_widget_show_all(window);
    gtk_main();
    
    g_free(widgets);
    return 0;
}
