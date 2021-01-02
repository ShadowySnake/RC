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
  char ban[]="true";
  char not_ban[]="false";
  int admin_logon = 0;

  char mode1[]="Single Elimination";
  char mode2[]="Double Elimination";

  // se verifica daca loginul este de tip administrator
  // in caz contrar se presupune ca userul logat este unul normal

  char *errmsg;
  int rc = sqlite3_open("database",&database);
  struct threadData th;
  th = *((struct threadData*)arg);
  char str[10000];
  char sql[10000];

  while(1){

     if(read(th.cl,&received,sizeof(received)) == -1)
     {
      printf("Eroare la read de la client, in thread-ul %d.\n",th.idThread);
    // perror("Eroare la read de la client.");
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

         read(th.cl,&received,sizeof(received));
         if(strcmp(received,"Ban?") != 0) write(th.cl,"Error",sizeof("Error"));
         else {
           sql[0]=0;
           str[0]=0;

           sprintf(sql,"SELECT banned FROM users WHERE username='%s' AND password='%s';",nume,parola);
           rc = sqlite3_exec(database,sql,callback,str,&errmsg);
           if(strstr(str,ban)) write(th.cl,"Account banned!",sizeof("Account banned!"));
           else write(th.cl,"Continue",sizeof("Continue"));
         }
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
     read(th.cl,&user_type,sizeof(user_type));
     read(th.cl,&email,sizeof(email));

     if( (strcmp(user_type,"admin") != 0) || (strcmp(user_type,"normal") != 0) ) {
       write(th.cl,"The user type must be 'admin' or 'normal'!",sizeof("The user type must be 'admin' or 'normal'!"));
     }
     else {
     sql[0]=0;
     str[0]=0;
     sprintf(sql,"SELECT username FROM users WHERE username='%s';",nume);
      rc = sqlite3_exec(database,sql,callback,str,&errmsg);

     if(strstr(str,nume)) write(th.cl,"User already exists!",sizeof("User already exists"));
     else {
     sql[0]=0;
     str[0]=0;

     sprintf(sql,"INSERT INTO users (username,password,type,email,banned) VALUES ('%s','%s','%s','%s','%s');"
     ,nume,parola,user_type,email,not_ban);
     rc = sqlite3_exec(database,sql,callback,str,&errmsg);

     write(th.cl,"Registered successfully!",sizeof("Registered successfully!")); }
     }
   }
 }

     else if(strcmp(received,"all_commands") == 0){
     write(th.cl,"show_all",sizeof("show_all"));
     }

     else if(strcmp(received,"show_champs") == 0){
     write(th.cl,"active_champ",sizeof("active_champ"));

     read(th.cl,&received,sizeof(received));

     if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
     else {
       sql[0]=0;
       str[0]=0;

       sprintf(sql,"SELECT * FROM championship;");
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);
       char output_backup[10000];
       sprintf(output_backup,"%s",str);
       write(th.cl,output_backup,sizeof(output_backup));
     }
   }

     else if(strcmp(received,"join_champ") == 0){
     write(th.cl,"join_champ",sizeof("join_champ"));

     read(th.cl,&received,sizeof(received));
     if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
     else if(admin_logon == 1)  write(th.cl,"Must be normal user!",sizeof("Must be normal user!"));
     else{
       sql[0] = 0;
       str[0] = 0;

       char nume_champ[100];
       read(th.cl,&nume_champ,sizeof(nume_champ));

       sprintf(sql,"SELECT COUNT(*) FROM registered WHERE championship='%s';",nume_champ);
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);
       char output1[100],output2[100];
       strcpy(output1,str+11);
       strncpy(output1,output1,1);
       //printf("%s\n",output1);

       sql[0] = 0;
       str[0] = 0;
       sprintf(sql,"SELECT maxplayers FROM championship WHERE name='%s';",nume_champ);
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);
       strcpy(output2,str+13);
       strncpy(output2,output2,1);
       //printf("%s",output2);

       sql[0] = 0;
       str[0] = 0;
       sprintf(sql,"SELECT championship FROM registered WHERE username='%s' AND championship='%s';",nume,nume_champ);
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);

       if(strstr(output2,output1)) write(th.cl,"Too many people registered!",sizeof("Too many people registered!"));
       else if(strstr(str,nume_champ)) { write(th.cl,"Already registered to that one!",sizeof("Already registered to that one!")); }
       else {
         sql[0] = 0;
         str[0] = 0;
         sprintf(sql,"INSERT INTO registered VALUES('%s','%s') ;",nume,nume_champ);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);
         write(th.cl,"Registered to that championship!",sizeof("Registered to that championship!"));
       }
    }
}
     else if(strcmp(received,"create_champ") == 0){
     write(th.cl,"create_champ",sizeof("create_champ"));

     read(th.cl,&received,sizeof(received));

     if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
     else if(admin_logon == 0) write(th.cl,"Must be an admin!",sizeof("Must be an admin!"));
     else {

       write(th.cl,"Good to go!",sizeof("Good to go!"));
       char nume_champ[100];
       char gametype[100];
       char players[10];
       char given_mode[10];
       char date[100];

       read(th.cl,&nume_champ,sizeof(nume_champ));
       read(th.cl,&gametype,sizeof(gametype));
       read(th.cl,&players,sizeof(players));
       read(th.cl,&given_mode,sizeof(given_mode));
       read(th.cl,&date,sizeof(date));

       sql[0]=0;
       str[0]=0;

       sprintf(sql,"SELECT name FROM championship WHERE name='%s';",nume_champ);
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);
       if(strstr(str,nume_champ)) write(th.cl,"A championship with this name alread exists!",sizeof("A championship with this name alread exists!"));
       else {
         sql[0] = 0;
         str[0] = 0;

       if(strcmp(given_mode,"single") == 0) {
         sprintf(sql,"INSERT INTO championship VALUES('%s','%s','%s','%s','%s');"
         ,nume_champ,gametype,players,mode1,date);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);
         write(th.cl,"Championship registered!",sizeof("Championship registered!"));
       }
       else if(strcmp(given_mode,"double") == 0){
         sprintf(sql,"INSERT INTO championship VALUES('%s','%s','%s','%s','%s');"
         ,nume_champ,gametype,players,mode2,date);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);
         write(th.cl,"Championship registered!",sizeof("Championship registered!"));
       }
       else write(th.cl,"No such mode exists!",sizeof("No such mode exists!"));
     }
   }
 }

     else if(strcmp(received,"postpone") == 0){
     write(th.cl,"postpone",sizeof("postpone"));

     read(th.cl,&received,sizeof(received));
     if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
     else if(admin_logon == 1)  write(th.cl,"Must be normal user!",sizeof("Must be normal user!"));
     else{
       write(th.cl,"Go on!",sizeof("Go on!"));

       char time[100];
       char nume_champ[100];

       read(th.cl,&nume_champ,sizeof(nume_champ));
       read(th.cl,&time,sizeof(time));

       sql[0]=0;
       str[0]=0;

       sprintf(sql,"UPDATE rounds SET time='%s' WHERE championship='%s' AND ( player1='%s' OR player2='%s');",
       time,nume_champ,nume,nume);
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);

       write(th.cl,"Posptone success!",sizeof("Postpone success!"));
     }
    }

     else if(strcmp(received,"history") == 0){
     write(th.cl,"round_history",sizeof("round_history"));

     read(th.cl,&received,sizeof(received));
     if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
     else if(admin_logon == 0)  write(th.cl,"Must be an admin!",sizeof("Must be an admin!"));
     else{
       sql[0]=0;
       str[0]=0;

       sprintf(sql,"SELECT * FROM rounds;");
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);
       write(th.cl,str,sizeof(str));
     }
    }
     else if(strcmp(received,"delete_account") == 0)
     {
       write(th.cl,"delete_account",sizeof("delete_account"));

       read(th.cl,&received,sizeof(received));
       if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
       else {
         sql[0]=0;
         str[0]=0;

         sprintf(sql,"DELETE FROM users WHERE username='%s';",nume);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);

         sql[0]=0;
         str[0]=0;

         sprintf(sql,"DELETE FROM registered WHERE username='%s';",nume);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);

         sql[0]=0;
         str[0]=0;

         sprintf(sql,"DELETE FROM rounds WHERE player1='%s'OR player2='%s';",nume,nume);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);

         write(th.cl,"Account has been deleted!",sizeof("Account has been deleted!"));
       }
     }
     else if(strcmp(received,"ban_account") == 0){
     write(th.cl,"ban_account",sizeof("ban_account"));

     read(th.cl,&received,sizeof(received));
     if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
     else if(admin_logon == 0)  write(th.cl,"Must be an admin!",sizeof("Must be an admin!"));
     else {
       write(th.cl,"Go on!",sizeof("Go on!"));
       char user_to_ban[100];

       read(th.cl,&user_to_ban,sizeof(user_to_ban));

       sql[0]=0;
       str[0]=0;

       sprintf(sql,"SELECT username FROM users WHERE username='%s';",user_to_ban);
       rc = sqlite3_exec(database,sql,callback,str,&errmsg);
       if(strstr(str,user_to_ban)){
         sql[0]=0;
         str[0]=0;

         sprintf(sql,"SELECT banned FROM users WHERE username='%s';",user_to_ban);
         rc = sqlite3_exec(database,sql,callback,str,&errmsg);
         if(strstr(str,ban)) write(th.cl,"User already banned!",sizeof("User already banned!"));
         else {
           sql[0]=0;
           str[0]=0;

           sprintf(sql,"UPDATE users SET banned='%s' WHERE username='%s'","true",user_to_ban);
           rc = sqlite3_exec(database,sql,callback,str,&errmsg);
           write(th.cl,"Successfull ban!",sizeof("Successfull ban!"));
         }
       }
       else write(th.cl,"User inexistent! Cannot ban someone who doesn't exist!",sizeof("User inexistent! Cannot ban someone who doesn't exist!"));
     }
    }
    else if(strcmp(received,"unban_account") == 0){
    write(th.cl,"unban_account",sizeof("unban_account"));

    read(th.cl,&received,sizeof(received));
    if(strcmp(received,"No") == 0) write(th.cl,"Log in first!",sizeof("Log in first!"));
    else if(admin_logon == 0)  write(th.cl,"Must be an admin!",sizeof("Must be an admin!"));
    else {
      write(th.cl,"Go on!",sizeof("Go on!"));
      char user_to_ban[100];

      read(th.cl,&user_to_ban,sizeof(user_to_ban));

      sql[0]=0;
      str[0]=0;

      sprintf(sql,"SELECT username FROM users WHERE username='%s';",user_to_ban);
      rc = sqlite3_exec(database,sql,callback,str,&errmsg);
      if(strstr(str,user_to_ban)){
        sql[0]=0;
        str[0]=0;

        sprintf(sql,"SELECT banned FROM users WHERE username='%s';",user_to_ban);
        rc = sqlite3_exec(database,sql,callback,str,&errmsg);
        if(strstr(str,not_ban)) write(th.cl,"User already unbanned!",sizeof("User already unbanned!"));
        else {
          sql[0]=0;
          str[0]=0;

          sprintf(sql,"UPDATE users SET banned='%s' WHERE username='%s'","false",user_to_ban);
          rc = sqlite3_exec(database,sql,callback,str,&errmsg);
          write(th.cl,"Successfull unban!",sizeof("Successfull unban!"));
        }
      }
      else write(th.cl,"User inexistent! Cannot unban someone who doesn't exist!",sizeof("User inexistent! Cannot unban someone who doesn't exist!"));
    }
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
