#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

extern int errno;

#define PORT 1031

int main(int argc,char *argv[])
{
   int sd; // descriptorul socketului
   struct sockaddr_in server;
   char command[100];
   char received[100];

   char nume[100];
   char parola[100];
   char user_type[100];
   char email[100];

   int logged = 0;

   if((sd = socket(AF_INET,SOCK_STREAM,0)) == -1)
   {
     perror("Erroare la deschiderea socketului in client.\n");
     return errno;
   }

   server.sin_family = AF_INET;
   server.sin_addr.s_addr = inet_addr("127.0.0.1");
   server.sin_port = htons(PORT);

   if(connect(sd,(struct sockaddr *) &server, sizeof(struct sockaddr))==-1)
   {
    perror("Erroare la connect in client.\n");
    return errno;
   }

   while(1)
   {
    printf("Dati comanda: ");
    fflush(stdout);

   scanf("%s",&command);
   fflush(stdin);

    write(sd,&command,sizeof(command)); // scrierea catre server
    read(sd,&received,sizeof(received)); // citirea din server

    printf("The command received from server is: ");
    printf("%s\n",received);
    fflush(stdout);

    if(strcmp(received,"quit")==0) {
    printf("Ending the connection...\n");
    break;
    }
    else if((strcmp(received,"login")==0) && (logged == 0)){

          write(sd,"NORMAL_STUFF",sizeof("NORMAL_STUFF"));

          printf("Dati username: ");fflush(stdout);
          scanf("%s",nume);fflush(stdin);
          printf("Dati parola: ");fflush(stdout);
          scanf("%s",parola);fflush(stdin);

          write(sd,&nume,sizeof(nume));
          write(sd,&parola,sizeof(parola));

          read(sd,&received,sizeof(received));
          printf("%s\n",received);
          if( (strcmp(received,"Admin logged in!") == 0) || (strcmp(received,"Logged in!") == 0) ) logged = 1;
          fflush(stdout);
         }
         else if((strcmp(received,"register")==0) && (logged==0)){

                write(sd,"NORMAL_STUFF",sizeof("NORMAL_STUFF"));

                printf("Dati username: ");fflush(stdout);
                scanf("%s",nume);fflush(stdin);
                printf("Dati parola: ");fflush(stdout);
                scanf("%s",parola);fflush(stdin);
                printf("Alegeti tipul de user. Type 'admin' sau 'normal'.\n");fflush(stdout);
                scanf("%s",user_type);fflush(stdin);
                printf("Dati un email: ");fflush(stdout);
                scanf("%s",email);fflush(stdin);

                write(sd,&nume,sizeof(nume));
                write(sd,&parola,sizeof(parola));
                write(sd,&user_type,sizeof(user_type));
                write(sd,&email,sizeof(email));

                read(sd,&received,sizeof(received));
                printf("%s\n",received);
                fflush(stdout);

                }
                else if(((strcmp(received,"login")==0) || (strcmp(received,"register")==0)) && (logged == 1))
                {
                  write(sd,"Why",sizeof("Why"));
                  read(sd,&received,sizeof(received));
                  printf("%s\n",received);
                  fflush(stdout);
                }
                else if(strcmp(received,"show_all")==0){

                      printf("The commands list is as follows:\n");
                      printf("Unrestricted: 'login' 'register' 'quit'\n");
                      printf("Commands for all users: 'all_commands' 'show_champs'\n");
                      printf("Commands for normal users only: 'join_champ' 'postpone'\n");
                      printf("Commands for administrators only: 'history' 'create_champ'\n");
                      fflush(stdout);
                      }
                      else if( (strcmp(received,"active_champ")==0) || (strcmp(received,"join_champ")==0) || (strcmp(received,"create_champ")==0)|| (strcmp(received,"postpone")==0) || (strcmp(received,"round_history")==0))
                      {
                        printf("NOT YET IMPLEMENTED!\n"); fflush(stdout);
                      }
                      else if(strcmp(received,"No commands")==0){
                       printf("No commands found. Try typing 'all_commands' ( or 'quit' in order to stop ).\n");
                       }
   }

   close(sd);
   return 0;
}
