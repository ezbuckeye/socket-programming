#include <string.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <arpa/inet.h> 
#include <unistd.h> 

#define BUFFSIZE 1000

int main(int argc, char *argv[]) 
{ 
  int sd; 
  struct sockaddr_in server_address; 
  int portNumber; 
  char serverIP[29]; 
  int rc = 0; 
 
  if (argc < 3){ 
    printf ("usage is client <ipaddr> <port>\n"); 
    exit(1); 
  } 
 
  sd = socket(AF_INET, SOCK_STREAM, 0); 
 
  portNumber = strtol(argv[2], NULL, 10); 
  strcpy(serverIP, argv[1]); 
 
  server_address.sin_family = AF_INET; 
  server_address.sin_port = htons(portNumber); 
  server_address.sin_addr.s_addr = inet_addr(serverIP); 
 
  if(connect(sd, (struct sockaddr *)&server_address, sizeof(struct 
sockaddr_in)) < 0) { 
    close(sd); 
    perror("error connecting stream socket"); 
    exit(1); 
  } 
  
  char filenameToBeSent[20], filenameToBeSaved[20];
  FILE *fileToBeSaved;
  uint32_t sizeOfFilenameToBeSaved, sizeOfFileToBeSaved, totalBytesClientSent, totalBytesServerRead;
  unsigned char buffer[BUFFSIZE];
  int numberOfBytes;
  
  while (1){
    printf("Please input the filename to be sent: ");
    scanf("%s", filenameToBeSent);
    // iff the filename is "DONE", then quit the loop
    if(!strcmp(filenameToBeSent, "DONE")){
      break;
    }
    if ((fileToBeSaved = fopen(filenameToBeSent, "rb")) != NULL) {
      printf("Please input the filename to be saved: ");
      scanf("%s", filenameToBeSaved);

      // send the size of file name
      sizeOfFilenameToBeSaved = htonl(strlen(filenameToBeSaved));
      rc = write(sd, &sizeOfFilenameToBeSaved, sizeof(sizeOfFilenameToBeSaved));
      if(rc < 0)  printf("write error");
      printf("The size of filename(%lu) has been sent.\n", htonl(sizeOfFilenameToBeSaved));

      // send the file name
      rc = write(sd, filenameToBeSaved, htonl(sizeOfFilenameToBeSaved));
      if(rc < 0)  printf("write error");
      printf("The filename(%s) has been sent.\n", filenameToBeSaved);
      
      // send the size of file
      // get the file size
      fseek(fileToBeSaved, 0, SEEK_END);
      sizeOfFileToBeSaved = htonl(ftell(fileToBeSaved));
      fseek(fileToBeSaved, 0, SEEK_SET);
      rc = write(sd, &sizeOfFileToBeSaved, sizeof(sizeOfFileToBeSaved));
      if(rc < 0)  printf("write error");
      printf("The size of file(%lu) has been sent.\n", htonl(sizeOfFileToBeSaved));

      // send the file
      printf("********start sending the file**************\n");
      numberOfBytes = fread(buffer, 1, BUFFSIZE, fileToBeSaved);
      printf("The client read %d bytes from the file \"%s\"\n", numberOfBytes, filenameToBeSent);
      totalBytesClientSent = 0;
      while(numberOfBytes > 0){
	      rc = write(sd, buffer, numberOfBytes);
        if(rc < 0)  printf("write error");
        printf("The client wrote %d bytes to the server\n", rc, filenameToBeSent);
        printf("=====\n");
        numberOfBytes = fread(buffer, 1, BUFFSIZE, fileToBeSaved);
        printf("The client read %d bytes from the file \"%s\"\n", numberOfBytes, filenameToBeSent);
        totalBytesClientSent += rc;
      }
      printf("********finish sending the file**************\n");
      printf("The client has sent %lu bytes.\n", totalBytesClientSent);
      rc = read(sd, &totalBytesServerRead, sizeof(totalBytesServerRead));
      totalBytesServerRead = htonl(totalBytesServerRead);
      printf("The server has read %lu bytes.\n", totalBytesServerRead);
    }else{
      printf("the file [%s] doesn't exist! Please enter again or enter \"DONE\" to quit the process.\n", filenameToBeSent);
    }
    
  }
 
  return 0 ;  
 
} 