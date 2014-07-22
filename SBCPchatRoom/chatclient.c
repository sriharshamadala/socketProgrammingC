/*
 * Name: chatclient.c
 * Descriptions: This client program can chat with other client
 * within a chat room running upon a chat client using Simple Broadcasting
 * Chat Protocol (SBCP). The encryption of message is using the SBCP packet
 * structure. It has the functionalities of: Sending JOIN request to join 
 * the chat room, send message to other clients in the chat room, etc.
 * Version 5.0
 * Author: Bolun Huang, Sriharsha Madala.         
 * Date: 09/17/2012
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <inttypes.h>

/*
 * The main function of chatclient
 * The command line argument should be: ./<executable file name> <username> <server_ip> <server_port>
 */
int main(int argc, char** argv) {
  int csd, check;  // file descriptor of client and return value 
  struct sockaddr_in addr;  // socket address of client
  struct hostent* hret;
  struct timeval tv;  // time structure
  short port = 0;  // initialize the port number
  char buff[520];  // message buffer
  char username[20];  // username of client
  memcpy(username, (const char *) argv[1],sizeof(username));
  fd_set readfds;  // client file descriptor set
  tv.tv_sec = 10;
  tv.tv_usec = 10000000;
  int ct = 0;  // flag
  // Define and initialize the structure of SBCP packets, including protocol header and message header
  struct header_sbcp {
    uint16_t vrsn : 9;
    uint16_t type : 7;
    uint16_t len;
    struct header_attr {
      uint16_t type_attr;
      uint16_t len_attr;
      char payload[520];
    } msgattr;
  } sbcpmsg, sbcpmsg_r, sbcpidle;
  // create socket
  if ((csd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Cannot create client socket.\n");
    exit(EXIT_FAILURE);
  }
  port = atoi(argv[3]);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  hret = gethostbyname(argv[2]);
  memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);
  // connect to server
  if(connect(csd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1) {
    perror("Cannot connect to Server");
    exit(EXIT_FAILURE);
  } else {
    // define the JOIN message with "username" (type_attr = 2)
    memcpy(sbcpmsg.msgattr.payload, (const char *) username, sizeof(username));
    sbcpmsg.msgattr.type_attr = 2;
    sbcpmsg.msgattr.len_attr = sizeof(int) + sizeof(sbcpmsg.msgattr.payload);
    sbcpmsg.vrsn = 3;
    sbcpmsg.type = 2;
    sbcpmsg.len = sizeof(int) + sbcpmsg.msgattr.len_attr;

    //ready to send JOIN and receive ACK
    FD_ZERO(&readfds);
    FD_SET(0,&readfds);
    FD_SET(csd,&readfds);
    select(csd+1, &readfds, NULL, NULL, &tv);
  }
  
  // The loop    
  while (1) {
    tv.tv_sec = 10;
    tv.tv_usec = 10000000;
    FD_ZERO(&readfds);
    FD_SET(0,&readfds);
    FD_SET(csd,&readfds);
    select(csd+1, &readfds, NULL, NULL, &tv);
    if(FD_ISSET(0,&readfds)){ // handling the sending messages
      // define the  JOIN message.
      if (ct == 0) {
        memcpy(sbcpmsg.msgattr.payload, (const char *) username, sizeof(username));
        sbcpmsg.msgattr.type_attr = 2;
        sbcpmsg.msgattr.len_attr = sizeof(int) + sizeof(sbcpmsg.msgattr.payload);
        sbcpmsg.vrsn = 3;
        sbcpmsg.type = 2;
        sbcpmsg.len = sizeof(int) + sbcpmsg.msgattr.len_attr;
        // send the JOIN first!
        if(send(csd, (char *) &sbcpmsg, sizeof(sbcpmsg),0)==-1) {
          perror("Error in sending JOIN message.\n");
        }
      }
      ct = 1; // mark that it is not the first loop any more...
      // Send my message.
      gets(buff);// gets message on the command line..
      // define the SEND message information
      memcpy(sbcpmsg.msgattr.payload, (const char *) buff, sizeof(sbcpmsg.msgattr.payload));
      sbcpmsg.msgattr.type_attr = 4;
      sbcpmsg.msgattr.len_attr = sizeof(int) + sizeof(sbcpmsg.msgattr.payload);
      sbcpmsg.vrsn = 3;
      sbcpmsg.type = 3;
      sbcpmsg.len = sizeof(int) + sbcpmsg.msgattr.len_attr;
      // handle the sending of SEND message
      if (send(csd, (char *) &sbcpmsg, sizeof(sbcpmsg),0)==-1) {
        perror("Error in sending the message.\n");
      }
    } else if(FD_ISSET(csd,&readfds)) { // handling the receiving packets
      check = recv(csd,(char *) &sbcpmsg_r,sizeof(sbcpmsg_r),0);
      if(check<=0) {
        if (check == 0) {
          // connection closed
          return 0;
        } else {
        perror("Error in receiving the message.\n");
        }
      } else { // received successfully
        if (sbcpmsg_r.type == 4) {
          //FWD
          if (sbcpmsg_r.msgattr.type_attr == 2) { // FWD_Username
            printf("%s :", sbcpmsg_r.msgattr.payload);
          } else if (sbcpmsg_r.msgattr.type_attr == 4) { // FWD_Message
            printf("%s\n", sbcpmsg_r.msgattr.payload);
          } else {
            printf("Error FWD attribute types!\n");
            return 0;
          }
        } else if (sbcpmsg_r.type == 7) {
          //ACK
          if (sbcpmsg_r.msgattr.type_attr == 3) { // ACK_Client count
            printf("<Number of Clients: %s.\n", sbcpmsg_r.msgattr.payload);
          } else if (sbcpmsg_r.msgattr.type_attr == 2) { // ACK_Username_list
            printf("<List of Clients: %s.\n", sbcpmsg_r.msgattr.payload);
          } else {
            printf("Error ACK attribute types!\n");
          }
        } else if (sbcpmsg_r.type == 5) {
          //NAK
          printf("%s", sbcpmsg_r.msgattr.payload);// The reason
          printf("Please use a different user name to try again!\n");
          exit(0);
        } else if (sbcpmsg_r.type == 8) {
          //ONLINE
          printf("<%s> JOIN the chat!\n", sbcpmsg_r.msgattr.payload);
        } else if (sbcpmsg_r.type == 6) {
          //OFFLINE
          printf("<%s> EXIT the chat!\n", sbcpmsg_r.msgattr.payload);
        } else if (sbcpmsg_r.type == 9) {
          //IDLE of some other client
          printf("<%s> is IDLE.\n", sbcpmsg_r.msgattr.payload);
        } else {
          printf("Error header type!\n");
        }
      }
    } else {
      // define IDLE from client side
      sbcpidle.msgattr.len_attr = sizeof(int) + sizeof(sbcpidle.msgattr.payload);
      sbcpidle.vrsn = 3;
      sbcpidle.type = 9;
      sbcpidle.len = sizeof(int) + sbcpidle.msgattr.len_attr;
      // send IDLE packet without carrying anything
      if (send(csd, (char *) &sbcpidle, sizeof(sbcpidle),0)==-1) {
        printf("Error in sending the IDLE message.\n");
      }
      tv = (struct timeval) {0};
    }  // End of file descriptor selection
  }  // End of connection loop
close(csd);
return 0;
}
