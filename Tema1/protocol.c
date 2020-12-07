#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/dir.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>


#define MSG1 "Successfull log in!"
#define MSG2 "Unable to log in, username or password incorect!"

void searching(char *dirname,char file_name[50],int* found) {
  DIR *dir;
  struct dirent *dirp;
  dir = opendir(dirname);
  
 while((dirp = readdir(dir))!=NULL){
   if( strcmp(dirp->d_name,file_name) == 0 )
      { printf("Am gasit un fisier cu numele cerut.\n");
        printf("The path is: %s/%s\n",dirname,file_name);
        *found = 1;
        char perm[10];
        struct stat filestat;
        stat(dirp->d_name,&filestat);
        printf("Dimensiunea fisierului: %lld octeti\n", (long long) filestat.st_size);
        printf("Permisiuni in octal: %o\n", filestat.st_mode & 0777);
        printf("Last access time: %s",ctime(&filestat.st_atime));
        printf("Last modification time: %s", ctime(&filestat.st_mtime));
        printf("Last stat change: %s",ctime(&filestat.st_ctime));
      }
   }
} 

void loging_in()
{
  int pid;
  int fd1;
  
  mkfifo("log_fifo",0666);
  pid = fork();
  
  if( pid == 0 )
  { // child
    fd1 = open("log_fifo",O_RDONLY);
    char nume[15];
    char pass[10];
    char concat[25];
    char buffer[25];
    int found = 0;
    read(fd1,&nume,sizeof(nume));
    read(fd1,&pass,sizeof(pass));
    close(fd1);
    strcpy(concat,nume);
    strcat(concat,pass);
    
    FILE *file; 
    file = fopen("./names.txt","r");
    
    while(!feof(file))
    {
      fscanf(file,"%s",buffer);
      if(strcmp(buffer,concat) == 0)
      {
       found = 1;
       break;
      }
    }
   if(found == 1) printf("%s\n",MSG1);
   else printf("%s\n",MSG2);
   exit(0); 
  } else {
  // parent
  fd1 = open("log_fifo",O_WRONLY);
  char nume[15];
  char pass[10];
  char message[100];
  printf("Username: ");
  scanf("%s",nume);
  write(fd1,&nume,sizeof(nume));
  printf("Password: ");
  scanf("%s",pass);
  write(fd1,&pass,sizeof(pass));
  close(fd1);
  wait(NULL);
  }
}

void quiting()
{
 int p1[2],p2[2];
  
  if( (pipe(p1) == -1 )|| (pipe(p2) == -1) )
   {
     perror("ERROR AT CREATING PIPE.\n");
     exit(1);
   }
   int pid = fork();
   if( pid < 0 )
   {
     perror("ERROR AT FORK.\n");
     exit(1);
   }
   
   if(pid == 0)
   { // in child
     close(p1[0]);
     close(p2[1]);
     char str[30];
     int signal;
     read(p2[0],str,sizeof(str));
     if(strcmp(str,"quit") == 0) signal = 1;
      else signal = 0;
     write(p1[1],&signal,sizeof(int));
     close(p1[1]);
     close(p2[0]);
   }
   
   else {
   close(p1[1]);
   close(p2[0]);
   char str[30];
   int signal;
   write(p2[1],"quit",sizeof("quit"));
   read(p1[0],&signal,sizeof(int));
   if( signal == 1 ) {
    printf("Programul se va inchide...\n");
   } else {
   printf("A avut loc o erroare pe drum\n");
   printf("Attempting to force-quit the program\n");
   exit(1);
   }
   close(p1[0]);
   close(p2[1]);
   }  
  exit(0); 
}

void socket_find()
{ 
    int sockp[2], child; 
    char filename[50],dirname[50];
    char end_message[50];
    int printed = 0;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0) 
      { 
        perror("Err... socketpair"); 
        exit(1); 
      }

    if ((child = fork()) == -1) perror("Err...fork"); 
    else 
      if (child)   //parinte 
        {  
            close(sockp[1]); 
            read(sockp[0], filename, sizeof(filename));
            printf("Dati calea directorului: ");
            scanf("%s",dirname);
            searching(dirname,filename,&printed);
            if( printed == 0 ) write(sockp[0], "No such file found!", sizeof("Nu such file found!"));
            else write(sockp[0],"DONE!",sizeof("DONE!"));
            close(sockp[0]);
            wait(NULL);
          } 
        else     //copil
          { 
            printf("Dati nume fisierului pe care sa-l cautam impreuna cu extensia sa: ");
            scanf("%s",filename);
            close(sockp[0]); 
            write(sockp[1], filename, sizeof(filename));
            read(sockp[1], end_message, sizeof(end_message));
            printf("%s\n\n",end_message);
            close(sockp[1]);
            exit(0);
           } 
} 

void status()
{
 int pid;
 int fifo;
 mkfifo("status_fifo",0666);
 pid = fork();
 
 if(pid == 0) { //copil
 fifo = open("status_fifo",O_RDONLY);
 char attributes[10];
 printf("Atributele fisierului: ");
 read(fifo,&attributes,sizeof("read"));
 if(strcmp(attributes,"read") == 0) printf("%s---",attributes);
   else printf("unable---");
 read(fifo,&attributes,sizeof("write"));
 if(strcmp(attributes,"write") == 0) printf("%s---",attributes);
   else printf("unable---");
 read(fifo,&attributes,sizeof("execute"));
 if(strcmp(attributes,"execute") == 0) printf("%s\n",attributes);
   else printf("unable\n");
 close(fifo);
 exit(0);
 } else { // parinte
 fifo = open("status_fifo",O_WRONLY);
 char path[100];
 printf("Dati calea catre fisier: ");
 scanf("%s",path);
 struct stat filestat;
    stat(path,&filestat);
 if( filestat.st_mode & R_OK ) write(fifo,"read",sizeof("read"));
 if( filestat.st_mode & W_OK ) write(fifo,"write",sizeof("write"));
 if( filestat.st_mode & X_OK ) write(fifo,"execute",sizeof("execute"));
 close(fifo);
 wait(NULL);
 }
}

int main()
{
int printed = 0;
  while(1)
  {
    char command[50];
    printf("Your command: ");
    scanf("%s",command);
    
    if( strcmp(command,"quit") == 0 ) quiting();
    else if( strcmp(command,"login") == 0 ) loging_in();
         else if( strcmp(command,"mystat") == 0)  status(); 
              else if( strcmp(command,"myfind") == 0)  socket_find();
                   else printf("There is no such command!\n");
  }
 
}
