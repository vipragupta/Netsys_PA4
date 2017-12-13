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

//Names of the files that Proxy server uses.
#define ipHostCacheFile "htimhtim"
#define forbiddenIPAddressFile "forbiddenIp.txt"
#define cacheFile "cacheFileData.txt"

//This function checks if the given IP address or hostname is one of the blocked ones.
int checkIfBlockedIpHost(char* blockedList[], char* host, char* ip, int maxIndex) {
  
  for (int i = 0; i < maxIndex; i++) {
    if (strcmp(blockedList[i], host) == 0 || strcmp(blockedList[i], ip) == 0 ) {
      printf("Blocked IP/Host.\n\n");
      return 1;
    }
  }

  return -1;
}

//This function calculates the md5Sum of the given URI.
void getMd5Sum(char *hostStr, char* c_new) {

  unsigned char c[64];
  bzero(c, sizeof(c));
  bzero(c_new, sizeof(c_new));

  bzero(c, sizeof(c));
  MD5_CTX mdContext;
  MD5_Init (&mdContext);

  MD5_Update (&mdContext, hostStr, strlen(hostStr));
  MD5_Final (c, &mdContext);
  
  //Converting the MD5 into a char array.
  for(int i = 0; i < strlen(c); i++) {
    char temp[3];
    sprintf(temp, "%0x", c[i]);
    strcat(c_new, temp);
  }
}


//This function checks the hostname to ip cache and returns the IP address if it exists in the cache.
int getHostIP(char* hostMap[], char* host, int maxIndex) {
  for (int i = 0; i < maxIndex; i++) {
    if (strcmp(hostMap[i], host) == 0) {
      printf("Found corresponding IP of Hostname in cache.\n\n");
      return i;
    }
  }
  printf("Couldn't find corresponding IP of HostName in cache.\n\n");
  return -1;
}

//A generalized function to write the data into the given file name. 'append' parameter tells if the data should be appended to te file or the file
//should be truncated and then data should be written.
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

/*This function checks the URI vs timestamp mapping for cache and returns the line number where that mapping exists in the mapping file.
If the URI doesn't have a associted cache file, then it return -1.
If the cache file exists but has expired, then the line number where that mapping exists in the mapper file is returned.

pathHash: the hash value of the uri
allowedDifference: time in seconds which tells when a cache file will be considered as expired.
*yes: single element char array which is set to 1 by funtion when the cache file exists and hasn't expired.

retuns the line number where the mapping of hash vs timestamp exists in the mapper file, this is returned irrespective of the cache is new or expired.

*/
int isFileInCache(char *pathHash, char *allowedDifference, char *yes) {

  FILE *file;
  char line[1024]; 
  time_t now;
  now = time(NULL);
  yes[0] = '0';
  file = fopen(cacheFile, "r");
  int index = 0;
  int matchIndex = -1;

  if (file) {
    while (fgets(line, sizeof(line), file)) {
      char *lineTemp = strtok(line, "\n");
      char *hash = strtok(lineTemp, " ");

      if (hash != NULL) {
        char *timestamp = strtok(NULL, " ");
        
        if (timestamp != NULL) {
          
          
          if (strcmp(hash, pathHash) == 0) {
            long int then;
            long int allowedDifferenceInt;
            then = atol(timestamp);
            long int difference = difftime(now, then);

            allowedDifferenceInt = atol(allowedDifference);
            printf("Cache time Diff:%ld        ", difference);
            printf("allowed: %ld\n", allowedDifferenceInt);

            if (difference <= allowedDifferenceInt) {
              yes[0] = '1';
            }
            matchIndex = index;
          }
        }
      }
      index++;
    }
    fclose(file);
  }
  
  return matchIndex;
}

/*
This function updates the timestamp in the mapper file for the given URI's hash.
If lineNum is -1, that means that there is no entry of this URI in mapper file and so a new entry should be made in the mapper file.
Else just find that entry and update its timestamp.
*/
void updateCacheTimestamp(char *pathHash, int lineNum) {
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

  FILE *file;
  
  int index = -1;
  int flag = 0;
  
    if (lineNum != -1) {
      file = fopen(cacheFile, "r+");
      if (file) {
        fseek(file, 0, 0);
        do {
          if (index+1 == lineNum) {
            fwrite(updateData , sizeof(unsigned char), strlen(updateData), file);
            flag = 1;
            break;
          }
          index++;
        } while (fgets(line, sizeof(line), file));
      }
    }

    if (flag == 0) {
      file = fopen(cacheFile, "a+");
      if (file) {
        fwrite(updateData , sizeof(unsigned char), strlen(updateData), file);
      }
    }
    printf("\nCACHE UPDATED!!!!\n");
    fclose(file);
}

//Read the Host to IP cache file and load the values in the passed char arrays.
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
  for (int i=0; i < 50; i++) {
    free(dataTemp[i]);
  } 
  return index;
}

//Read the blocked Host and IP file and load the values in the passed char array.
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
   
    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clientLength);
       
    if(newsockfd < 0) {
      perror("Problem in accepting connection");
      exit(0);
    }
      
    pid = fork();
    if(pid == 0)
    {
      printf("\n\n\n");
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
         
      //Continue only if the request is a valid request.
      if(((strncmp(httpMethod, "GET", 3) == 0))&&((strncmp(httpVersion, "HTTP/1.1", 8) == 0)||(strncmp(httpVersion, "HTTP/1.0", 8) == 0))&&(strncmp(hostStr, "http://", 7) == 0))
      {
        printf("Request: %s %s %s\n\n", httpMethod, hostStr, httpVersion);
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
        
        //Check if the mapper file has an entry for the URI's hash. If yes, then has the cache expired or is fresh.
        int cacheLine = isFileInCache(hashOfPath, argv[2], fileInCache);
        int fileExists = 0;
        
        //If mapper file has an entry that there is a fresh cache file for the URI hash, then check if that cache file really exists on the disk or not.
        //There is a chance that mapper has an entry, but in actual no such file exists on the disk.
        if (fileInCache[0] == '1') {
          FILE *fileForCacheRead;
          fileForCacheRead = fopen(hashFileName, "r");

          if (fileForCacheRead) {
            fileExists = 1;
          }
          fclose(fileForCacheRead);
        }


        if (fileInCache[0] == '1' && fileExists == 1) {
          //If the cache is fresh and the cache file exists, then read data from that file and send it to the client.
          printf(".......RETURNING DATA FROM CACHE......\n");
          FILE *fileForCacheRead;
          int byte_read = 0;
          fileForCacheRead = fopen(hashFileName, "r");

          if (fileForCacheRead) {
            bzero((char*)buffer, sizeof(buffer));
            
            while (byte_read = fread(buffer, 1, 500, fileForCacheRead)) {
              
              send(newsockfd, buffer, byte_read, 0);
              
              if (byte_read < 0) {
                break;
              }
              bzero((char*)buffer, sizeof(buffer));
            }
          fclose(fileForCacheRead);  
          }
        } else {
          //The data doesn't exist in the cache or has expired for the given URI's hash. There is also a chance that the mapper file has an entry for the hash,
          //but the cache file doesn't exist on disk.
          printf(".......URL DATA Doesn't EXIST or HAS EXPIRED in CACHE......\n\n");
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
             
          socketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
          recvsocketId = connect(socketId, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
          
          if (recvsocketId < 0) {
            send(newsockfd, "Error in connecting to remote server. Please try again.\n", 56, 0);
            close(socketId);
            close(newsockfd);
            close(sockfd);
            perror("Error in connecting to remote server");
            exit(0);
          }

          printf("HostName: %s\nIP: %s\n", hostStr, inet_ntoa(server_addr.sin_addr));
          bzero((char*)buffer, sizeof(buffer));
          
          //Construct the request to send to Server.
          if(temp != NULL) {
            sprintf(buffer, "GET http://%s/%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", hostStr, temp, httpVersion, hostStr);
          }
          else {
            sprintf(buffer, "GET http://%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", hostStr, httpVersion, hostStr);
          }
          
          printf("Request to Server:  %s\n", buffer);
          
          //Send the request to server and get the data back.
          n = send(socketId, buffer, strlen(buffer), 0);

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
                memcpy(fileBufferData[packetNum], buffer, n); //Save the data to update the cache.
                fileBufferSize[packetNum] = n;
                packetNum++;
              }
              
            } while(n > 0);

            //Update the cache.
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