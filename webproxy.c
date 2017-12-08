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
#include <openssl/md5.h>
#include <time.h>

#define ipHostCacheFile "htimhtim"
#define forbiddenIPAddressFile "forbiddenIp.txt"
#define cacheFile "cacheFileData.txt"
   
void error(char* msg)
{
  perror(msg);
  exit(0);
}


int checkIfBlockedIpHost(char* blockedList[], char* host, char* ip, int maxIndex) {
  
  for (int i = 0; i < maxIndex; i++) {
    if (strcmp(blockedList[i], host) == 0 || strcmp(blockedList[i], ip) == 0 ) {
      printf("Blocked IP/Host.\n\n");
      return 1;
    }
  }

  return -1;
}


void getMd5Sum(char *hostStr, char* c_new) {

  unsigned char c[64];
  bzero(c, sizeof(c));
  //unsigned char c_new[32];
  bzero(c_new, sizeof(c_new));

  bzero(c, sizeof(c));
  MD5_CTX mdContext;
  MD5_Init (&mdContext);

  MD5_Update (&mdContext, hostStr, strlen(hostStr));
  MD5_Final (c, &mdContext);
  
  for(int i = 0; i < strlen(c); i++) {
    char temp[3];
    sprintf(temp, "%0x", c[i]);
    //printf("%s", temp);
    strcat(c_new, temp);
  }

  printf("md5Sum:%s\n", c_new);
}


int getHostIP(char* hostMap[], char* host, int maxIndex) {
  
  for (int i = 0; i < maxIndex; i++) {
    if (strcmp(hostMap[i], host) == 0) {
      printf("Found IP in cache.\n\n");
      return i;
    }
  }
  printf("Couldn't find IP in cache.\n\n");
  return -1;
}

  
void writeFile(char *fileName, char *data, int size, int append) {
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


int isFileInCache(char *pathHash, char *allowedDifference, char *yes) {

  printf(".................................\nCHECK FILE IN CACHE!!!!\nhash:%s    allowedDifference:%s\n\n", pathHash, allowedDifference);
  FILE *file;
  char line[1024]; 
  time_t now;
  now = time(NULL);
  yes[0] = '0';
  file = fopen(cacheFile, "r");
  int index = 0;

  if (file) {
    while (fgets(line, sizeof(line), file)) {
      char *lineTemp = strtok(line, "\n");
      char *hash = strtok(lineTemp, " ");

      if (hash != NULL) {
        char *timestamp = strtok(NULL, " ");
        
        if (timestamp != NULL) {
          printf("hash:%s    timestamp:%s\n", hash, timestamp);
          
          if (strcmp(hash, pathHash) == 0) {
            printf("\nEqual\n");
            long int then;
            long int allowedDifferenceInt;
            

            printf("now:%ld\n", now);
            
            then = atol(timestamp);
            //sprintf(then, "%s", timestamp);
            
            printf("then:%ld\n", then);
            

            long int difference = difftime(now, then);
            printf("difference:%ld\n", difference);

            allowedDifferenceInt = atol(allowedDifference);
            //sprintf(allowedDifference, "%ld", allowedDifferenceInt);
            printf("allowedDifferenceInt: %ld\n", allowedDifferenceInt);
            //sscanf(allowedDifference, "%ld", allowedDifferenceInt);

            if (difference <= allowedDifferenceInt) {
              printf("Returning: %d\n\n", index);
              yes[0] = '1';
            }
            return index;
          }
        }
      }
      index++;
    }
    fclose(file);
  }
  
  return -1;
}

void updateCacheTimestamp(char *pathHash, int lineNum) {

  printf("\n..........................................\nUpDATE CACHE!!!!\nhash:%s    lineNum:%d\n", pathHash, lineNum);
  char line[1024]; 
  time_t now;
  now = time(NULL);
  
  char timeStr[20];
  char updateData[200];
  bzero(updateData, sizeof(updateData));
  bzero(timeStr, sizeof(timeStr));
  
  sprintf(timeStr, "%ld", now);

  strcpy(updateData, pathHash);
  strcat(updateData, " ");
  strcat(updateData, timeStr);
  strcat(updateData, "\n");

  printf("updateData:%s\n", updateData);
  
  FILE *file;
  file = fopen(cacheFile, "a+");
  int index = 0;
  int flag = 0;
  if (file) {
    if (lineNum != -1) {
      while (fgets(line, sizeof(line), file)) {
      
        if (index == lineNum) {
          int temp = -1 * strlen(line);

          fseek(file, temp, SEEK_CUR);
          fwrite(updateData , sizeof(unsigned char), strlen(updateData), file);
          temp = temp * -1;
          fseek(file, temp, SEEK_CUR);
          printf("...UPDATED...\n");
          flag = 1;
          break;
        }
        index++;
      }
    }
    if (flag == 0) {
      fwrite(updateData , sizeof(unsigned char), strlen(updateData), file);
      printf("...UPDATED...\n");
    }
    fclose(file);
  }
}


int loadIpHostCache(char* hostMap[], char* ipMap[]) {

  FILE *file;
  char data[1024*1024];
  char* dataTemp[50];

  for (int i=0; i < 50; i++) {
    dataTemp[i] = calloc(50, sizeof(char));
  }   

  file = fopen(ipHostCacheFile, "r");
  int index = 0;

  if (file) {
    fgets(data, sizeof(data), file);
    fclose(file);

    char *tokkk = strtok(data, "#");
    int lineNum = 0;

    while (tokkk != NULL) {
      strcpy(dataTemp[lineNum], tokkk);
      tokkk = strtok(NULL, "#");
      lineNum++;
    }
   
    for (int i =0; i < lineNum; i++)
    {
      char *temp = strtok(dataTemp[i], " ");
      if(temp != NULL)
      {
        strcpy(hostMap[index], temp);
        temp = strtok(NULL, " ");
        if(temp != NULL)
        {
          strcpy(ipMap[index],temp);
          index++;
        }
          
      }
    }
  }
  return index;
}

int loadBlockedHostsIP(char* blockedHostIp[]) {

  FILE *file;
  char data[1024*1024]; 

  file = fopen(forbiddenIPAddressFile, "r");
  int lineNum = 0;

  if (file) {
    while (fgets(data, sizeof(data), file)) {
      char *tokkk = strtok(data, "\n");
      strcpy(blockedHostIp[lineNum], tokkk);
      lineNum++;
    }
    fclose(file);
  }
  return lineNum;
}

int main(int argc, char* argv[])
{
  pid_t pid;
  struct sockaddr_in addr_in, cli_addr, proxy_addr;
  struct hostent* host;
  int sockfd, newsockfd;
     
  if(argc < 3) {
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
   
    // unsigned int microseconds = 1000000;
    // usleep(microseconds);
    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clientLength);
       
    if(newsockfd < 0) {
      perror("Problem in accepting connection");
      exit(0);
    }
      
    pid = fork();
    if(pid == 0)
    {
      printf("******************************************************************\n\n");

      char* hostMap[50];
      char* ipMap[50];
      char* blockedHostIp[50];

      int hostIpIndex = 0;
      int blockedhostIpIndex = 0;

      for (int i=0; i < 50; i++) {
        hostMap[i] = calloc(50, sizeof(char));
        ipMap[i] = calloc(50, sizeof(char));
        blockedHostIp[i] = calloc(50, sizeof(char));
      }   
      
      hostIpIndex = loadIpHostCache(hostMap, ipMap);
      blockedhostIpIndex = loadBlockedHostsIP(blockedHostIp);


      struct sockaddr_in server_addr;
      int flag = 0, recvsocketId, n,port = 0, i,socketId;
      char buffer[510], httpMethod[300], hostStr[300], httpVersion[10], hashOfPath[50], hashFileName[100];
      char* temp = NULL;
      bzero((char*)buffer, 500);
      recv(newsockfd, buffer, 500, 0);
         
      sscanf(buffer, "%s %s %s", httpMethod, hostStr, httpVersion);
      bzero(hashFileName, sizeof(hashFileName));
      strcpy(hashFileName, "cache/");
         
      if(((strncmp(httpMethod, "GET", 3) == 0))&&((strncmp(httpVersion, "HTTP/1.1", 8) == 0)||(strncmp(httpVersion, "HTTP/1.0", 8) == 0))&&(strncmp(hostStr, "http://", 7) == 0))
      {
        printf("Request:\n%s %s %s\n\n", httpMethod, hostStr, httpVersion);
        strcpy(httpMethod, hostStr);

        getMd5Sum(httpMethod, hashOfPath);
           
        bzero(httpMethod,sizeof(httpMethod));
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
          } else if (hostStr[i] == '/') {
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

        int pass = getHostIP(hostMap, hostStr, hostIpIndex);

        if (pass == -1) {

          host = gethostbyname(hostStr);
          
          if (host != NULL) {
            strcpy(hostMap[hostIpIndex], hostStr);
            bcopy( (char*)host->h_addr, (char*)ipMap[hostIpIndex], host->h_length);
            
            
            pass = hostIpIndex;
            hostIpIndex++;

            char ipHostCacheData[200];
            bzero(ipHostCacheData, sizeof(ipHostCacheData));
            strcpy(ipHostCacheData, hostMap[hostIpIndex-1]);
            strcat(ipHostCacheData, " ");
            strcat(ipHostCacheData, ipMap[hostIpIndex-1]);
            strcat(ipHostCacheData, "#");
            ipHostCacheData[strlen(ipHostCacheData)] = '\0';
            
            writeFile(ipHostCacheFile, ipHostCacheData, strlen(ipHostCacheData), 1);
          } else {
            send(newsockfd, "SERVER NOT FOUND\n", 17, 0);

            close(socketId);
            close(newsockfd);
            close(sockfd);
            exit(0);
          }
        }
           
        bzero((char*)&server_addr, sizeof(server_addr));
        server_addr.sin_port=htons(port);
        server_addr.sin_family=AF_INET;

        strcpy((char*)&server_addr.sin_addr.s_addr, (char*)ipMap[pass]);

        char* ipAddr = inet_ntoa(server_addr.sin_addr);
        int blocked = checkIfBlockedIpHost(blockedHostIp, hostStr, ipAddr, blockedhostIpIndex);
        
        if (blocked == 1) {
          send(newsockfd, "ERROR 403 FORBIDDEN\n", 19, 0);
          close(socketId);
          close(newsockfd);
          close(sockfd);
          exit(0);
        }
        char fileInCache[1];
        strcat(hashFileName, hashOfPath);
        int cacheLine = isFileInCache(hashOfPath, argv[2], fileInCache);
        int fileExists = 0;
        
        if (fileInCache[0] == '1') {
          FILE *fileForCacheRead;
          fileForCacheRead = fopen(hashFileName, "r");

          if (fileForCacheRead) {
            fileExists = 1;
          }
          fclose(fileForCacheRead);
        }

        printf("fileExists:%d\n", fileExists);

        if (fileInCache[0] == '1' && fileExists == 1) {
          printf(".......RETURNING DATA FROM CACHE......\n");
          FILE *fileForCacheRead;
          int byte_read = 0;
          fileForCacheRead = fopen(hashFileName, "r");

          if (fileForCacheRead) {
            bzero((char*)buffer, sizeof(buffer));
            
            while (byte_read = fread(buffer, 1, 500, fileForCacheRead)) {
              printf("byte_read:%d\n", byte_read);
              send(newsockfd, buffer, byte_read, 0);
              
              if (byte_read < 0) {
                break;
              }
              bzero((char*)buffer, sizeof(buffer));
            }
          fclose(fileForCacheRead);  
          }
        } else {

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

          //bcopy((char*)host->h_addr, (char*)&server_addr.sin_addr.s_addr, host->h_length);
             
          socketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
          recvsocketId = connect(socketId, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
          
          if (recvsocketId < 0) {
            perror("Error in connecting to remote server");
            exit(0);
          }
             
          printf("HostName: %s\nIP: %s\n", hostStr, inet_ntoa(server_addr.sin_addr));

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
            int packetNum = 0;
            char* fileBufferData[500];
            int fileBufferSize[500];
            
            for (int i=0; i < 500; i++) {
              fileBufferData[i] = calloc(510, sizeof(char));
            }

            do
            {
              bzero((char*)buffer, 500);
              n = recv(socketId, buffer, 500, 0);
              
              if(n > 0) {
                send(newsockfd, buffer, n, 0);
                strcpy(fileBufferData[packetNum], buffer);
                fileBufferSize[packetNum] = n;
                packetNum++;
              }
              
            } while(n > 0);

            for (int i=0; i < packetNum; i++) {
              writeFile(hashFileName, fileBufferData[i], fileBufferSize[i], i);
            }
            updateCacheTimestamp(hashOfPath, cacheLine);

            for (int i=0; i < 500; i++) {
              free(fileBufferData[i]);
            }
          }
        }
      }
      else
      {
        send(newsockfd, "400 : BAD REQUEST\n", 18, 0);
      }
      for (int i=0; i < 50; i++) {
        free(hostMap[i]);
        free(ipMap[i]);
        free(blockedHostIp[i]);
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