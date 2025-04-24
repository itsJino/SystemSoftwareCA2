/* server.c - Implementation of the server socket program
 * Systems Software Continuous Assessment 2
 *
 * This file implements the server functionality:
 * - Socket initialization and connection handling
 * - Multithreaded client processing
 * - File transfer management with user permissions
 * - File ownership attribution
 * - Thread synchronization using mutex
 */

 #include "server.h"

 /* Global variables */
 pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
 int active_clients = 0;
 
 /* Main function */
 int main(int argc, char *argv[]) {
     int server_socket, client_socket;
     struct sockaddr_in client_addr;
     socklen_t client_addr_len = sizeof(client_addr);
     pthread_t thread_id;
     
     /* Initialize server socket */
     server_socket = initialize_server();
     if (server_socket == -1) {
         fprintf(stderr, "Failed to initialize server. Exiting.\n");
         return EXIT_FAILURE;
     }
     
     printf("Server initialized. Listening on port %d...\n", PORT);
     
     /* Accept and handle client connections */
     while (1) {
         /* Accept new client connection */
         client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
         if (client_socket < 0) {
             perror("accept");
             continue;
         }
         
         /* Check if maximum clients limit reached */
         if (active_clients >= MAX_CLIENTS) {
             printf("Maximum clients reached. Rejecting connection.\n");
             close(client_socket);
             continue;
         }
         
         /* Create client data structure */
         client_t *client = (client_t *)malloc(sizeof(client_t));
         if (!client) {
             perror("malloc");
             close(client_socket);
             continue;
         }
         
         /* Initialize client data */
         client->client_socket = client_socket;
         client->client_addr = client_addr;
         client->client_id = active_clients++;
         
         printf("New connection from %s:%d. Client ID: %d\n", 
                inet_ntoa(client_addr.sin_addr), 
                ntohs(client_addr.sin_port),
                client->client_id);
         
         /* Create thread to handle client */
         if (pthread_create(&thread_id, NULL, handle_client, (void *)client) != 0) {
             perror("pthread_create");
             free(client);
             close(client_socket);
             active_clients--;
             continue;
         }
         
         /* Detach thread to allow resources to be freed automatically */
         pthread_detach(thread_id);
     }
     
     /* Clean up (Note: This point is not reached in the current implementation) */
     cleanup_server(server_socket);
     
     return EXIT_SUCCESS;
 }
 
 /* Initialize server socket */
 int initialize_server(void) {
     int server_socket, opt = 1;
     struct sockaddr_in server_addr;
     
     /* Create socket */
     server_socket = socket(AF_INET, SOCK_STREAM, 0);
     if (server_socket < 0) {
         perror("socket");
         return -1;
     }
     
     /* Set socket options */
     if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
         perror("setsockopt");
         close(server_socket);
         return -1;
     }
     
     /* Prepare the sockaddr_in structure */
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = INADDR_ANY;
     server_addr.sin_port = htons(PORT);
     
     /* Bind the socket */
     if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
         perror("bind");
         close(server_socket);
         return -1;
     }
     
     /* Listen for connections */
     if (listen(server_socket, MAX_CLIENTS) < 0) {
         perror("listen");
         close(server_socket);
         return -1;
     }
     
     return server_socket;
 }
 
 /* Handle client connection in a separate thread */
 void *handle_client(void *arg) {
     client_t *client = (client_t *)arg;
     int client_socket = client->client_socket;
     char buffer[BUFFER_SIZE] = {0};
     char username[64] = {0};
     char target_dir[64] = {0};
     char filename[MAX_PATH_LENGTH] = {0};
     int status_code;
     
     /* Receive username from client */
     if (recv(client_socket, username, sizeof(username) - 1, 0) <= 0) {
         perror("recv username");
         goto cleanup;
     }
     
     printf("Client %d identified as user: %s\n", client->client_id, username);
     
     /* Receive target directory from client */
     if (recv(client_socket, target_dir, sizeof(target_dir) - 1, 0) <= 0) {
         perror("recv target_dir");
         goto cleanup;
     }
     
     printf("Client %d requested transfer to directory: %s\n", client->client_id, target_dir);
     
     /* Receive filename from client */
     if (recv(client_socket, filename, sizeof(filename) - 1, 0) <= 0) {
         perror("recv filename");
         goto cleanup;
     }
     
     printf("Client %d requested transfer of file: %s\n", client->client_id, filename);
     
     /* Process file transfer request */
     status_code = process_file_transfer(client_socket, username, target_dir, filename);
     
     /* Send status code back to client */
     if (send(client_socket, &status_code, sizeof(status_code), 0) < 0) {
         perror("send status code");
     }
     
     /* Clean up after client handling */
 cleanup:
     close(client_socket);
     free(client);
     active_clients--;
     
     printf("Client %d disconnected. Total active clients: %d\n", client->client_id, active_clients);
     
     pthread_exit(NULL);
 }
 
 /* Process file transfer request from client */
 int process_file_transfer(int client_socket, const char *username, const char *target_dir, const char *filename) {
     char full_target_dir[MAX_PATH_LENGTH] = {0};
     char target_path[MAX_PATH_LENGTH] = {0};
     int file_fd;
     ssize_t bytes_read, bytes_written;
     char buffer[BUFFER_SIZE] = {0};
     long filesize = 0;
     long total_received = 0;
     
     /* Determine the full target directory path */
     if (strcmp(target_dir, MANUFACTURING_DIR) == 0) {
         strcpy(full_target_dir, MANUFACTURING_DIR);
     } else if (strcmp(target_dir, DISTRIBUTION_DIR) == 0) {
         strcpy(full_target_dir, DISTRIBUTION_DIR);
     } else {
         fprintf(stderr, "Invalid target directory: %s\n", target_dir);
         return STATUS_PERMISSION_DENIED;
     }
     
     /* Verify user access to the target directory */
     if (!verify_user_access(username, target_dir)) {
         fprintf(stderr, "User %s does not have permission to access %s\n", username, target_dir);
         return STATUS_PERMISSION_DENIED;
     }
     
     /* Create the target file path */
     snprintf(target_path, MAX_PATH_LENGTH, "%s/%s", full_target_dir, filename);
     
     /* Receive file size */
     if (recv(client_socket, &filesize, sizeof(filesize), 0) <= 0) {
         perror("recv filesize");
         return STATUS_UNKNOWN_ERROR;
     }
     
     printf("Expected file size: %ld bytes\n", filesize);
     
     /* Lock mutex for file operation */
     pthread_mutex_lock(&file_mutex);
     
     /* Open target file for writing */
     file_fd = open(target_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
     if (file_fd < 0) {
         perror("open target file");
         pthread_mutex_unlock(&file_mutex);
         return STATUS_FILE_ERROR;
     }
     
     /* Acknowledge ready to receive file */
     int ready = 1;
     if (send(client_socket, &ready, sizeof(ready), 0) < 0) {
         perror("send ready");
         close(file_fd);
         pthread_mutex_unlock(&file_mutex);
         return STATUS_UNKNOWN_ERROR;
     }
     
     /* Receive and write file data */
     while (total_received < filesize) {
         bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
         if (bytes_read <= 0) {
             perror("recv file data");
             close(file_fd);
             pthread_mutex_unlock(&file_mutex);
             return STATUS_FILE_ERROR;
         }
         
         bytes_written = write(file_fd, buffer, bytes_read);
         if (bytes_written != bytes_read) {
             perror("write file data");
             close(file_fd);
             pthread_mutex_unlock(&file_mutex);
             return STATUS_FILE_ERROR;
         }
         
         total_received += bytes_read;
     }
     
     /* Close file */
     close(file_fd);
     
     /* Set file ownership to the user who transferred it */
     if (set_file_ownership(target_path, username) != 0) {
         fprintf(stderr, "Failed to set file ownership for %s\n", target_path);
         pthread_mutex_unlock(&file_mutex);
         return STATUS_FILE_ERROR;
     }
     
     /* Unlock mutex */
     pthread_mutex_unlock(&file_mutex);
     
     printf("File transfer completed: %s -> %s\n", filename, target_path);
     
     return STATUS_SUCCESS;
 }
 
 /* Verify user permissions for accessing a directory */
 int verify_user_access(const char *username, const char *target_dir) {
     struct passwd *pw;
     struct group *gr;
     gid_t *groups = NULL;
     int ngroups = 0;
     int i, has_access = 0;
     
     /* Look up user information */
     pw = getpwnam(username);
     if (!pw) {
         fprintf(stderr, "User not found: %s\n", username);
         return 0;
     }
     
     /* Get groups count */
     getgrouplist(username, pw->pw_gid, NULL, &ngroups);
     
     /* Allocate memory for groups array */
     groups = malloc(ngroups * sizeof(gid_t));
     if (!groups) {
         perror("malloc");
         return 0;
     }
     
     /* Get user's groups */
     getgrouplist(username, pw->pw_gid, groups, &ngroups);
     
     /* Check if user is in the appropriate group based on target directory */
     const char *required_group = NULL;
     
     if (strcmp(target_dir, MANUFACTURING_DIR) == 0) {
         required_group = "manufacturing";
     } else if (strcmp(target_dir, DISTRIBUTION_DIR) == 0) {
         required_group = "distribution";
     } else {
         free(groups);
         return 0;
     }
     
     /* Look up the required group */
     gr = getgrnam(required_group);
     if (!gr) {
         fprintf(stderr, "Group not found: %s\n", required_group);
         free(groups);
         return 0;
     }
     
     /* Check if user is a member of the required group */
     for (i = 0; i < ngroups; i++) {
         if (groups[i] == gr->gr_gid) {
             has_access = 1;
             break;
         }
     }
     
     free(groups);
     return has_access;
 }
 
 /* Set file ownership to the user who transferred it */
 int set_file_ownership(const char *filepath, const char *username) {
     struct passwd *pw;
     
     /* Look up user information */
     pw = getpwnam(username);
     if (!pw) {
         fprintf(stderr, "User not found: %s\n", username);
         return -1;
     }
     
     /* Change file ownership */
     if (chown(filepath, pw->pw_uid, pw->pw_gid) < 0) {
         perror("chown");
         return -1;
     }
     
     return 0;
 }
 
 /* Get username from UID */
 char *get_username_from_uid(uid_t uid) {
     struct passwd *pw;
     
     pw = getpwuid(uid);
     if (!pw) {
         return NULL;
     }
     
     return pw->pw_name;
 }
 
 /* Clean up resources */
 void cleanup_server(int server_socket) {
     /* Close server socket */
     close(server_socket);
     
     /* Destroy mutex */
     pthread_mutex_destroy(&file_mutex);
 }