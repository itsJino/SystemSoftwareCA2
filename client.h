/* client.h - Header file for the client socket program
 * Systems Software Continuous Assessment 2
 * 
 * This file contains declarations for the client program including:
 * - Constants for client configuration
 * - Function prototypes for client operations
 */

 #ifndef CLIENT_H
 #define CLIENT_H
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <pwd.h>
 #include <errno.h>
 
 /* Client configuration constants */
 #define SERVER_IP "127.0.0.1"
 #define PORT 8080
 #define BUFFER_SIZE 1024
 #define MAX_PATH_LENGTH 256
 
 /* Available transfer destinations */
 #define MANUFACTURING_DIR "Manufacturing"
 #define DISTRIBUTION_DIR "Distribution"
 
 /* Status codes from server responses */
 #define STATUS_SUCCESS 0
 #define STATUS_PERMISSION_DENIED 1
 #define STATUS_FILE_ERROR 2
 #define STATUS_UNKNOWN_ERROR 3
 
 /* Function prototypes */
 
 /* Connect to the server */
 int connect_to_server(void);
 
 /* Get current username */
 char *get_current_username(void);
 
 /* Send file to server */
 int send_file(int server_socket, const char *filepath, const char *target_dir);
 
 /* Display transfer status message */
 void display_status_message(int status_code);
 
 /* Get file size */
 long get_file_size(const char *filepath);
 
 /* Display usage instructions */
 void display_usage(void);
 
 /* Clean up resources */
 void cleanup_client(int server_socket);
 
 #endif /* CLIENT_H */