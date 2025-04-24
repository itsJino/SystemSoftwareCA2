/* client.c - Implementation of the client socket program
 * Systems Software Continuous Assessment 2
 *
 * This file implements the client functionality:
 * - Socket connection to server
 * - User authentication
 * - File selection and transfer
 * - Status reporting
 */

 #include "client.h"

 /* Main function */
 int main(int argc, char *argv[]) {
     int server_socket;
     char filepath[MAX_PATH_LENGTH] = {0};
     char target_dir[64] = {0};
     int status_code;
     
     /* Display usage if arguments are not provided correctly */
     if (argc != 3) {
         display_usage();
         return EXIT_FAILURE;
     }
     
     /* Parse command line arguments */
     strncpy(filepath, argv[1], MAX_PATH_LENGTH - 1);
     strncpy(target_dir, argv[2], 63);
     
     /* Validate target directory */
     if (strcmp(target_dir, MANUFACTURING_DIR) != 0 && strcmp(target_dir, DISTRIBUTION_DIR) != 0) {
         fprintf(stderr, "Error: Target directory must be either 'Manufacturing' or 'Distribution'\n");
         display_usage();
         return EXIT_FAILURE;
     }
     
     /* Validate file path */
     struct stat st;
     if (stat(filepath, &st) < 0 || !S_ISREG(st.st_mode)) {
         fprintf(stderr, "Error: File '%s' does not exist or is not a regular file\n", filepath);
         return EXIT_FAILURE;
     }
     
     /* Connect to server */
     server_socket = connect_to_server();
     if (server_socket == -1) {
         fprintf(stderr, "Failed to connect to server. Exiting.\n");
         return EXIT_FAILURE;
     }
     
     printf("Connected to server at %s:%d\n", SERVER_IP, PORT);
     
     /* Send file to server */
     status_code = send_file(server_socket, filepath, target_dir);
     
     /* Display transfer status */
     display_status_message(status_code);
     
     /* Clean up resources */
     cleanup_client(server_socket);
     
     return (status_code == STATUS_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
 
 /* Connect to the server */
 int connect_to_server(void) {
     int server_socket;
     struct sockaddr_in server_addr;
     
     /* Create socket */
     server_socket = socket(AF_INET, SOCK_STREAM, 0);
     if (server_socket < 0) {
         perror("socket");
         return -1;
     }
     
     /* Prepare the sockaddr_in structure */
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(PORT);
     
     /* Convert IP address to binary form */
     if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
         perror("inet_pton");
         close(server_socket);
         return -1;
     }
     
     /* Connect to server */
     if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
         perror("connect");
         close(server_socket);
         return -1;
     }
     
     return server_socket;
 }
 
 /* Get current username */
 char *get_current_username(void) {
     uid_t uid = getuid();
     struct passwd *pw = getpwuid(uid);
     
     if (!pw) {
         perror("getpwuid");
         return NULL;
     }
     
     return pw->pw_name;
 }
 
 /* Send file to server */
 int send_file(int server_socket, const char *filepath, const char *target_dir) {
     char *username = get_current_username();
     char filename[MAX_PATH_LENGTH] = {0};
     int file_fd;
     ssize_t bytes_read, bytes_sent;
     char buffer[BUFFER_SIZE] = {0};
     long filesize;
     int status_code = STATUS_UNKNOWN_ERROR;
     int ready;
     
     /* Check if username was successfully retrieved */
     if (!username) {
         fprintf(stderr, "Failed to get username\n");
         return STATUS_UNKNOWN_ERROR;
     }
     
     /* Extract filename from filepath */
     const char *last_slash = strrchr(filepath, '/');
     if (last_slash) {
         strncpy(filename, last_slash + 1, MAX_PATH_LENGTH - 1);
     } else {
         strncpy(filename, filepath, MAX_PATH_LENGTH - 1);
     }
     
     /* Send username to server */
     if (send(server_socket, username, strlen(username), 0) < 0) {
         perror("send username");
         return STATUS_UNKNOWN_ERROR;
     }
     
     /* Send target directory to server */
     if (send(server_socket, target_dir, strlen(target_dir), 0) < 0) {
         perror("send target directory");
         return STATUS_UNKNOWN_ERROR;
     }
     
     /* Send filename to server */
     if (send(server_socket, filename, strlen(filename), 0) < 0) {
         perror("send filename");
         return STATUS_UNKNOWN_ERROR;
     }
     
     /* Get file size */
     filesize = get_file_size(filepath);
     if (filesize < 0) {
         fprintf(stderr, "Failed to get file size for '%s'\n", filepath);
         return STATUS_FILE_ERROR;
     }
     
     /* Send file size to server */
     if (send(server_socket, &filesize, sizeof(filesize), 0) < 0) {
         perror("send filesize");
         return STATUS_UNKNOWN_ERROR;
     }
     
     /* Wait for server ready signal */
     if (recv(server_socket, &ready, sizeof(ready), 0) <= 0) {
         perror("recv ready signal");
         return STATUS_UNKNOWN_ERROR;
     }
     
     /* Open file for reading */
     file_fd = open(filepath, O_RDONLY);
     if (file_fd < 0) {
         perror("open file");
         return STATUS_FILE_ERROR;
     }
     
     /* Send file data */
     printf("Sending file: %s (%ld bytes)\n", filename, filesize);
     
     while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
         bytes_sent = send(server_socket, buffer, bytes_read, 0);
         if (bytes_sent < 0) {
             perror("send file data");
             close(file_fd);
             return STATUS_UNKNOWN_ERROR;
         }
         
         printf("Sent %zd bytes\n", bytes_sent);
     }
     
     /* Close file */
     close(file_fd);
     
     /* Receive status code from server */
     if (recv(server_socket, &status_code, sizeof(status_code), 0) <= 0) {
         perror("recv status code");
         return STATUS_UNKNOWN_ERROR;
     }
     
     return status_code;
 }
 
 /* Display transfer status message */
 void display_status_message(int status_code) {
     switch (status_code) {
         case STATUS_SUCCESS:
             printf("File transfer successful.\n");
             break;
         case STATUS_PERMISSION_DENIED:
             printf("Permission denied. You do not have access to the target directory.\n");
             break;
         case STATUS_FILE_ERROR:
             printf("File transfer failed due to a file-related error.\n");
             break;
         case STATUS_UNKNOWN_ERROR:
         default:
             printf("File transfer failed due to an unknown error.\n");
             break;
     }
 }
 
 /* Get file size */
 long get_file_size(const char *filepath) {
     struct stat st;
     
     if (stat(filepath, &st) < 0) {
         perror("stat");
         return -1;
     }
     
     return st.st_size;
 }
 
 /* Display usage instructions */
 void display_usage(void) {
     printf("Usage: client <filepath> <target_directory>\n");
     printf("  filepath: Path to the file you want to transfer\n");
     printf("  target_directory: Either 'Manufacturing' or 'Distribution'\n");
     printf("\nExample: ./client /path/to/myfile.txt Manufacturing\n");
 }
 
 /* Clean up resources */
 void cleanup_client(int server_socket) {
     /* Close server socket */
     close(server_socket);
 }