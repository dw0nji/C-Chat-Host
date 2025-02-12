#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>

#define PORT 1234

pid_t pid = -1;

void send_message(int client_fd, char* message);
void childProcess(int client_fd, char *buffer);

int main(void)
{	
	 
	int server_fd, client_fd;
    struct sockaddr_in serv_addr;
	socklen_t serv_size = sizeof(serv_addr);
    char buffer[1024] = { 0 };
	
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
		close(server_fd);
        return -1;
    }
	
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //allow the server to accept a client connection on any interface
	
	if(bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("could not connect to a server \n");
		close(server_fd);
		return -1;
	
	} 
	
	if (listen(server_fd, 3) < 0) {
        perror("listen");
		close(server_fd);
        return -1;
    }
	
    if ((client_fd = accept (server_fd, (struct sockaddr *) &serv_addr, &serv_size))
        < 0) {
        printf("\nConnection Failed \n");
		close(server_fd);
		close(client_fd);
        return -1;
    }
	
	childProcess(client_fd,buffer);
	
	char currentInput[512]; 
	
	while (true){
		currentInput[0] = '\0';
		fgets(currentInput, 512, stdin);
		
		if (strcmp(currentInput, "\n") == 0 || strcmp(currentInput, "exit\n") == 0 ) {
			break;
		}
		send_message(client_fd, currentInput);
		printf("send: %s",currentInput);
		
	}
	
	printf("closed");
	kill(pid,SIGTERM);
	close(client_fd);
	close(server_fd);

    return 0;
}

void send_message(int client_fd, char* message){
	
	//----------------add length header to the start of the message and send-----
	uint16_t length = htons(strlen(message));  // Convert to big-endian
	char buffer2[2 + strlen(message)];  

	memcpy(buffer2, &length, 2);        
	memcpy(buffer2 + 2, message, strlen(message));  

	write(client_fd, buffer2, sizeof(buffer2)); 
	//--------------------------------------------------------
	
}
	

void childProcess(int client_fd, char *buffer) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        while (1) {
			
			//checks if header has been received, if so it reads for more input until the message size has been met before we repeat reading for another message
            ssize_t total_bytes_received = 0;
            ssize_t header_bytes_received = read(client_fd, buffer, 2); // 2 bytes for header

            if (header_bytes_received < 2) {
                perror("Failed to read header");
                break;
            }

        
            uint16_t message_size = (unsigned char)buffer[0] << 8 | (unsigned char)buffer[1];

         
            char *message_buffer = malloc(message_size + 1); //mallocing
            if (message_buffer == NULL) {
                perror("malloc failed");
                break;
            }

            
            while (total_bytes_received < message_size) {
                ssize_t bytes_received = read(client_fd, message_buffer + total_bytes_received, message_size - total_bytes_received);
                if (bytes_received <= 0) {
                    perror("Failed to read message");
                    free(message_buffer);
                    break;
                }
                total_bytes_received += bytes_received;
            }

          
            message_buffer[message_size] = '\0';
            printf("Message: %s\n", message_buffer);

        
            free(message_buffer);//giving back memory :D
        }
        exit(0);
    }

}
