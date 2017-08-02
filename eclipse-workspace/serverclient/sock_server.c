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

    return n==-1?-1:0;
}

void trim_string(char *a,char *buff)
{
  int i = 0;
  int k = 0;
  for (i = 0;i<strlen(buff);i++)
  {
    if (buff[i] == '\n' || buff[i] == '\r') k++;
  }
  for (i=0;i<k;i++)
  {
    if (buff[i] != '\n' || buff[i] != '\r')
    {
      a[i] = buff[i];
    }
  }
  a[i]= '\0';
}

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
      printf("***USAGE: ./server <port>\n");
      exit(1);
    }

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char sendBuff[1025],recvBuff[1025];
    int n = 0;
    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(9999);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    FILE *f;

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);


        while ( (n = read(connfd, recvBuff, sizeof(recvBuff)-1)) > 0)
        {
            recvBuff[n] = '\0';
            if(fputs(recvBuff, stdout) == EOF)
            {
                printf("\n Error : Fputs error\n");
            }
            else break;

        }

        /*
        while (1)
        {
            n = read(connfd, recvBuff[i], 1);
            if (n)
            {
              i++;
              if (recvBuff[i-1] == '\0' || recvBuff[i-1] == '\n' || recvBuff[i-1] == '\r')
              {
                recvBuff[i] = '\0';
                //filename=(char)malloc((i+1)*sizeof(char));

                //sprintf(filename,"%s",recvBuff);
                printf("RECVBUFF: %s\n",recvBuff);
                break;
              }
            }
            else
            {
              printf("Error receiving filename!\n");
              return 1;
            }
        }*/

        char *source = NULL;
        long bufsize;
        char filename[strlen(recvBuff)];
        trim_string(filename, recvBuff);
        printf("FILENAME: ***%s***\n",filename);
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

                sprintf(sendBuff,"%ld\0",bufsize + 1);
                send(connfd, sendBuff, strlen(sendBuff), 0);
                //printf("SENDBUFF: %s\n",sendBuff);

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
            fclose(f);
            printf("FILE: %s\n",source);
        }
        else
        {
          printf("File not found!\n");
          return 1;
        }
        /*
        while (1)
        {
            recvBuff[n] = 0;
            if(fputs(recvBuff, stdout) == EOF)
            {
                printf("\n Error : Fputs error\n");
            }

        }*/

        sendall(connfd,source,bufsize);
        /*
        ticks = time(NULL);*/
        //fgets(sendBuff, sizeof(sendBuff), stdin);
        //snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
        //write(connfd, sendBuff, strlen(sendBuff));

        close(connfd);
        sleep(1);
     }
}
