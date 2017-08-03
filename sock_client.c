#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAX_CHUNK 2097154 //the maximum file size to be loaded in memory without fragmenting

char* trim_string(char *buff)
{
  int i = 0;
  int k = 0;
  for (i = 0;i<strlen(buff);i++)
  {
    if (buff[i] != '\n' && buff[i] != '\r') k++;
  }
  char *a = (char*)malloc((k+1)*sizeof(char));
  for (i=0;i<k;i++)
  {
    if (buff[i] != '\n' || buff[i] != '\r')
    {
      a[i] = buff[i];
    }
  }
  a[i+1] = '\0';
  return a;
}

int receiveall(int s, char *buf, int len)
{
  int total = 0;
  int bytesleft = len;
  int n;

  while(total < len) {
    n = read(s, buf+total, bytesleft);
    if (n == -1 || n == 0) { break; }
    total += n;
    bytesleft -= n;
  }
  len = total;
  return n == -1 ? 0 : total;
}

void cleanup()
{
  printf("Exiting\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd = 0, n = 0;
  char recvBuff[128];
  char *recvFile;
  struct sockaddr_in serv_addr;

  signal(SIGINT, cleanup);
  atexit(cleanup);
  printf("\n");

  if(argc != 4)
  {
    printf("\n ***Usage: %s <ip of server> <port> <filename>\n",argv[0]);
    return 1;
  }

  memset(recvBuff, '0',sizeof(recvBuff));
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Error : Could not create socket \n");
    return 1;
  }

  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[2]));

  if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
  {
    printf("\n inet_pton error occured\n");
    return 1;
  }

  if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\n Error : Connect Failed \n");
    return 1;
  }

  char *file_save_name = (char*)malloc(50*sizeof(char));
  sprintf(file_save_name,"received_%s",argv[3]);
  FILE *f = fopen(file_save_name, "wb");
  char *file_requested = argv[3];

  while (1)
  {
    n = send(sockfd, file_requested, strlen(file_requested), 0);
    printf("Sent file name (%d bytes)\n",n);

    printf("Start receiving\n");
    n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
    recvBuff[n] = 0;
    if (recvBuff[0] == 'F' && recvBuff[1] == 'N' && recvBuff[2] == 'F')
    {
      printf("File not found! \n");
    }
    else if (recvBuff[0] == 'T' && recvBuff[1] == 'M' && recvBuff[2] == 'C')
    {
      printf("Too many clients. Try again later \n");
    }
    else
    {
      long file_size = atoi(recvBuff);
      printf("File size received: %ld bytes\n",file_size);

      if (file_size < MAX_CHUNK)
      {
        recvFile = (char*)malloc(file_size*sizeof(char));

        send(sockfd, "ok", 2, 0);
        n = receiveall(sockfd, recvFile,file_size);

        printf("Bytes received from file: %d bytes\n",n);
        if (n == file_size) printf("File received! Saved as %s\n",file_save_name);

        fwrite(recvFile , 1 , file_size , f);

        free(recvFile);
      }
      else
      {
        long total_received = 0;
        long bytesleft = file_size;
        recvFile = (char*)malloc(MAX_CHUNK*sizeof(char));
        send(sockfd, "ok", 2, 0);
        printf("Receiving file...\n");
        int i = 1; int width = 20;
        while (total_received < file_size)
        {
          if (bytesleft > MAX_CHUNK)
          {
            n = receiveall(sockfd, recvFile, MAX_CHUNK);
            fwrite(recvFile , 1 , MAX_CHUNK , f);
            total_received += MAX_CHUNK;
            bytesleft -= MAX_CHUNK;
          }
          else
          {
            n = receiveall(sockfd, recvFile, bytesleft);
            fwrite(recvFile , 1 , bytesleft, f);
            total_received += bytesleft;
            bytesleft -= bytesleft;
          }
          if (total_received >= i*0.05*file_size && i < 21)
          {
            i = (int)(total_received/(int)(0.05*file_size));
            printf("\r[");
            fflush(stdout);
            for (int k=0; k < width;k++)
            {
              if (i < k) printf(" ");
              else printf("=");
              fflush(stdout);
            }
            printf("] %d%%  ",i*5);
            fflush(stdout);
          }
        }
        printf("\n");
        printf("Total received %ld bytes\n",total_received);
        if (total_received == file_size) printf("File received! Saving as %s\n",file_save_name);
        else printf("File not received entirely. Saving as %s\n",file_save_name);
        free(recvFile);
        free(file_save_name);
      }
    }
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    break;
  }

  fclose(f);

  printf("\n");
  return 0;
}
