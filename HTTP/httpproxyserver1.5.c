/*
 * Name: httpproxyserver.c
 * Descriptions: a multi-client http proxy server using HTTP 1.0 protocol specifications. This server program keeps listening to 
 * incoming HTTP requests.
 * Author: Bolun Huang, Sriharsha Madala.         
 * Date: 10/22/2012
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
#include <time.h>

#define PORT 80
#define TIMDIF 18000
#define MAX 5
#define USERAGENT "HTTPTool/1.0"

// creating a linked list of webpages to store based on MRU
struct page {
  char pagename[50];
  char *expiretime;
  char *expiretime_week;
  struct page *next;
  };
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
  int servfd, newservfd;  // listening socket descriptor
  int newfd;  // newly accapted socket descriptor
  struct sockaddr_storage cltaddr;  // client address
  struct sockaddr_in addr;  // socket address of intended HTTP server
  struct hostent* hret;

  socklen_t addrlen;  // length of client address
  char buff[BUFSIZ+1];  // message buffer
  char buff2[BUFSIZ+1];  // message buffer2 (temp)
  char buffr[BUFSIZ+1];  // received message buffer

  // the structure of page information
  struct page *root, *temp1, *temp2, *temp3, *temp4, *prevtemp, *prevtemp2, *temp5;
  root = malloc(sizeof(struct page));
  root->next = 0;
  temp1 = root;
  // conditional GET query prototype
  char *query;
  char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nFrom: huangbolun1989@gmail.com\r\nUser-Agent: %s\r\nIf-Modified-Since: %s\r\n\r\n";
  char *code;  // response code. can be 200/400/etc.

  int nbytes;  // buffer for client data
  int check;  // length of buffer from server
  int i;  
  int j, a;
  int pagecount = 0; 
  int flag3=0;  // for page counting cache  
  struct addrinfo hints, *ai;
  FD_ZERO(&fdslist);  // clear the file descriptor list
  FD_ZERO(&read_fds);  // get us a socket and bind it

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // Determine whether the input arguments are right.
  if (argc != 3) {
    printf("USAGE: ./<executable file name> <server_ip_to_bind> <server_port_to_bind>\n");
    exit(0);
  }

  // Get address information according to server_ip and server_port.
  if ((getaddrinfo((const char *) argv[1], (const char *) argv[2], &hints, &ai)) != 0) {
    printf("Fail to get address information from input arguments.\n");
    exit(1);
  }
  servfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

  // Bind the server.
  if (bind(servfd, ai->ai_addr, ai->ai_addrlen) < 0) {
    printf("Server fails to bind!\n");
    exit(2);
  }
  // When done, free the address.
  freeaddrinfo(ai);

  // Listen.
  if (listen(servfd, 10) == -1) {
    perror("Error in listening connection.\n");
    exit(3);
  }

  // Add the server socket descriptor to the file descriptor list.
  FD_SET(servfd, &fdslist);
  // Keep track of the biggest file descriptor.
  fdmax = servfd;
    
  // Main loop to build a never-crash server.
  for(;;) {
    // listening for income request...
    //printf("Waiting for incoming HTTP request...\n");
    read_fds = fdslist;
    // Select.
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("Error in selecting client socket.\n");
    }
    // Traversal the existing connections looking for data to handle.
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == servfd) {
          // New connection!
          addrlen = sizeof cltaddr;
          // accept new client!
          newfd = accept(servfd,(struct sockaddr *)&cltaddr,&addrlen);
          if (newfd == -1) {
            perror("Error in accepting client.\n");
          } else {
            FD_SET(newfd, &fdslist); // add to file descriptor list
            read_fds = fdslist;
            // keep track of the max
            if (newfd > fdmax) {
              fdmax = newfd;
            }
          }
        } else {
          // Handle data from a client.
          if ((nbytes = recv(i,buff, BUFSIZ, 0)) <= 0) {
            if (nbytes == 0) {
              // connection closed by client
              printf("connection close by the client.\n");
            } else {
              // receive data failed
              printf("Error in receiving data - (1)\n");
            }
            close(i);  // close file descriptor i
            FD_CLR(i, &fdslist); // remove the client from the file descriptor list
          } else {  // receive succeed
            memcpy(buff2, buff, BUFSIZ);  // copy the buff
            printf("%s",buff2);
            // Receive Packet successfully...
            char *type1;  // message type1 -- GET
            type1 = malloc(3);
            char *get = "GET";
            memcpy(type1, (const char *)buff2, 3);
            memcpy(type1+3, "\0", 1);		//changed1
            printf("type1: %s\n", type1);
            if (strcmp((const char *)type1, (const char *)get) == 0) {
              printf("It is a GET request.\n");
              // decoding the GET request...
              char *spl = "\r\n";  // split deliminator
              char *spl2 = " ";  // space deliminator
              char *spl3 = "/";  // backslash deliminator
              char *spl5 = "_";  // backslash deliminator
              char *p, *p2;  // token pointer
              char *getinfo;
              char *hostinfo, *hostinfo2;
              
              p = strtok(buff2, spl);
              getinfo = p;  // get information
              printf("getinfo: %s\n",getinfo);
              p = strtok(NULL, spl);
              hostinfo = p+6;  // host information
              printf("hostinfo: %s\n",hostinfo);
              p2 = strtok(getinfo,spl2);
              p2 = strtok(NULL,spl2);  // page information
              printf("pageinfo: %s\n",p2);
              int len = strlen(p2);
              int m;
              for (m = 0; m < len; m++) {
                if ((strcmp(p2+m,spl3))==0) {  // find '/'
                  strcpy(p2+m, spl5);  // replace '/' by '_'
                }
              }
              printf("pageinfo2: %s\n",p2);
              //check if u need to forward the get
              /* Expiration time management */
              int flag1=0;  // whether there is a webpage already stored
              int flag2=0;  // whether the page is expired

              hostinfo2= (char *)malloc(200*sizeof(char));
              strncpy(hostinfo2,(const char *)hostinfo,strlen(hostinfo));
              strcat(hostinfo2,(const char *)spl5);
              strcat(hostinfo2,(const char *)p2+1);
              strcat(hostinfo2,".html");  // cache file name
              printf("filename: %s\n",hostinfo2);
              memset(buffr, 0, sizeof(buffr));
              int htmlstart = 0;
              char *htmlcontent, *expmark, *expmark_week;
              char *spl4 = "\n";
              char *p3, *p4;
              p4 = (char *)malloc(50*sizeof(char));
              FILE *fd;
              FILE *fpo;
              FILE *fpo2;
              //printf("here1\n");
              struct tm tm1,*tm3;
              time_t t2,t3;
             
              for(temp3=root; temp3->next != 0; temp3 = temp3->next) {
                //printf("entered the for\n");
                if( strcmp(hostinfo2, temp3->pagename) == 0 ) {  // existing in cache, compare with expiration field with current time
                  flag1 = 1;
                  printf("%s\n",temp3->expiretime);
                  if (strptime(temp3->expiretime, "%d %b %Y %H:%M:%S", &tm1) == NULL) {
                    printf("couldnt parse\n");
                  }

                  tm1.tm_isdst = 0; // Not set by strptime(); tells mktime() to determine whether daylight saving time is in effect
                  t2 = mktime(&tm1);  // the expiration time
                  if (t2 == -1) {
                    printf("couldnt mktime\n");
                  }
                  printf("expiry time t2: %ld\n", (long) t2);  // expire time in epoch
                  time(&t3);  // current time
                  tm3 = gmtime(&t3);
                  //printf("GMT current time: %s\n",asctime(tm3));
                  t3 = mktime(tm3);
                  printf("current time t3: %ld\n", (long) t3);  // current time in epoch
                  if(t3 >= t2) {  // expired, delete the existing one, then send GET
                    flag2 = 1;
                    printf("You can not use the page\n");
                    break;
                  } else {  // not expired, open the existing file and send it to client
                    flag2 = 0;
                    printf("You can use the page\n");
                    // print the linked list
                    for (j =0, temp2=root; temp2->next != 0; j++) {
                      printf("%s\n", temp2->pagename);
                      printf("%s\n", temp2->expiretime);
                      //printf("%s\n",temp2->lastaccessed);
                      temp2 = temp2->next;
                    }
                    printf("page count: %d\n", pagecount);
                    /* just open and send the html file to the client! */
                    fpo = fopen(hostinfo2, "r");
                    int data = 0;
                    while( !feof(fpo) && !ferror(fpo)) {
                      unsigned char msg[BUFSIZ] = "";
                      //j++;
                      for( a=0; a<=BUFSIZ-1; a++) {
                        if (( data = fgetc(fpo) ) == EOF) {
                          break;
                        } else {
                          msg[a] =(unsigned char) data;
                        }
                      }
                      /*send the datas to the client*/
                      if ((send(i, (const char *) msg, BUFSIZ, 0)) == -1) {
                        perror("Error in relaying HTTP response to client - (1).\n");
                      }
                    }
                    memset(hostinfo2, 0, strlen(hostinfo));
                    fclose(fpo);
                  }
                }
                prevtemp2 = temp3;
              }
              // flag management: when did not exist in cache or cache expired, then send GET; when expired, deletion before sending.
              if (flag2 == 1 || flag1 == 0) {
                // delete the existing webpage that expired
                if (flag2 == 1) {
                  // deletion
                  for(j=0, temp4=root; temp4->next != 0; j++) {
                    if( strcmp(hostinfo2, temp4->pagename) == 0 && temp4 == root) {
                      // delete the struct
                      root = root->next;
                      break;
                    } else if (strcmp(hostinfo2, temp4->pagename) == 0 && temp4 != root) {
                      prevtemp->next = temp4->next;
                      break;
                    } else{
                      prevtemp = temp4;
                      temp4 = temp4->next;
                    }     
                  }
                  pagecount--;  // page count down
                }
                
                /*send GET*/
                // create a new server socket
                if ((newservfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                  perror("Cannot create new server socket.\n");
                  exit(5);
                }
                // connected to HTTP server and forwarding the GET request
                addr.sin_family = AF_INET;
                addr.sin_port = htons(PORT);
                hret = gethostbyname(hostinfo);  // proxy_server_ip
                memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);
                // connect to actual server
                if(connect(newservfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1) {
                  perror("Cannot connect to HTTP Server\n");
                  exit(7);
                } else {
                  printf("connect to HTTP server successfully.\n");
                }
                // handle the Conditional GET
                if (flag2 == 1) {
                  printf("entered\n");
                  printf("expiretime_week: %s\n",temp3->expiretime_week);
                  printf("%d\n",strlen(p2+1)+strlen(hostinfo)+strlen(USERAGENT)+strlen(temp3->expiretime_week)+strlen(tpl)-6);
                  query = (char *)malloc(strlen(p2+1)+strlen(hostinfo)+strlen(USERAGENT)+strlen(temp3->expiretime_week)+strlen(tpl)-6);
                  sprintf(query, "GET /%s HTTP/1.0\r\nHost: %s\r\nFrom: huangbolun1989@gmail.com\r\nUser-Agent: %s\r\nIf-Modified-Since: %s\r\n\r\n", p2+1, hostinfo, USERAGENT, temp3->expiretime_week);
                  memcpy(buff, (const char *)query, strlen(query));
                }

                // forwarding the GET message
                if (send(newservfd, (char *) buff, BUFSIZ, 0) == -1) {
                  perror("Error in forwarding the GET message.\n");
                } else {
                  printf("Fowarding GET successfully.\n");
                  printf("%s", buff);
                }

                fd = fopen(hostinfo2,"w");
                expmark_week = (char *)malloc(50*sizeof(char));
                // receiving the response message from actually server
                while ((check = recv(newservfd, buffr, BUFSIZ, 0)) > 0) {
                  char *buffc;
                  buffc = malloc(strlen(buffr));
                  memcpy(buffc, buffr, strlen(buffr));
                  //printf("inside while\n");
                  if(htmlstart == 0) {  // the first packet of HTTP response
                    char *buffd = (char *)malloc(strlen(buffr));  // header information
                    memcpy(buffd, buffr, strlen(buffr));
                    char *p22 = strtok(buffd, "\r\n");
                    printf("%s\n\n",p22);  // HTTP response
                    code = strtok(p22, " ");  // capture the response code
                    code = strtok(NULL, " ");  // this is the response number
                    printf("%s\n",code);
                    if( (p3 = strstr(buffc, "Expires:")) != NULL)
                      {
                      p3 = p3 + 9;
                      memcpy(p4, p3, 50);  
                      expmark = strtok(p4, spl4);
                      memcpy(expmark_week, expmark, strlen(expmark));
                      expmark = expmark + 5;
                      //printf("The requested page expires in: %s\n", expmark);
                      }
                    else
                      {
                      expmark = (char *) malloc(50*sizeof(char));
                      printf("Did not find the 'Expires' field\n");
                      strcpy(expmark,"18 Oct 2010 12:40:32 GMT");		// hence always gives out expired
                      strcpy(expmark_week,"Mon, 18 Oct 2010 12:40:32 GMT");		// hence always gives out expired
                      //printf("expmark: %s\n", expmark);
                      }
               
                    htmlcontent = strstr(buffr, "\r\n\r\n");
                    //p2 = strtok(buffr, "\r\n\r\n");
                    //printf("%s\n", p2);
                    if(htmlcontent != NULL) {
                      htmlstart = 1;
                      htmlcontent += 4;
                    }
                  } else {
                    htmlcontent = buffr;
                  }
                  if(htmlstart) {
                    // relay the content to the client
                    fprintf(fd,htmlcontent);
                    if ((send(i, htmlcontent, strlen(htmlcontent), 0)) == -1) {
                      perror("Error in relaying HTTP response to client.\n");
                    }
                  }
                  memset(buffr, 0, check);
                }
                fclose(fd);
                // handle the 304 case: just open the file and send it
                if ((strcmp(code, "304"))==0) {
                    fpo2 = fopen(hostinfo2, "r");
                    int data1 = 0;
                    while( !feof(fpo2) && !ferror(fpo2)) {
                      unsigned char msg1[BUFSIZ] = "";
                      //j++;
                      for( a=0; a<=BUFSIZ-1; a++) {
                        if (( data1 = fgetc(fpo2) ) == EOF) {
                          break;
                        } else {
                          msg1[a] =(unsigned char) data1;
                        }
                      }
                      /*send the datas to the client*/
                      if ((send(i, (const char *) msg1, BUFSIZ, 0)) == -1) {
                        perror("Error in relaying HTTP response to client - (1).\n");
                      }
                    }
                    fclose(fpo2);
                }
                // once the page is written it has to be added to the linked list.
                if (flag3 == 0 && (strcmp(code, "200")==0)) {
                  strcpy(temp1->pagename,hostinfo2);
                  // update the expiretime of the structure.
                  temp1->expiretime = (char *)malloc(50*sizeof(char));
                  strcpy(temp1->expiretime, expmark);
                  temp1->expiretime_week = (char *)malloc(50*sizeof(char));
                  strcpy(temp1->expiretime_week,expmark_week);
                  temp1->next = malloc(sizeof(struct page));  
                  temp1 = temp1->next;
                  temp1->next = 0;
                  pagecount++;  // page count up
                  if (pagecount >= MAX && flag1 == 0) {
                    flag3=1;
                  }
                  //printf("flag3:%d\n",flag3);
                }
                if (flag3 == 1) {
                    temp5=root->next;
                    root=temp5;
                    pagecount--;  // page count down
                    flag3=0;
                }
              //printing the list.
              for (j =0, temp2=root; temp2->next != 0; j++) {
                printf("%s\n", temp2->pagename);
                printf("%s\n", temp2->expiretime);
                temp2 = temp2->next;                
              }
              printf("page count: %d\n", pagecount);
              if(check <= 0) {
                printf("Forwarding requested message complete!\n");
              }
            }
            memset(hostinfo, 0, strlen(hostinfo));
            memset(hostinfo2, 0, strlen(hostinfo));
            memset(buff, 0, strlen(buff));
            }
            else {
              printf("GET request FAILED.\n");
            }
          }
          close(i);
          FD_CLR(i, &fdslist);
        } // END of handling data exchange with clients
      } // END of new incoming connection handling
    } // END of file descriptors looping
    close(newservfd);  // close the new server socket
    //free(type1);
   // free(p4);
   // free(buffc);
  } // END of for(;;)standing-always server
close(servfd);  // close the server socket
return 0;
}

