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

//2MB = the maximum file size to be loaded in memory without fragmenting
#define MAX_CHUNK 2097154

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
    printf("\n***Usage: %s <ip of server> <port> <relative/path/filename>\n",argv[0]);
    return 1;
  }

  memset(recvBuff, '0',sizeof(recvBuff));
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\nERR: Could not create socket \n");
    return 1;
  }

  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[2]));

  if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
  {
    printf("\nERR: inet_pton error occured\n");
    return 1;
  }

  if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nERR: Connect Failed \n");
    return 1;
  }

  FILE *f;
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
      printf("File %s not found! \n",argv[3]);
    }
    else if (recvBuff[0] == 'T' && recvBuff[1] == 'M' && recvBuff[2] == 'C')
    {
      printf("Too many clients. Try again later \n");
    }
    else
    {
      char *file_save_name = (char*)malloc(50*sizeof(char));
      int pos = -1;
      char *path=(char*)malloc(25*sizeof(char));
      char *actual_file_name=(char*)malloc(25*sizeof(char));

      for (int i = strlen(argv[3])-1; i > -1; i--)
      {
        if (argv[3][i] == '/')
        {
          pos = i;
          break;
        }
      }

      if (pos > 0)
      {
        //SAVE REQUESTED FILE IN SAME PATH AS ORIGINAL
        /*
        memcpy(path, &argv[3][0], pos );
        memcpy(actual_file_name, &argv[3][pos+1], strlen(argv[3]) );
        printf("Path: *%s*; Actual file name *%s*\n",path,actual_file_name);
        sprintf(file_save_name,"%s/received_%s",path,actual_file_name); */
        //SAVE REQUESTED FILE IN CURRENT DIRECTORY
        memcpy(actual_file_name, &argv[3][pos+1], strlen(argv[3]) );
        //printf("Actual file name *%s*\n",actual_file_name);
        sprintf(file_save_name,"received_%s",actual_file_name);
      }
      else
      {
        sprintf(file_save_name,"received_%s",argv[3]);
      }

      free (path);
      free (actual_file_name);

      f = fopen(file_save_name, "wb");
      if (f == NULL)
      {
        printf("ERR: could not create file for saving, maybe wrong path ?\n");
        exit(-1);
      }

      long file_size = atoi(recvBuff);
      printf("Requested file size: %ld bytes\n",file_size);

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
      fclose(f);
    }
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    break;
  }

  printf("\n");
  return 0;
}
