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
#include <fcntl.h>

#define ipHostCacheFile "htimhtim"
   
void error(char* msg)
{
  perror(msg);
  exit(0);
}

int getHostIP(char* hostMap[], char* host, int maxIndex) {
  printf("\n");
  printf("host: %s\n", host);
  
  for (int i = 0; i < maxIndex; i++) {
    if (strcmp(hostMap[i], host) == 0) {
      printf("Returning: %d\n\n", i);
      return i;
    }
  }
  printf("Returning: -1\n\n");
  return -1;
}
  
int writeFile(char *fileName, char *data, int size, int append) {
  FILE *file;
  char fileNameW[50];
  bzero(fileNameW, sizeof(fileNameW));

  strcpy(fileNameW, fileName);

  if (append == 0) {
    file = fopen(fileNameW,"wb");
  } else {
    file = fopen(fileNameW,"ab");
  }

  int fileSize = fwrite(data , sizeof(unsigned char), size, file);

  if(fileSize < 0)
    {
      printf("Error writting file\n");
        exit(1);
    }
    fclose(file);
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

      FILE *file;
      char data[1024*1024];
      char* hostMap[50];
      char* ipMap[50];
  
      for (int i=0; i < 50; i++) {
        hostMap[i] = calloc(50, sizeof(char));
        ipMap[i] = calloc(50, sizeof(char));
      }   

      file = fopen(ipHostCacheFile, "r");
      int index = 0;

      if (file) {
        while (fgets(data, sizeof(data), file)) {
          char *tok = strtok(data, "\n");
          char *tokkk = strtok(tok, " ");

          if (tokkk != NULL) {
            strcpy(hostMap[index],tokkk);
            char *temp = strtok(NULL, " ");
            strcpy(ipMap[index],temp);
            
            index++;
          }
        }
      }
      fclose(file);

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
        printf("host: %s\n", hostStr);

        int pass = getHostIP(hostMap, hostStr, index);

        for (int i = 0; i < index; i++) {
          printf("i:%d     host:%s     ip:%s\n", i, hostMap[i], ipMap[i]);
        }

        if (pass == -1) {

          host = gethostbyname(hostStr);
          
          strcpy(hostMap[index], hostStr);
          bcopy( (char*)host->h_addr, (char*)ipMap[index], host->h_length);
          
          
          pass = index;
          index++;

          char ipHostCacheData[200];
          bzero(ipHostCacheData, sizeof(ipHostCacheData));
          strcpy(ipHostCacheData, hostMap[index-1]);
          strcat(ipHostCacheData, " ");
          strcat(ipHostCacheData, ipMap[index-1]);
          strcat(ipHostCacheData, "\n");
          ipHostCacheData[strlen(ipHostCacheData)] = '\0';
          printf("str: %s   host: %s     ip: %s    index: %d\n", hostStr,  hostMap[pass], ipMap[pass], pass);
          writeFile(ipHostCacheFile, ipHostCacheData, strlen(ipHostCacheData), 1);
        } else {
          printf("FOUNDDDDDDDDDD\n");
          printf("str: %s   host: %s     ip: %s    index: %d\n", hostStr,  hostMap[pass], ipMap[pass], pass);
        }
           
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
        printf("path: %s\n",  temp);
        printf("port: %d\n",  port);
           
           
        bzero((char*)&server_addr, sizeof(server_addr));
        server_addr.sin_port=htons(port);
        server_addr.sin_family=AF_INET;
        strcpy((char*)&server_addr.sin_addr.s_addr, (char*)ipMap[pass]);

        //bcopy((char*)host->h_addr, (char*)&server_addr.sin_addr.s_addr, host->h_length);
           
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
        
        printf("\n\n%s\n", buffer);
         
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