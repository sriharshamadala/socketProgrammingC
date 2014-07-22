/*
 * Name: chatserver.c
 * Descriptions: a multi-client chat server using Simple Broadcasting
 * Chat Protocol (SBCP). This server program provides a chat room for
 * those chating clients who are connected to the server. Becides, this
 * server employs specifically SBCP as its encoding and decoding protocol.
 * It has the functionalities of: Accepting JOIN request from every chater,
 * acknowledging the signalling messages from each clientsi, relaying
 * SEND message from one chater to the other chaters, managing the 
 * ON/OFFLINE status of all clients in the chat room.
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
 * Main function of the chatserver
 * Command line input argument should be:
 * ./<executable filename> <server_IP> <server_port> <max_num_client>
 */
int main(int argc, char** argv)
{
  fd_set fdslist;  // file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;  // max number of file descriptor
  int servfd;  // listening socket descriptor
  int newfd;  // newly accapted socket descriptor
  struct sockaddr_storage cltaddr;  // client address
  socklen_t addrlen;  // length of client address

  struct timeval tv;  // time structure
  tv.tv_sec = 10;
  tv.tv_usec = 10000000;

  // Define and initialize the namelist structure.
  struct user {
    char usernm[20];  // to store username string of corresponding sktfd
    int mapfd;  // to store sktfd
    struct user *next;  // pointer to the next user
  };
  struct user *root, *temp, *temp1, *temp2;
  root = malloc(sizeof(struct user));
  root->next = 0;
  temp = root; 

  // Define and initialize the structure of SBCP packets, including protocol header and message header
  struct header_sbcp {
    uint16_t vrsn : 9;  // SBCP header version
    uint16_t type : 7;  // SBCP header type
    uint16_t len;  // SBCP packet length
    struct header_attr {
      uint16_t type_attr;  // SBCP attribute type
      uint16_t len_attr;  // SBCP attribute length
      char payload[520];  // SBCP attribute payload
    } msgattr;
  } sbcpmsg, sbcpack_uc,sbcpack_un, sbcpfwd_un, sbcponline,sbcpoffline, sbcpidle, sbcpnak;

  int nbytes;  // buffer for client data
  int numClt = 0;  // the count of clients
  
  int flg=0;
  int i, j, k, l;
    
  
  struct addrinfo hints, *ai;
  FD_ZERO(&fdslist);  // clear the file descriptor list
  FD_ZERO(&read_fds);  // get us a socket and bind it

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // Determine whether the input arguments are right.
  if (argc != 4) {
    printf("Wrong input argument!\n");
    printf("Usage: ./chatserver server_ip server_port num_of_max_clients\n");
    exit(1);
  }

  // Get address information according to server_ip and server_port.
  if ((getaddrinfo((const char *) argv[1], (const char *) argv[2], &hints, &ai)) != 0) {
    printf("Fail to get address information from input arguments.\n");
    exit(2);
  }    
  servfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

  // Bind the server.
  if (bind(servfd, ai->ai_addr, ai->ai_addrlen) < 0) {
    printf("Server fails to bind!\n");
    exit(3);
  }
  // When done, free the address.
  freeaddrinfo(ai);

  // Listen.
  if (listen(servfd, 10) == -1) {
    perror("Error in listening connection.\n");
    exit(4);
  }

  // Add the server socket descriptor to the file descriptor list.
  FD_SET(servfd, &fdslist);
  // Keep track of the biggest file descriptor.
  fdmax = servfd;
    
  // Main loop to build a never-crash server.
  for(;;) {
    read_fds = fdslist;
    // Select.
    if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
      perror("Error in selecting client socket.\n");
      exit(5);
    }
    // Traversal the existing connections looking for data to handle.
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == servfd) {
          // New connection!
          addrlen = sizeof cltaddr;
          newfd = accept(servfd,(struct sockaddr *)&cltaddr,&addrlen);
          if (newfd == -1) {
            perror("Error in accepting client.\n");
          } else {
            FD_SET(newfd, &fdslist); // add to file descriptor list 
            // keep track of the max
            if (newfd > fdmax) {
              fdmax = newfd;
            }
                        
            /* To determine whether the number of clients
               has exceed the capacity */
            numClt = fdmax - 3;
            if (numClt > atoi((const char *) argv[3])) {
              printf("Error: the number of client exceed the maximum capacity.\n");
              exit(6);
            }
          }
        } else {
        // Handle data from a client.
          if ((nbytes = recv(i,(char *) &sbcpmsg, sizeof(sbcpmsg), 0)) <= 0) {
            if (nbytes == 0) {
              // connection closed by client
              // get username of client
              char username[20];
              for (l=0,temp1=root; temp1->next!=0; l++) {
                if (temp1->mapfd == i) {
                  strcpy(username, temp1->usernm);
                  break;
                } else {
                  temp1 = temp1->next;
                  continue;
                }
              }
              // define OFFLINE packet
              memcpy(sbcpoffline.msgattr.payload, (const char *) username, sizeof(sbcpoffline.msgattr.payload));
              sbcpoffline.msgattr.type_attr = 2;
              sbcpoffline.msgattr.len_attr = sizeof(int) + sizeof(sbcpoffline.msgattr.payload);
              sbcpoffline.vrsn = 3;
              sbcpoffline.type = 6;
              sbcpoffline.len = sizeof(int) + sbcpoffline.msgattr.len_attr;
              // broadcast OFFLINE to other clients
              for (j = 0; j <= fdmax; j++) {
                if (FD_ISSET(j, &fdslist)) {
                  // except the server itself and the client just left
                  if (j != servfd && j != i) {
                    if (send(j, (char *) &sbcpoffline, sizeof(sbcpoffline), 0) == -1) {
                      perror("Error in broadcasting the OFFLINE message.\n");
                    }
                  }
                }
              }
              // delete the username of the client just left
              for (l=0, temp1=root, temp2 = root; temp1->next != 0; l++) {
                if ( strcmp(username, temp1->usernm) == 0 && temp1 == root) {
                  // delete the struct
                  root = root->next;
                  break;
                } else if (strcmp(username, temp1->usernm) == 0 && temp1 != root) {
                  temp2->next = temp1->next;
                  break;
                } else {
                  temp2 = temp1;
                  temp1 = temp1->next;
                } 		
              }
              //if (temp1->next == 0) {
              //  printf("the user deletion did not occur.\n");
              //}
              // end of deletion...
              for (k = 0, temp1=root; temp1->next != 0; k++) {
                printf("%s  %d\n", temp1->usernm, temp1->mapfd);
                temp1 = temp1->next;
              }
                            
              printf("%s Exit the chat.\n", username);
            } else {
              // connection closed by client
              if(flg == 1) {
                printf("Client tried to access with an already existing user name. Access denied.\n");
              } 
            }
            close(i);  // close file descriptor i
            FD_CLR(i, &fdslist); // remove the client from the file descriptor list
          } else { 
          // Receive Packet successfully...
            if (sbcpmsg.type == 2) {  // handling the JOIN message
            // get a JOIN message from the client
              char username[20];
              memcpy(username, (const char *) sbcpmsg.msgattr.payload, sizeof(username));// user name of client i
              // dertermine whether there is an existing username
              for(l=0, temp1=root, temp2 = root; temp1->next != 0; l++) {
                if( strcmp(username, temp1->usernm) == 0) {
                  // define the NAK packet
                  char reason[200]="This user name has already been taken.\n";
                  memcpy(sbcpnak.msgattr.payload, (const char *) reason, sizeof(sbcpnak.msgattr.payload));
                  sbcpnak.msgattr.type_attr = 1;
                  sbcpnak.msgattr.len_attr = sizeof(int) + sizeof(sbcpnak.msgattr.payload);
                  sbcpnak.vrsn = 3;
                  sbcpnak.type = 5;
                  sbcpnak.len = sizeof(int) + sbcpnak.msgattr.len_attr;
                  // send NAK packet
                  if (FD_ISSET(i, &fdslist)) {
                    if (send(i, (char *) &sbcpnak, sizeof(sbcpnak), 0) == -1) {
                      perror("Error in forwarding the ACK_UN message.\n");
                    }
                  }
                  flg = 1;
                  break;
                } else {
                  temp2 = temp1;
                  temp1 = temp1->next;
                } 		
              }
              if(flg == 0) {
                // When gets here, it means it passes the username check.
                printf("the user deletion did not occur.\n");
                temp->next = malloc(sizeof(struct user));
                strcpy(temp->usernm, username);
                temp->mapfd = i;
                temp = temp->next;
                temp->next = 0;
                // Then print join the chat!
                printf("%s JOIN the chat!\n",username);
              }

              // define the ACK_UN message with the clients' name list (type_attr=2) ready to be send
              char nmlist[500] = "";
              char backslash[20] = "/";

              for (l =0, temp1=root; temp1->next != 0; l++) {
                strcat((char *) nmlist, (const char *) temp1->usernm);
                strcat((char *) nmlist, (const char *) backslash);
                temp1 = temp1->next;
              }
              memcpy(sbcpack_un.msgattr.payload, (const char *) nmlist, sizeof(sbcpack_un.msgattr.payload));
              sbcpack_un.msgattr.type_attr = 2;
              sbcpack_un.msgattr.len_attr = sizeof(int) + sizeof(sbcpack_un.msgattr.payload);
              sbcpack_un.vrsn = 3;
              sbcpack_un.type = 7;
              sbcpack_un.len = sizeof(int) + sbcpack_un.msgattr.len_attr;

              // define the ACK_UC message with only client count (type_attr=3) ready to be send
              char charnum[20];
              sprintf(charnum, "%d", numClt);
              memcpy(sbcpack_uc.msgattr.payload, (const char *) charnum, sizeof(sbcpack_uc.msgattr.payload));
              sbcpack_uc.msgattr.type_attr = 3;
              sbcpack_uc.msgattr.len_attr = sizeof(int) + sizeof(sbcpack_uc.msgattr.payload);
              sbcpack_uc.vrsn = 3;
              sbcpack_uc.type = 7;
              sbcpack_uc.len = sizeof(int) + sbcpack_uc.msgattr.len_attr;
  
              // send ACK to the client with file descriptor i
              if (FD_ISSET(i, &fdslist)) {
                if (send(i, (char *) &sbcpack_un, sizeof(sbcpack_un), 0) == -1) {
                  perror("Error in forwarding the ACK_UN message.\n");
                }
                if (send(i, (char *) &sbcpack_uc, sizeof(sbcpack_un), 0) == -1) {
                  perror("Error in forwarding the ACK_UC message.\n");                                 
                }
              }
              // define ONLINE packet with username of the client
              memcpy(sbcponline.msgattr.payload, (const char *) username, sizeof(sbcponline.msgattr.payload));
              sbcponline.msgattr.type_attr = 2;
              sbcponline.msgattr.len_attr = sizeof(int) + sizeof(sbcponline.msgattr.payload);
              sbcponline.vrsn = 3;
              sbcponline.type = 8;
              sbcponline.len = sizeof(int) + sbcponline.msgattr.len_attr;
              // send ONLINE to other clients
              for(j = 0; j <= fdmax; j++) {
                if (FD_ISSET(j, &fdslist)) {
                  // except the server itself and the client just arrived
                  if (j != servfd && j != i) {
                    if (send(j, (char *) &sbcponline, sizeof(sbcponline), 0) == -1) {
                      perror("Error in broadcasting the ONLINE message.\n");
                    }
                  }
                }
              }
            } else if (sbcpmsg.type == 3) { // handling SEND message from client
            // FWD_Username
            for (l=0,temp1=root; temp1->next!=0; l++) {
              if(temp1->mapfd == i) {
                strcpy(sbcpfwd_un.msgattr.payload, temp1->usernm);
                break;
              } else {
                temp1 = temp1->next;
                continue;
              }
            }
            sbcpfwd_un.msgattr.type_attr = 2;
            sbcpfwd_un.msgattr.len_attr = sizeof(int) + sizeof(sbcpfwd_un.msgattr.payload);
            sbcpfwd_un.vrsn = 3;
            sbcpfwd_un.type = 4;
            sbcpfwd_un.len = sizeof(int) + sbcpfwd_un.msgattr.len_attr;

            // FWD_Message
            sbcpmsg.msgattr.type_attr = 4;
            sbcpmsg.msgattr.len_attr = sizeof(int) + sizeof(sbcpmsg.msgattr.payload);
            sbcpmsg.vrsn = 3;
            sbcpmsg.type = 4;
            sbcpmsg.len = sizeof(int) + sbcpmsg.msgattr.len_attr;                            
            // we got some data from a client
            for(j = 0; j <= fdmax; j++) {
              // broadcast to other clients
              if (FD_ISSET(j, &fdslist)) {
                // except the server itself and the client whom sends the message
                if (j != servfd && j != i) {
                  // FWD_username
                  if (send(j, (char *) &sbcpfwd_un, sizeof(sbcpfwd_un), 0) == -1) {
                    perror("Error in broadcasting the FWD_username.\n");
                  }
                  // FWD_message
                  if (send(j, (char *) &sbcpmsg, sizeof(sbcpmsg), 0) == -1) {
                    perror("Error in broadcasting the FWD message.\n");
                  }
                }
              }
            }
          } else if (sbcpmsg.type == 9) {
            // define IDLE packet
            // get username of i from the name list
            char name[20];
            for (l=0,temp1=root; temp1->next!=0; l++) {
              if(temp1->mapfd == i) {
                strcpy(name, temp1->usernm);
                break;
              } else {
                temp1 = temp1->next;
                continue;
              }
            }
            memcpy(sbcpidle.msgattr.payload, (const char *) name, sizeof(sbcpidle.msgattr.payload));
            sbcpidle.msgattr.type_attr = 2;
            sbcpidle.msgattr.len_attr = sizeof(int) + sizeof(sbcpidle.msgattr.payload);
            sbcpidle.vrsn = 3;
            sbcpidle.type = 9;
            sbcpidle.len = sizeof(int) + sbcpidle.msgattr.len_attr;
              // broadcast IDLE message to other clients
              for(j = 0; j <= fdmax; j++) {
                if (FD_ISSET(j, &fdslist)) {
                  // except the server itself and the idle client
                  if (j != servfd && j != i) {
                    // IDLE message
                    if (send(j, (char *) &sbcpidle, sizeof(sbcpidle), 0) == -1) {
                      perror("Error in broadcasting the IDLE message.\n");
                    }
                  }
                }
              }
            }
          }
        } // END of handling data exchange with clients
      } // END of new incoming connection handling
    } // END of file descriptors looping
  } // END of for(;;)standing-always server
return 0;
}

