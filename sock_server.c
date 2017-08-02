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

#define MAX_CHUNK 2097154 //2MB
#define MAX_CLIENTS 50

int listenfd = 0;

int sendall(int s, char *buf, int len)
{
  int total = 0;
  int bytesleft = len;
  int n;

  while(total < len) {
    n = send(s, buf+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }
  len = total;
  return n==-1?0:1;
}

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

void cleanup()
{
  close(listenfd);
  printf("Exiting \n");
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
  char fd_array[MAX_CLIENTS];
  int num_clients = 1;
  fd_set readfds, testfds;

  char sendBuff[128],recvBuff[128];
  int n = 0;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(sendBuff, '0', sizeof(sendBuff));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  listen(listenfd, 10);
  printf("Listenfd %d\n",listenfd);
  FD_ZERO(&readfds);
  FD_SET(listenfd, &readfds);

  FILE *f;

  signal(SIGINT, cleanup);
  atexit(cleanup);

  while(1)
  {
    testfds = readfds;
    select(FD_SETSIZE, &testfds, NULL, NULL, NULL);

    /* If there is activity, find which descriptor it's on using FD_ISSET */
    for (fd = 0; fd < FD_SETSIZE; fd++) {
      if (FD_ISSET(fd, &testfds)) {

        if (fd == listenfd) { /* Accept a new connection request */
          connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
          printf("COnnfd %d\n",connfd);

          if (num_clients < MAX_CLIENTS) {
            FD_SET(connfd, &readfds);
            fd_array[num_clients]=connfd;
            /*Client ID*/
            printf("Client %d joined\n",num_clients++);
            fflush(stdout);
          }
          else {

            //sprintf(msg, "Sorry, too many clients.  Try again later.\n");
            //write(client_sockfd, msg, strlen(msg));
            close(connfd);
          }
        }
        else if(fd)
        {  /*Process Client specific activity*/
          n = read(fd, recvBuff, sizeof(recvBuff)-1);
          recvBuff[n] = '\0';
          printf("RECBUFF: %s\n",recvBuff);

          char *source = NULL;
          long bufsize;
          char *filename = trim_string(recvBuff);

          printf("\nFILENAME trimmed: ***%s***\n",filename);
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
              printf("File size read: %s (%d bytes sent)\n",sendBuff,n);

              if (bufsize + 1 <= MAX_CHUNK)
              {
                source = malloc(sizeof(char) * (bufsize + 1));
                if (fseek(f, 0L, SEEK_SET) != 0)
                {
                  printf("Error fseek!\n");
                }

                int newLen = fread(source, sizeof(char), bufsize, f);
                if ( ferror( f ) != 0 )
                {
                  fputs("Error reading file", stderr);
                } else {
                  source[newLen++] = '\0'; //just to be safe
                }

                fclose(f);
                printf("FILE: %s\n",source);

                n = read(fd, recvBuff, sizeof(recvBuff)-1);
                recvBuff[n] = '\0';
                char *test_ok = trim_string(recvBuff);
                if (test_ok[0] == 'o' && test_ok[1] == 'k') {
                  if (sendall(fd,source,bufsize))
                    printf("File sent!\n");
                  else printf("Error sending file!\n");
                }
                else {
                  printf("Client didn't want to accept the file!\n");
                }
                free(test_ok);
                free(filename);
                free(source);
              }
              else
              {
                 int total_sent = 0;
                 int bytesleft = bufsize;
                 source = malloc(sizeof(char) * (MAX_CHUNK));
                 while (total_sent != bufsize)
                 {
                   fseek(f, total_sent, SEEK_SET);

                   if (bytesleft >= MAX_CHUNK)
                   {
                     int newLen = fread(source, sizeof(char), MAX_CHUNK, f);
                     if ( ferror( f ) != 0 )
                     {
                       fputs("Error reading file", stderr);
                     }
                     if (sendall(fd,source,MAX_CHUNK))
                     {
                       total_sent += MAX_CHUNK;
                       bytesleft -= MAX_CHUNK;
                     }
                    }
                    else
                    {
                      int newLen = fread(source, sizeof(char), bytesleft, f);
                      if ( ferror( f ) != 0 )
                      {
                        fputs("Error reading file", stderr);
                      }
                      if (sendall(fd,source,bytesleft))
                      {
                        total_sent += bytesleft;
                        bytesleft -= bytesleft;
                      }
                    }
                 }//while
              } //else
            }

            close(fd);
            num_clients--;

          }
          else
          {
            printf("File not found!\n");
            send(fd, "FNF", strlen("FNF"), 0);
            close(fd);
            num_clients--;
          }

        }//if
      }//if
    }//for

    close(connfd);
    sleep(1);
  } //while

  return 1;
}
