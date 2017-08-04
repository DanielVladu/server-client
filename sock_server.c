#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

//2MB = the maximum file size to be loaded in memory without fragmenting
#define MAX_CHUNK 2097154 //2MB
#define MAX_CLIENTS 20

int listenfd = 0;

int sendall(int s, char *buf, int len)
{
  int total = 0;
  int bytesleft = len;
  int n;

  while(total < len) {
    n = send(s, buf+total, bytesleft, MSG_NOSIGNAL);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }
  len = total;
  return n == -1 ? 0 : total;
}

char* trim_string(char *buff)
{
  int i = 0;
  int k = 0;

  for (i = 0;i < strlen(buff);i++)
  {
    if (buff[i] != '\n' || buff[i] != '\r') k++;
  }

  char *a = malloc((k+1)*sizeof(char));
  for (i=0;i<k;i++)
  {
    if (buff[i] != '\n' || buff[i] != '\r')
    {
      a[i] = buff[i];
    }
  }
  a[i] = '\0';
  return a;
}

void cleanup()
{
  close(listenfd);
  printf("\nExiting ");
  exit(1);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("***USAGE: ./server <port>\n");
    exit(1);
  }

  int connfd = 0;
  struct sockaddr_in serv_addr;

  int fd;
  //char fd_array[MAX_CLIENTS];
  int num_clients = 1;
  fd_set readfds;

  char sendBuff[128],recvBuff[128];
  int n = 0;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(sendBuff, '0', sizeof(sendBuff));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  listen(listenfd, 1);
  FD_ZERO(&readfds);
  FD_SET(listenfd, &readfds);
  FD_SET(0, &readfds);  /* Add keyboard to file descriptor set */

  printf("Server listening on port %d\n",atoi(argv[1]));
  FILE *f;

  signal(SIGINT, cleanup);
  atexit(cleanup);

  while(1)
  {
    select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

    for (fd = 0; fd < FD_SETSIZE; fd++) {
      if (FD_ISSET(fd, &readfds)) {

        if (fd == listenfd)
        {
          connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

          if (num_clients < MAX_CLIENTS)
          {
            FD_SET(connfd, &readfds);
            //fd_array[num_clients]=connfd;
            printf("\nClient %d connected\n",num_clients++);
            fflush(stdout);
          }
          else {
            printf("Too many clients.  Try again later.\n");
            sendall(connfd, "TMC", strlen("TMC"));
            close(connfd);
          }
        }
        else if(fd)
        {
          n = read(fd, recvBuff, sizeof(recvBuff)-1);
          recvBuff[n] = '\0';

          char *source = NULL;
          long bufsize;
          char *filename = trim_string(recvBuff);

          printf("File name trimmed: *%s*\n",filename);
          f = fopen(filename, "rb");
          if (f != NULL)
          {
            if (fseek(f, 0L, SEEK_END) == 0)
            {
              bufsize = ftell(f);
              if (bufsize == -1)
              {
                printf("Error ftell\n");
              }

              //sending file size
              sprintf(sendBuff,"%ld",bufsize);
              n = send(fd, sendBuff, strlen(sendBuff), 0);
              printf("File size read with ftell: %s (%d bytes sent)\n",sendBuff,n);

              if (fseek(f, 0L, SEEK_SET) != 0)
              {
                printf("Error fseek!\n");
              }

              if (bufsize + 1 <= MAX_CHUNK)
              {
                source = malloc(sizeof(char) * (bufsize + 1));

                int newLen = fread(source, sizeof(char), bufsize, f);
                if ( ferror( f ) != 0 )
                {
                  fputs("Error reading file", stderr);
                } else {
                  source[newLen++] = '\0';
                }

                fclose(f);

                n = read(fd, recvBuff, sizeof(recvBuff)-1);
                recvBuff[n] = '\0';
                char *test_ok = trim_string(recvBuff);
                if (test_ok[0] == 'o' && test_ok[1] == 'k') {
                  if ((n = sendall(fd,source,bufsize)))
                  {
                    printf("File sent (total %d bytes)!\n",n);
                  }
                  else {
                    printf("Error sending file!\n");
                  }
                }
                else {
                  printf("Client didn't want to accept the file!\n");
                }

                free(test_ok);
                free(source);
              }
              else
              {
                long total_sent = 0;
                int bytesread = 0;
                source = malloc(sizeof(char) * (MAX_CHUNK));
                printf("Sending file...\n");
                int i = 1; int width = 20;
                while ((bytesread = fread(source, sizeof(char), MAX_CHUNK, f)) > 0 )
                {
                  //sending file
                  if (sendall(fd,source,bytesread))
                  {
                    total_sent += bytesread;
                  }
                  //progres bar
                  if (total_sent >= i*0.05*bufsize && i < 21)
                  {
                    i = (int)(total_sent/(int)(0.05*bufsize));
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
                }//while
                printf("\n");

                if (total_sent == bufsize)
                {
                  printf("File sent (total %ld of %ld bytes)\n",total_sent,bufsize);
                }
                else
                {
                  printf("File not sent (total %ld of %ld bytes)\n",total_sent,bufsize);
                }
                free(source);
              } //else
            }

            printf("Client %d disconnected\n",--num_clients);
            close(fd);

          }
          else
          {
            printf("File %s not found!\n",filename);
            send(fd, "FNF", strlen("FNF"), 0);
            printf("Client %d disconnected\n",--num_clients);
            close(fd);
          }

          free(filename);
        }//if
      }//if
    }//for

    close(connfd);
    //sleep(1);
  } //while

  return 1;
}
