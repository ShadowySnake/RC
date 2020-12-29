#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sqlite3.h>

sqlite3* database;

extern int errno;

#define PORT 1031

static int callback(void *str,int argc, char **argv,char **azColName)
{
  int i;
  char* data =(char*) str;
  for(i = 0; i<argc ; i++)
  {
    strcat(data,azColName[i]);
    strcat(data, " = ");
    if(argv[i]) strcat(data,argv[i]);
    else strcat(data,"NULL");
    strcat(data,"\n");
  }

  strcat(data,"\n");
  return 0;
}

typedef struct threadData{
  int idThread; // id-ul threadului
  int cl; // descriptorul clientului ( intors de accept )
}threadData;

void Client_Commands(void* arg)
{
  char received[1000];
  char sent[100];

  char nume[100];
  char parola[100];
  char user_type[100];
  char email[100];
  char admin_type[]="admin";
  int admin_logon = 0;
  // se verifica daca loginul este de tip administrator
  // in caz contrar se presupune ca userul logat este unul normal

  char *errmsg;
  int rc = sqlite3_open("database",&database);
  struct threadData th;
  th = *((struct threadData*)arg);
  char str[100];
  char sql[100];

  while(1){

     if(read(th.cl,&received,sizeof(received)) == -1)
     {
      printf("Eroare la read de la client, in thread-ul %d.\n",th.idThread);
      perror("Eroare la read de la client.");
      break;
     }

     if(strcmp(received,"quit") == 0)
     {
      printf("A quit signal will be sent to client in thread %d.\n",th.idThread);
      strcpy(sent,"quit");
      write(th.cl,&sent,sizeof(sent));
      break;
     }
     else if(strcmp(received,"login") == 0) {
     strcpy(sent,"login");
     write(th.cl,&sent,sizeof(sent));

     read(th.cl,&received,sizeof(received));

     if(strcmp(received,"Why") == 0)
     {
       write(th.cl,"Already logged in!",sizeof("Already logged in!"));
     }
     else {

     read(th.cl,nume,sizeof(nume));
     read(th.cl,parola,sizeof(parola));

     sql[0]=0;
     str[0]=0;

     sprintf(sql,"SELECT username FROM users WHERE username='%s';",nume);
     rc = sqlite3_exec(database,sql,callback,str,&errmsg);
     if(strstr(str,nume))
     {
       sql[0]=0;
       str[0]=0;

       sprintf(sql,"SELECT password FROM users WHERE username='%s' AND password='%s';",nume,parola);
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);
       if(strstr(str,parola))
       {
         sql[0]=0;
         str[0]=0;

         sprintf(sql,"SELECT type FROM users WHERE username='%s' AND password='%s';",nume,parola);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);
         if(strstr(str,admin_type)) {
           write(th.cl,"Admin logged in!",sizeof("Admin logged in!"));
           admin_logon = 1;
         }
         else write(th.cl,"Logged in!",sizeof("Logged in!"));
       }
       else write(th.cl,"Wrong password!",sizeof("Wrong password!"));
     }
     else write(th.cl,"Inexistent username!",sizeof("Inexistent username!"));

     }
   }
     else if(strcmp(received,"register") == 0){
     strcpy(sent,"register");
     write(th.cl,&sent,sizeof(sent));

     read(th.cl,&received,sizeof(received));

     if(strcmp(received,"Why")==0)
     {
       write(th.cl,"Already logged in!",sizeof("Already logged in!"));
     }
     else {

     read(th.cl,&nume,sizeof(nume));
     read(th.cl,&parola,sizeof(parola));
     read(th.cl,&user_type,sizeof(nume));
     read(th.cl,&email,sizeof(email));

     sql[0]=0;
     str[0]=0;

     sprintf(sql,"INSERT INTO users (username,password,type,email) VALUES ('%s','%s','%s','%s');",nume,parola,user_type,email);
     rc = sqlite3_exec(database,sql,callback,str,&errmsg);
     write(th.cl,"Registered successfully!",sizeof("Registered successfully!"));
     }
   }
     else if(strcmp(received,"all_commands") == 0){
     write(th.cl,"show_all",sizeof("show_all"));
     }
     else if(strcmp(received,"show_champs") == 0){
     write(th.cl,"active_champ",sizeof("active_champs"));
     }
     else if(strcmp(received,"join_champ") == 0){
     write(th.cl,"join_champ",sizeof("join_champ"));
     }
     else if(strcmp(received,"create_champ") == 0){
     write(th.cl,"create_champ",sizeof("create_champ"));
     }
     else if(strcmp(received,"postpone") == 0){
     write(th.cl,"postpone",sizeof("postpone"));
     }
     else if(strcmp(received,"history") == 0){
     write(th.cl,"round_history",sizeof("round_history"));
     }
     else { write(th.cl,"No commands",sizeof("No commands")); }

  }

 sqlite3_close(database);
}

static void *Client_to_thread(void *arg)
{
  struct threadData th;
  th = *((struct threadData*)arg);
  printf("Se asteapta comenzile clientului in thread-ul %d.\n",th.idThread);
  fflush(stdout);

  pthread_detach(pthread_self());
  Client_Commands((struct threadData*)arg);
  close((intptr_t)arg);
  return(NULL);
}

int main()
{
  struct sockaddr_in server;
  struct sockaddr_in from;
  int sd;
  int pid;
  pthread_t threads[100];
  int i=0;

  if((sd = socket(AF_INET,SOCK_STREAM,0)) == -1)
  {
   perror("Erroare la socket in server.\n");
   return errno;
  }

  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  bzero(&server,sizeof(server));
  bzero(&from,sizeof(from));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port=htons(PORT);

  if(bind(sd,(struct sockaddr*)&server,sizeof(struct sockaddr))==-1)
  {
   perror("Erroare la bind in server.\n");
   return errno;
  }

  if(listen(sd,2) == -1)
  {
   perror("Erroare la listen.\n");
   return errno;
  }

  while(1)
  {
   int client;
   threadData * th;
   int length = sizeof(from);

   printf("Se asteapta la portul %d un client...\n",PORT);
   fflush(stdout);

   if( (client = accept(sd,(struct sockaddr *)&from,&length))<0)
   {
    perror("Erroare la accept in server.\n");
    continue;
   }

   th = (struct threadData*)malloc(sizeof(struct threadData));
   th->idThread = i++;
   th->cl = client;

   pthread_create(&threads[i],NULL,&Client_to_thread,th);

  }
//return 0;
};
