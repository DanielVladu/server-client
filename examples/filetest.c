#include <stdio.h>
#include <stdlib.h>


int main() {
  char *source = NULL;
  FILE *fp = fopen("foo.txt", "r");
  if (fp != NULL)
  {
      if (fseek(fp, 0L, SEEK_END) == 0)
      {
          long bufsize = ftell(fp);
          if (bufsize == -1)
          {
            printf("Error ftell\n");
          }

          source = malloc(sizeof(char) * (bufsize + 1));
          if (fseek(fp, 0L, SEEK_SET) != 0)
          {
            printf("Error fseek!\n");
           }

          size_t newLen = fread(source, sizeof(char), bufsize, fp);
          if ( ferror( fp ) != 0 )
          {
              fputs("Error reading file", stderr);
          } else {
              source[newLen++] = '\0';
          }
      }
      fclose(fp);
      printf("file: %s\n",source);
  }
  else printf("file not found\n");

  free(source); 
  return 0;
}
