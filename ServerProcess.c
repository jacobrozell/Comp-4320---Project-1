#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#define PACKET_SIZE 128

FILE *of;
int serverSocket, newSocket;
char buffer[1024];
struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
struct sockaddr_storage serverStorage;
socklen_t addressSize;
socklen_t cLength = sizeof(clientAddr);

/*---- Connects to the Client ----*/
int clientConnect() {
   serverSocket = socket(PF_INET, SOCK_DGRAM, 0); // Creates socket with UDP
   
   bzero((char *) &serverAddr, sizeof(serverAddr)); // Clear address structure

   // Configure Client address settings
   serverAddr.sin_family = AF_INET; // Set address family to internet
   serverAddr.sin_port = htons(10042); // Set port number using htons for correct byte order
   serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically filled with current host's IP address
   memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero)); // Set padding bits to 0

   // Bind the address to the socket
   bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

   // recvfrom(serverSocket, buffer, 1024, 0, (struct sockaddr *) &clientAddr, &cLength); -- causes the program to not have any output, even with print statements included
   
   // Listen on the socket; max connection requests in the queue == 5
   if(listen(serverSocket, 5) == 0) {
      printf("Listening...\n");
   } else {
      printf("Error!\n");
      exit(EXIT_FAILURE);
   }
   
	// Creates a new socket for the incoming connection and connect them
   addressSize = sizeof(serverStorage);
   newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addressSize);
   connect(newSocket, (struct sockaddr *) &serverStorage, addressSize);
   
   return 0;
}

/*---- Calculates the checksum ----*/
int calculateChecksum(char packet[]) {
   // Variable declarations
   int sum = 0;
   int i;
   
   // Calculates the sum
   for(i = 1; i < 11; i++) {
      sum += packet[i];
   }
   
   // Returns true (1) if checksum contains all 3's; otherwise, returns false (0)
   if(sum % 3 == 0) {
      return 1;
   } else {
      return 0;
   }
}

/*---- Receives a packet with the data from the file ----*/
int receiveMessage() {
   // Variable declarations
   char c;
   int packetNumber = 1;
   
   of = fopen("write_file", "w");
   
   // Checks if file opened properly
   if(of == NULL) {
      printf("Can't open output file");
      exit(1);
   }
   
   for(;;) {
      int passed;
      recv(newSocket, buffer, PACKET_SIZE, 0);
   
      printf("\nReceived packet %d...\n", packetNumber);
      packetNumber++;
      printf("The packet is being checked for errors...\n");
      passed = calculateChecksum(buffer);
      if(passed == 1) {
         printf("The packet was valid!\n");
         int i;
         for(i = 11; i < PACKET_SIZE; i++) {
            fputc(buffer[i], of);
            c = buffer[i];
         }
      
         send(newSocket, buffer, PACKET_SIZE, 0);
      } else { // Packet corrupted
         printf("The packet was corrupted!\n");
         buffer[0] = '1';
         send(newSocket, buffer, PACKET_SIZE, 0);
      }
      sleep(5);
   }
   
   fclose(of); // Close file
   return 0;
}

/*---- Main function ----*/
int main() {
   clientConnect();
   receiveMessage();
   
   // Variable declarations
   char c;
   int pos = 0;
   
   of = fopen("write_file", "r");
   
   // Checks if file opened properly
   if(of == NULL) {
      printf("Can't open output file");
      exit(1);
   }
   
   fseek(of, 0L, SEEK_END);
   int size = ftell(of);
   fseek(of, 0L, SEEK_SET);
   printf("\n\nData from file: \n");
 
   // Prints all data in file
   while((c = fgetc(of)) != EOF) {
      if(pos < size - 1) {
         printf("%c", c);
      }
      pos++;
   }
   fclose(of); // closes file

   return 0;
}
