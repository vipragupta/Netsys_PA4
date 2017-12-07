#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
   
void error(char* msg)
{
  perror(msg);
  exit(0);
}
  
int main(int argc, char* argv[])
{
  pid_t pid;
  struct sockaddr_in addr_in, cli_addr, proxy_addr;
  struct hostent* host;
  int sockfd, newsockfd;
     
  if(argc < 2) {
    perror("./proxy <port_no> <timestamp>");
    exit(0);
  }
    
  printf("\n______________________________Proxy Server Started_____________________________\n");
     
  bzero((char*)&proxy_addr, sizeof(proxy_addr));
  bzero((char*)&cli_addr,  sizeof(cli_addr));
     
  proxy_addr.sin_family = AF_INET;
  proxy_addr.sin_port = htons(atoi(argv[1]));
  proxy_addr.sin_addr.s_addr = INADDR_ANY;
     
    
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sockfd < 0) {
    perror("Problem in initializing socket");
    exit(0);
  }
     
  if(bind(sockfd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr))<0) {
    perror("Error on binding");
    exit(0);
  }
    
    
  listen(sockfd, 50);
  int clientLength=sizeof(cli_addr);

  while(1) {
   
    //unsigned int microseconds = 100000;
    //usleep(microseconds);
    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clientLength);
       
    if(newsockfd < 0) {
      perror("Problem in accepting connection");
      exit(0);
    }
      
    pid = fork();
    if(pid == 0)
    {
      printf("******************************************************************************************\n");

      struct sockaddr_in server_addr;
      int flag = 0, recvsocketId, n,port = 0, i,socketId;
      char buffer[510], httpMethod[300], hostStr[300], httpVersion[10];
      char* temp = NULL;
      bzero((char*)buffer, 500);
      recv(newsockfd, buffer, 500, 0);
         
      sscanf(buffer, "%s %s %s", httpMethod, hostStr, httpVersion);
         
         
      if(((strncmp(httpMethod, "GET", 3) == 0))&&((strncmp(httpVersion, "HTTP/1.1", 8) == 0)||(strncmp(httpVersion, "HTTP/1.0", 8) == 0))&&(strncmp(hostStr, "http://", 7) == 0))
      {
        printf("Request:\n%s %s %s\n\n", httpMethod, hostStr, httpVersion);
        strcpy(httpMethod, hostStr);
           
        flag = 0;
           
        for(i = 7; i < strlen(hostStr); i++)
        {
          if(hostStr[i] == ':')
          {
            flag = 1;
            break;
          } else if (hostStr[i] == '?') {
            break;
          }
        }
           
        temp = strtok(hostStr, "//");
        if(flag == 0)
        {
          port = 80;
          temp = strtok(NULL, "/");
        }
        else
        {
          temp = strtok(NULL, ":");
        }
           
        sprintf(hostStr, "%s", temp);
        //printf("host: %s\n", hostStr);
        host = gethostbyname(hostStr);
           
        if(flag == 1)
        {
          temp = strtok(NULL, "/");
          port = atoi(temp);
        }
           
           
        strcat(httpMethod, "^]");
        temp = strtok(httpMethod, "//");
        temp = strtok(NULL, "/");
        if(temp != NULL) {
          temp = strtok(NULL, "^]");
        }
        //printf("path: %s\n",  temp);
        //printf("port: %d\n",  port);
           
           
        bzero((char*)&server_addr, sizeof(server_addr));
        server_addr.sin_port=htons(port);
        server_addr.sin_family=AF_INET;
        bcopy((char*)host->h_addr, (char*)&server_addr.sin_addr.s_addr, host->h_length);
           
        socketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        recvsocketId = connect(socketId, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
        
        if (recvsocketId < 0) {
          perror("Error in connecting to remote server");
          exit(0);
        }
           
        //printf("HostName: %s  IP: %s\n", hostStr, inet_ntoa(server_addr.sin_addr));

        bzero((char*)buffer, sizeof(buffer));
        
        if(temp != NULL) {
          sprintf(buffer, "GET http://%s/%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", hostStr, temp, httpVersion, hostStr);
        }
        else {
          sprintf(buffer, "GET http://%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", hostStr, httpVersion, hostStr);
        }
         
         
        n = send(socketId, buffer, strlen(buffer), 0);
        printf("\n%s\n", buffer);
        if(n < 0) {
          perror("Error writing to socket");
          exit(0);
        }
        else{
          do
          {
            bzero((char*)buffer, 500);
            n = recv(socketId, buffer, 500, 0);
            if(n > 0)
            send(newsockfd, buffer, n,0);
          } while(n>0);
        }
      }
      else
      {
        send(newsockfd, "400 : BAD REQUEST\n", 18, 0);
      }
      close(socketId);
      close(newsockfd);
      close(sockfd);
      exit(0);
    }
    else
    {
      close(newsockfd);
    }
  }
  return 0;
}