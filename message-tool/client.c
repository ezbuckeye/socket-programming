#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define INPUT_SIZE 1024
#define PACKET_SIZE 17
#define RECV_PACKET_SIZE 11
#define WINDOW_SIZE 10

void send_udp(void *buff, int size, int s, struct sockaddr_in addr);
void send_rdt(char *buff, int size, int s, struct sockaddr_in addr);

void send_udp(void *buff, int size, int s, struct sockaddr_in addr){
  int rc;
  rc = sendto(s, buff, size, 0, (struct sockaddr*)&addr, sizeof(addr));
  if(rc < 0){
    perror("An error occurs when sending udp datagram");
    exit(1);
  }
}

void send_rdt(char *buff, int size, int s, struct sockaddr_in addr){
  int rc;
  int window_base = 0;
  int lowest_ack = -1;
  char send_buf[PACKET_SIZE];
  char recv_buff[RECV_PACKET_SIZE];
  while(window_base < size){
    // Send All the Character Within Current Window
    for(int i = window_base; i < window_base+WINDOW_SIZE && i < size; i += 2){
      int data_size = i+1==size ? 1 : 2;
      sprintf(send_buf, "%11d%4d", i, data_size);
      send_buf[PACKET_SIZE-2] = buff[i];
      if(i+1!=size) send_buf[PACKET_SIZE-1] = buff[i+1];
      int send_size = PACKET_SIZE-2+data_size;
      send_udp(send_buf, send_size, s, addr);
      printf("sent %s\n", send_buf);
    }
    // Receive Ack from the Server and Update the Lowest Ack
    for(int i = window_base; i < window_base+WINDOW_SIZE && i < size; i += 2){
      socklen_t s_len = sizeof(addr);
      int seq_num;
      rc = recvfrom(s, recv_buff, RECV_PACKET_SIZE, 0, (struct sockaddr*)&addr, &s_len);
      if(rc >= 0){
        sscanf(recv_buff, "%11d", &seq_num);
        printf("received %d\n", seq_num);
        if(seq_num > lowest_ack)  lowest_ack = seq_num;
      }else{
        break;
      }
    }
    // Slide the Window
    if(lowest_ack>=window_base)  window_base = lowest_ack + 2;
  }
}

int main(int argc, char *argv[]){
  char input_buf[INPUT_SIZE];
  int rc; // return value
  int s;  // UDP socket
  struct sockaddr_in addr;  // server address
  if(argc != 3){
    printf("Please enter: %s <server_ip_address> <port>\n", argv[0]);
    exit(1);
  }
  // Create UDP Socket and Set Timeout
  s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(s < 0){
    perror("An error occurs when creating UDP socket");
    exit(1);
  }
  struct timeval tv;
  tv.tv_sec = 1;  // set receive timeout to 2 seconds
  tv.tv_usec = 0;
  rc = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
  if(rc < 0){
    perror("An error occurs when setting timeout");
    exit(1);
  }
  // Set Server Address
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  addr.sin_port = htons(atoi(argv[2]));
  // Ask Users for the message they want to send to server
  printf("Please enter a message: ");
  fgets(input_buf, INPUT_SIZE, stdin);
  // Send the First Message(input length) to the Server
  size_t len = strlen(input_buf)-1; // eliminate the '\n' character
  uint32_t net_len = htonl(len);
  send_udp(&net_len, 4, s, addr);
  // Send the Second Message(input) to the Server
  send_rdt(input_buf, len, s, addr);
  // Close the UDP socket
  close(s);

  return 0;
}