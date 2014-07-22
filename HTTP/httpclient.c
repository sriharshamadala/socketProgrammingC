/*
 * Name: httpclient.c
 * Descriptions: This client program can request an HTTP web page by using the 
 * hostname. The only function of this client is to generate a GET request to the Proxy Server.
 * Author: Bolun Huang, Sriharsha Madala.         
 * Date: 10/07/2012
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

#define PORT 80
#define USERAGENT "HTTPTool/1.0"

/*
 * The main function of httpclient
 * The command line argument should be: ./<executable file name> <proxy_server_ip> <proxy_server_port>
 */
int main(int argc, char** argv) {
  int csd, check;  // file descriptor of client and return value
  struct sockaddr_in addr;  // socket address of client
  struct hostent* hret;
  char buff[BUFSIZ+1];  // message bufffer
  char username[20];  // username of client
  memcpy(username, (const char *) argv[1],sizeof(username));
  char *host, *hosts, *host1, *host2;  // store the host information
  hosts = (char *)malloc(200*sizeof(char));
  host2 = (char *)malloc(200*sizeof(char));
  char *http = "http://";
  char *cp;  // comparison string of http
  cp = malloc(7);
  char *page;  // the specific page
  char *pagecontent;  // content of page
  pagecontent = (char *)malloc(200*sizeof(char));
  short port = 0;  // server port number
  FILE *fd;  // html file

  // check the input arguments
  if (argc != 4) {
    perror("USAGE: ./<executable file name> <proxy_server_ip> <proxy_server_port> <URL to retrieve>\n");
    //exit(0);
  }
  // deal with the URL
  memcpy(cp,argv[3],7);
  printf("cp: %s\n",cp);
  if (strcmp(cp, http)==0) {
    host = argv[3]+7;
    memcpy(hosts, host, strlen(host));
  } else {
    host = argv[3];  // website to retrieve
    memcpy(hosts, host, strlen(host));
  }
  printf("host: %s\n", host);
  printf("hosts: %s\n", hosts);


  if ((page = strstr(host, "/"))==NULL) {  // we dont have sub pages
    memcpy(host2, host, strlen(host));
  } else {  // we have sub pages
    host1 = strtok(hosts, "/");
    memcpy(host2, host1, strlen(host1));
    //page = strstr(host, "/");
    memcpy(pagecontent, page+1, 100);
  }
  printf("page: %s\n", page);
  printf("host1: %s\n", host2);
  
  // create socket
  if ((csd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Cannot create client socket.\n");
    exit(1);
  }
  port = atoi(argv[2]);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  hret = gethostbyname(argv[1]);  // proxy_server_ip
  memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);
  // connect to server
  if(connect(csd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1) {
    perror("Cannot connect to Server\n");
    exit(2);
  } else {
    printf("connect to Proxy server successfully.\n");
  }

  // request for the URL
  char *query;
  char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nFrom: huangbolun1989@gmail.com\r\nUser-Agent: %s\r\n\r\n";
 
  query = (char *) malloc(strlen(pagecontent)+strlen(host2)+strlen(USERAGENT)+strlen(tpl)-5);
  sprintf(query, "GET /%s HTTP/1.0\r\nHost: %s\r\nFrom: huangbolun1989@gmail.com\r\nUser-Agent: %s\r\n\r\n", pagecontent, host2, USERAGENT);
  // send the GET
  if((send(csd, query, strlen(query), 0)) == -1){
    perror("Can't send query");
    exit(1);
  } else {
    printf("Send successfully.\n");
  }
  // receive and display the content
  memset(buff, 0, sizeof(buff));
  char *   content;
  fd = fopen("test.html","w");
  // receiving HTML data
  while((check = recv(csd, buff, BUFSIZ, 0)) > 0) {
      content = buff;
    fprintf(fd,   content);
    memset(buff, 0, check);
  }  // end of receiving html data
  
  if(check < 0) {
    perror("Error in receiving data\n");
    exit(0);
  } else {
    printf("Retrieved complete!\n");
  }
  fclose(fd);
  close(csd);
  return 0;
}
