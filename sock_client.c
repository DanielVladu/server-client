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

#define MAX_CHUNK 2097154 //2MB

char* trim_string(char *buff)
{
  int i = 0;
  int k = 0;
  for (i = 0;i<strlen(buff);i++)
  {
    if (buff[i] != '\n' && buff[i] != '\r') k++;
  }
  char *a = (char*)malloc(k*sizeof(char));
  for (i=0;i<k;i++)
  {
    if (buff[i] != '\n' || buff[i] != '\r')
    {
      a[i] = buff[i];
    }
  }
  return a;
}

int main(int argc, char *argv[])
{
  int sockfd = 0, n = 0;
  char recvBuff[128], sendBuff[128];
  char *recvFile;
  struct sockaddr_in serv_addr;

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

  FILE *f = fopen("received_file.txt", "wb");
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
    else
    {
      int file_size = atoi(recvBuff);
      printf("File size received: %d bytes\n",file_size);


      //for (int i=0;i<strlen(recvBuff);i++)
        //recvBuff[i] = '\0';

      if (file_size < MAX_CHUNK)
      {
        recvFile = (char*)malloc(file_size*sizeof(char));
        send(sockfd, "ok", 2, 0);
        n = read(sockfd, recvFile, file_size);
        recvFile[n] = 0;
        printf("Bytes received from file: %d bytes\n",n - 1);
        if (n == file_size - 1) printf("File received!\n");
        char *received_text_file = trim_string(recvFile);
        //printf("recvBUFF **%s**\n",recvFile);
        //printf("recvBUFF trimmed **%s**\n",received_text_file);
        fwrite(recvFile , 1 , file_size , f);

        free(received_text_file);
        free(recvFile);
      }
      else
      {
        int total_received = 0;
        int bytesleft = file_size;
        while (total_received != file_size)
        {
          if (bytesleft >= MAX_CHUNK)
          {
            recvFile = (char*)malloc(MAX_CHUNK*sizeof(char));
            send(sockfd, "ok", 2, 0);
            n = read(sockfd, recvFile, MAX_CHUNK);
            fwrite(recvFile , 1 , MAX_CHUNK , f);
            total_received += MAX_CHUNK;
            bytesleft -= MAX_CHUNK;
          }
          else
          {
            recvFile = (char*)malloc(bytesleft*sizeof(char));
            send(sockfd, "ok", 2, 0);
            n = read(sockfd, recvFile, bytesleft);
            fwrite(recvFile , 1 , bytesleft, f);
            total_received += bytesleft;
            bytesleft -= bytesleft;
          }
        }
      }
    }
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    break;
  }

  fclose(f);

  if(n < 0)
  {
    printf("\n Read error \n");
  }

  printf("\n");
  return 0;
}
