/* server.h - Header file for the server socket program
 * Systems Software Continuous Assessment 2
 * 
 * This file contains declarations for the server program including:
 * - Constants for server configuration
 * - Data structures for client handling
 * - Function prototypes for server operations
 */

 #ifndef SERVER_H
 #define SERVER_H
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <pthread.h>
 #include <errno.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <pwd.h>
 #include <grp.h>
 
 /* Server configuration constants */
 #define PORT 8080
 #define BUFFER_SIZE 1024
 #define MAX_CLIENTS 10
 #define MAX_PATH_LENGTH 256
 
 /* Directory paths */
 #define MANUFACTURING_DIR "./Manufacturing"
 #define DISTRIBUTION_DIR "./Distribution"
 
 /* Status codes for client responses */
 #define STATUS_SUCCESS 0
 #define STATUS_PERMISSION_DENIED 1
 #define STATUS_FILE_ERROR 2
 #define STATUS_UNKNOWN_ERROR 3
 
 /* Thread synchronization mutex */
 extern pthread_mutex_t file_mutex;
 
 /* Client connection data structure */
 typedef struct {
     int client_socket;
     struct sockaddr_in client_addr;
     int client_id;
 } client_t;
 
 /* Function prototypes */
 
 /* Initialize server socket and start listening for connections */
 int initialize_server(void);
 
 /* Handle client connection in a separate thread */
 void *handle_client(void *arg);
 
 /* Process file transfer request from client */
 int process_file_transfer(int client_socket, const char *username, const char *target_dir, const char *filename);
 
 /* Verify user permissions for accessing a directory */
 int verify_user_access(const char *username, const char *target_dir);
 
 /* Set file ownership to the user who transferred it */
 int set_file_ownership(const char *filepath, const char *username);
 
 /* Get username from UID */
 char *get_username_from_uid(uid_t uid);
 
 /* Clean up resources */
 void cleanup_server(int server_socket);
 
 #endif /* SERVER_H */