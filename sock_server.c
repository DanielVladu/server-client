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

#define MAX_CHUNK 2097154 //2MB

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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
      printf("***USAGE: ./server <port>\n");
      exit(1);
    }

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

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

    FILE *f;

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        n = read(connfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = '\0';
        printf("RECBUFF: %s\n",recvBuff);

        char *source = NULL;
        long bufsize;
        char *filename = trim_string(recvBuff);

        printf("\nFILENAME trimmed: ***%s***\n",filename);
        f = fopen(filename, "r");
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
                sprintf(sendBuff,"%ld",bufsize + 1);
                send(connfd, sendBuff, strlen(sendBuff), 0);
                //printf("SENDBUFF: %s\n",sendBuff);

                if (bufsize + 1 <= MAX_CHUNK)
                {
                  source = malloc(sizeof(char) * (bufsize + 1));
                  if (fseek(f, 0L, SEEK_SET) != 0)
                  {
                    printf("Error fseek!\n");
                   }

                  size_t newLen = fread(source, sizeof(char), bufsize, f);
                  if ( ferror( f ) != 0 )
                  {
                      fputs("Error reading file", stderr);
                  } else {
                      source[newLen++] = '\0';
                  }
                }
            }
            fclose(f);
            printf("FILE: %s\n",source);
        }
        else
        {
          printf("File not found!\n");
          send(connfd, "FNF", strlen("FNF"), 0);
          return 1;
        }

        n = read(connfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = '\0';
        char *test_ok = trim_string(recvBuff);
        if (test_ok[0] == 'o' && test_ok[1] == 'k') {
            if (sendall(connfd,source,bufsize))
              printf("File sent!\n");
            else printf("Error sending file!\n");
        }
        else {
          printf("Client didn't want to accept the file!\n");
        }

        free(filename);
        free(source);
        //close(connfd);
        sleep(1);
     }
}
