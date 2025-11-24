#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_OUTPUT_SIZE 4096
#define PATH_MAX_LEN 256
#define PATH_OVERHEAD 10
#define COMMAND_OVERHEAD 50

#ifndef P_tmpdir
#define P_tmpdir "/tmp"
#endif

void execute_c_code(const char *c_code, char *output_buffer) {
    FILE *code_file = NULL;
    FILE *fp = NULL;

    char temp_file[PATH_MAX_LEN];
    char temp_executable[PATH_MAX_LEN];
    char command[PATH_MAX_LEN];

    printf("%s\n", c_code);
    snprintf(temp_file, sizeof(temp_file), "%s/temp_code_XXXXXX", P_tmpdir);

    int fd = mkstemp(temp_file);
    if (fd == -1) {
        snprintf(output_buffer, MAX_OUTPUT_SIZE, 
            "SERVER ERROR: mkstemp failed (%s) on path: %s\n", strerror(errno), temp_file);
        return;
    }

    close(fd);

    char final_temp_file[PATH_MAX_LEN];
    snprintf(final_temp_file, sizeof(final_temp_file), "%.*s.c", 
         (int)sizeof(final_temp_file) - PATH_OVERHEAD, temp_file);

    if (rename(temp_file, final_temp_file) != 0) {
        snprintf(output_buffer, MAX_OUTPUT_SIZE, 
            "SERVER ERROR: rename failed (%s) on path: %s to %s\n", strerror(errno), temp_file, final_temp_file);
        remove(temp_file);
        return;
    }

    code_file = fopen(final_temp_file, "w");
    if (code_file == NULL) {
        snprintf(output_buffer, MAX_OUTPUT_SIZE, 
            "SERVER ERROR: failed to open file (%s) on path: %s to %s\n", strerror(errno), temp_file, final_temp_file);
        remove(final_temp_file);
        return;
    }

    fprintf(code_file, "%s", c_code);
    fclose(code_file);

    snprintf(temp_executable, sizeof(temp_executable), "%.*s.out", 
         (int)sizeof(temp_executable) - PATH_OVERHEAD, temp_file);

    snprintf(command, sizeof(command),
         "gcc %.*s -o %.*s 2>&1", 
         (int)sizeof(command) - COMMAND_OVERHEAD * 2, final_temp_file,
         (int)sizeof(command) - COMMAND_OVERHEAD * 2, temp_executable);
    
    fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(output_buffer, MAX_OUTPUT_SIZE, "SERVER ERROR: Failed to run compilation command\n");
        remove(final_temp_file);
        return;
    }

    bzero(output_buffer, MAX_OUTPUT_SIZE);

    char line_buffer[256];
    size_t current_len = 0;

    while (fgets(line_buffer, sizeof(line_buffer), fp) != NULL) {
        size_t line_len = strlen(line_buffer);
        if (current_len + line_len < MAX_OUTPUT_SIZE - 1) {
            strncat(output_buffer, line_buffer, line_len);
            current_len += line_len;
        } else {
            strncat(output_buffer, "\n[Output truncated]\n", MAX_OUTPUT_SIZE - current_len - 1);
            break;
        }
    }

    int compile_status = pclose(fp);

    if (compile_status != 0) {
        char error_msg[MAX_OUTPUT_SIZE];
        if (output_buffer[0] == '\0') {
            snprintf(error_msg, MAX_OUTPUT_SIZE, "ERROR: Compilation failed with status %d\n", WEXITSTATUS(compile_status));
        } else {
            snprintf(error_msg, MAX_OUTPUT_SIZE, "ERROR: %s", output_buffer);
        }
        strncpy(output_buffer, error_msg, MAX_OUTPUT_SIZE - 1);
        output_buffer[MAX_OUTPUT_SIZE - 1] = '\0';
        
        printf("%s", output_buffer);
        remove(final_temp_file);
        return;
    }

    snprintf(command, sizeof(command),
         "%.*s 2>&1", (int)sizeof(command) - COMMAND_OVERHEAD, temp_executable);
    
    fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(output_buffer, MAX_OUTPUT_SIZE, "SERVER ERROR: Failed to run executable\n");
        remove(final_temp_file);
        remove(temp_executable);
        return;
    }

    bzero(output_buffer, MAX_OUTPUT_SIZE);
    current_len = 0;

    while (fgets(line_buffer, sizeof(line_buffer), fp) != NULL) {
        size_t line_len = strlen(line_buffer);
        if (current_len + line_len < MAX_OUTPUT_SIZE - 1) {
            strncat(output_buffer, line_buffer, line_len);
            current_len += line_len;
        } else {
            strncat(output_buffer, "\n[Output truncated]\n", MAX_OUTPUT_SIZE - current_len - 1);
            break;
        }
    }

    int status = pclose(fp);

    char final_output[MAX_OUTPUT_SIZE];
    if (status != 0 && output_buffer[0] == '\0') {
        snprintf(final_output, MAX_OUTPUT_SIZE, "ERROR: C execution failed with status %d\n", WEXITSTATUS(status));
    } else if (status == 0 && output_buffer[0] == '\0') {
        snprintf(final_output, MAX_OUTPUT_SIZE, 
            "EXECUTED: (Program finished successfully with no output.)");
    } else if (status == 0) {
        snprintf(final_output, MAX_OUTPUT_SIZE, "EXECUTED: %s", output_buffer);
    } else {
        snprintf(final_output, MAX_OUTPUT_SIZE, "ERROR: %s", output_buffer);
    }
    
    strncpy(output_buffer, final_output, MAX_OUTPUT_SIZE - 1);
    output_buffer[MAX_OUTPUT_SIZE - 1] = '\0';
    
    printf("%s", output_buffer);

    remove(final_temp_file);
    remove(temp_executable);
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    char output_buffer[MAX_OUTPUT_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc < 2) {
        fprintf(stderr, "no port provided, shutting of...\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    printf("Server listening on port %d...\n", portno);
    
    while (1) {
        newsockfd = accept(sockfd,
            (struct sockaddr *) &cli_addr,
            &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }
        
        printf("Client connected!\n");
        
        bzero(buffer,256);

        n = read(newsockfd,buffer,255);
        
        if (n < 0) {
            perror("ERROR reading from socket");
            close(newsockfd);
            continue;
        }
        
        printf("Here is the message: %s\n",buffer);
        
        execute_c_code(buffer, output_buffer);
        
        n = write(newsockfd, output_buffer, strlen(output_buffer));

        if (n < 0) {
            perror("ERROR writing to socket");
        }

        close(newsockfd);
        printf("Client disconnected.\n\n");
    }
    
    close(sockfd);
    return 0;
}
