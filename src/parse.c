
#include <stdio.h>

int main(int argc,char* argv[])
{
    char hostname[50];
    int port[4];
    char identifier[50];
    int p = 0;
    int i,j,u;
    int k =0;
    //printf("%c\n",argv[1][6]);
    for(i = 7; i < strlen(argv[1]);i++){
      if(argv[1][i] == ':'){
        p = -2;
        for(j =0; j < i;j++){
          hostname[j] = argv[1][j];
          printf("host:");
          printf("%c\n",hostname[j]);
         }
      }
      if(p == -2){
        for(u = 0; u<4;u++){
          port[u] = argv[1][i+u+1] - '0';
          printf("port");
          printf("%d\n",port[u]);
        }
        p =-1;
      }
      if(p == -1 && i+5 <strlen(argv[1])){

        identifier[k] = argv[1][i+5];
        printf("identifier");
        printf("%c\n",identifier[k]);
        k++;

      }

      if(argv[1][i] == '/' && p == 0){
        port[0] = '8' - '0';
        port[1] = '0' - '0';
        port[2] = NULL - '0';
        port[3] = NULL - '0';
        p = 1;
        for(j =0; j < i;j++){
          hostname[j] = argv[1][j];
          printf("host/");
          printf("%c\n",hostname[j]);
        }
      }
      if(p == 1){
        identifier[k] = argv[1][i];
        printf("identifier");
        printf("%c\n",identifier[k]);
        k++;
      }

      }
    printf("final\n");
    printf("%d\n",port[0]);
    printf("%d\n",port[1]);
    printf("%d\n",port[2]);
    printf("%d\n",port[3]);
    printf("%s\n",hostname);
    printf("%s\n",identifier);


}
