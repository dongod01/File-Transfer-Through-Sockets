/*
Assignment 3: File Transfer Protocol
By-
Ayush Pattnayak, 19CS10014
Pramit Chandra, 19CS10072
*/

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>

#define PORT 20008
#define MAXLINE 1000

#define FILE_COUNT_LIMIT 100
#define USER_COMMAND_MAX 200
#define BUFFER_SIZE 4
#define MAX_FILE_SIZE 500
#define MAX_DIR_SIZE 1000

int argument_separator(char **list,char *str,const char *delim){
    /*This function breaks each char array str into several tokens,
     which are separated by any character in the argument delim.
    */
	int i=0;
    //printf("Arguments:\n");
    char* token;
    token = strtok(strdup(str), delim);

    list[i] = token;
    //Debugging statements to print each token
    //printf( "-%s-\n", token );
    //printf(" list[%d] = %s \n",i,list[i]);

    while( token != NULL ) {
        i++;
        
        token = strtok(NULL, delim);
        if(token!=NULL){
            //Debugging statements
            //printf( "-%s-\n", token );
            list[i] = token;

            //printf(" listelem[%d] = %s \n",i,list[i]);
        }
       
    }

    //printf("%d\n",i);
	return i;                   //Return number of tokens we recieve after separating from delimiters
}
int calculate_size(char arr[],int l,int r)
{
    // convert binary to decimal within indices l to r
    int s=0;
    for(int i=l;i<=r;i++)
    {
        s=s*2;
        s+= arr[i]-'0';
    }
    return s;
}
void strtok_alt(char list[][200],char str[200],char first[200],char delimiter){
    //Separate function emulating strtok but only giving first token
    int j=0;
    for(int i=0;i<strlen(str);i++){
        if(str[i]==delimiter){
            first[i]='\0';
            break;
        }
        first[i] = str[i];
    }


}
void convert_int_to_chararr(int x,char bytes[])
{
    //Converts and stores binary form of int in char array
	bytes[32]='\0';//int is of 32 bits followed by a null character
	for(int i=0;i<32;i++)
	{
		if(x&1)
		{
			bytes[31-i]='1';
		}
		else
		{
			bytes[31-i]='0';
		}
		x=x>>1;
	}

}
void convert_short_to_chararr(int x,char bytes[])
{
    //Converts and stores binary form of short in char array
	bytes[16]='\0';//short is of 16 bits followed by a null character
	for(int i=0;i<16;i++)
	{
		if(x&1)
		{
			bytes[15-i]='1';
		}
		else
		{
			bytes[15-i]='0';
		}
		x=x>>1;
	}

}
void get_dir(int new_socket,char current_dir[])
{
	//char buffer[MAX_DIR_SIZE];
	//int valread= read(new_socket,buffer,MAX_DIR_SIZE);
	//printf("%s\n",buffer);
	//printf("Reached\n");
    DIR *d;
    struct dirent *dir;
    d = opendir(current_dir);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            //printf("%s\n", dir->d_name);
			char dirname[MAX_DIR_SIZE];
			strcpy(dirname,dir->d_name);
			send(new_socket,dirname,strlen(dirname)+1,0);
        }
        closedir(d);
    }
	//printf("End of dir\n");
	send(new_socket,"\0",1,0);
    return;

}
void get_file(int new_socket,char remote_file[])
{
    //printf("%s\n",remote_file);

    if(access( remote_file, F_OK|R_OK ) == -1)
    {
		printf("\nError executing command - Remote file does not exist or does not have reading permission");
        send(new_socket,"500", 3, 0);
        return;
    }
    else
    {
		send(new_socket,"200", 3, 0);
		int fd = open(remote_file,O_RDONLY);
		struct stat sb;
		if (stat(remote_file, &sb) == -1) 
		{
			perror("stat");
			exit(EXIT_FAILURE);
		}

		int* file_contents = malloc(sb.st_size);
		read(fd, file_contents, sb.st_size);

		/*for(int i=0;i<sb.st_size/4;i++)
			printf("%d ",file_contents[i]);*/
		
		//printf("%d %d",sb.st_size,sizeof(file_contents));
		int bytes_sent = 0;
		int bytes_rem = sb.st_size;
		int ind=0;
		while(bytes_rem>400) // We are sending 3200 bits at one go
		{
			char buffer[3218]={0};
			buffer[0]='M';
			buffer[1]='\0';
			bytes_sent+=400;
			bytes_rem-=400;
			char size_binver[17];
			convert_short_to_chararr(400,size_binver);
			strcat(buffer,size_binver);
			for(int i=0;i<100;i++)
			{
				char int_binver[33];
				convert_int_to_chararr(file_contents[ind],int_binver);
				strcat(buffer,int_binver);
				ind++;
			}
			//printf("%s\n",buffer);
			send(new_socket,buffer,3218,0);
		}
		//Send the last part of the file
		char buffer[3218]={0};
		buffer[0]='L';
		buffer[1]='\0';
		char size_binver[17];
		convert_short_to_chararr(bytes_rem,size_binver);
		strcat(buffer,size_binver);
		for(int i=0;i<100;i++)
		{
			char int_binver[33];
			convert_int_to_chararr(file_contents[ind],int_binver);
			strcat(buffer,int_binver);
			ind++;
		}
		//printf("%s\n",buffer);
		send(new_socket,buffer,strlen(buffer),0);
		
		close(fd);
  
    }
   
}
void put_file(int sock,char remote_file[])
{
    struct stat st;
    int fd = open(remote_file,O_WRONLY|O_CREAT);
    if( fd == -1)
    {
        send(sock,"500",3,0);
        printf("\nERROR(Cannot open the local file for writing)");
        close(fd);
        return;
    }
    else
    {
        //send(sock,remote_file,strlen(remote_file),0);
        send(sock,"200",3,0);
        int loop_times=0;
        while(1)
        {
            if(loop_times>1000)
                break;
            char buffer[3218]={0};
            int valread = read(sock,buffer,3218);
            int no_of_bytes = calculate_size(buffer,1,16);
            int* data_write = malloc(no_of_bytes);
            int ind=0;
            
            for(int i=17;i<17+8*no_of_bytes;i+=32)
            {
                data_write[ind] = calculate_size(buffer,i,i+31);
                ind++;
            }
            /*for(int i=0;i<no_of_bytes/4;i++)
                printf("%d ",data_write[i]);*/
            write(fd,data_write,no_of_bytes);
            if(buffer[0]=='L')
                break;
            loop_times++;
        }

        
        close(fd);

    }
}
int main(){

	int sockfd, ret;
	struct sockaddr_in serverAddr;

	int newSocket;
	struct sockaddr_in newAddr;

	socklen_t addr_size;

	pid_t childpid;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Server Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(ret < 0){
		printf("[-]Binding Error.(Change port number if encountered)\n");
        //Closing server before client can cause this issue.
		exit(1);
    }

	if(listen(sockfd, 10) == 0){
		printf("[+]Listening....\n");
	}else{
		printf("[-]Error in binding.(Change port number if encountered)\n");
	}


	while(1){
		newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
		if(newSocket < 0){
			exit(1);
		}
		printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

		if((childpid = fork()) == 0){
			close(sockfd);

            char user_command[USER_COMMAND_MAX] = "";
            char copy_string[USER_COMMAND_MAX]="";
            char server_response[BUFFER_SIZE];
            int login_status=1,num_args_recd,valread;
            const char delimiter[4] = " ,\n";
            const char code200[4] = "200",code500[4] = "500",code600[4] = "600";
            char line[MAX_FILE_SIZE],line1[MAX_FILE_SIZE];

            char** args = (char**)malloc(FILE_COUNT_LIMIT*sizeof(char *));

            char current_dir[MAXLINE];
            getcwd(current_dir,MAXLINE);

            char backup[200] = "";
            int probable_user=0;
            int first_loop = 1;

			while(1){
                if(first_loop==1){
                    printf("Server is running, please ignore the path.\n");
                    first_loop=0;
                }
                memset(user_command,0,USER_COMMAND_MAX);
				valread = read( newSocket , user_command, USER_COMMAND_MAX);
                printf("\nCommand Recd: %s ",user_command);
                strcpy(copy_string,user_command);
                num_args_recd = argument_separator(args,strdup(copy_string),delimiter);
                strcpy(backup,*args+1);
				if(login_status==1 && strcmp(args[0],"user")){
                    send(newSocket,code600,strlen(code600),0);
                    continue;
                }
                else if(login_status==1 && !strcmp(args[0],"user")){
                    //printf("\nMatching usernames");
                    FILE* file = fopen("user.txt", "r"); 

                    if(!file){
                        printf("\n Unable to open : %s ", "user.txt");
                        strcpy(server_response,"500");
                        continue;
                    }
                    
                    fgets(line, sizeof(line), file);
                    fgets(line1, sizeof(line1), file);
                    fclose(file);

                    line[strlen(line)]='\0';
                    line1[strlen(line1)]='\0';
                    //printf("\nClosed file %s",line);
                    
                    //strcpy(copy_string,line);
                    char* token;
                    
                    char first[200]="";
                    char first1[200]="";
                    //argument_separator(args,strdup(user_command)," \n");

                    //Storing usernames u1 and u2 in first and first1 respectively
                    int j=0;
                    for(int i=0;i<200;i++){
                        if(line[i]==' '){
                            first[i]='\0';
                            break;
                        }
                        first[i] = line[i];
                    }

                    j=0;
                    for(int i=0;i<200;i++){
                        if(line1[i]==' '){
                            first1[i]='\0';
                            break;
                        }
                        first1[i] = line1[i];
                    }
                    //printf("\n From user input -() From txt file :):):)");
                    

                    /*printf("\n");
                    for(int t=0;t<strlen(backup);t++){
                        if(args[1][t]==backup[t]) printf("%c",backup[t]);
                        else printf("\n %c %c\n",args[1][t],backup[t]);
                    }
                    printf("\n");

                    printf("\n");
                    for(int t=0;t<strlen(first);t++){
                        if(args[1][t]==first[t]) printf("*%c*",first[t]);
                        else printf("\n %c %c\n",args[1][t],first[t]);
                    }
                    printf("\n");*/

                    if(!strcmp(args[1],first) || !strcmp(args[1],first1)){
                        send(newSocket,code200,strlen(code200),0);
                        login_status=2;
                        if(!strcmp(args[1],first)) probable_user=1;
                        else probable_user=2;
                        //printf("Sent 200\n");
                        continue;
                    }
                    
                    else if(strcmp(args[1],first) && strcmp(args[1],first1)){
                        send(newSocket,code500,strlen(code500),0);
                        //printf("Sent 500\n");
                        continue;
                    }
                    

                }

                else if(login_status==2 && strcmp(args[0],"pass")){
                    send(newSocket,code600,strlen(code600),0);
                    continue;
                }
                else if(login_status==2 && !strcmp(args[0],"pass")){
                    //printf("\nMatching passwords");
                    FILE* file = fopen("user.txt", "r"); 

                    if(!file){
                        printf("\n Unable to open : %s ", "user.txt");
                        strcpy(server_response,"500");
                        continue;
                    }
                    
                    fgets(line, sizeof(line), file);
                    fgets(line1, sizeof(line1), file);
                    fclose(file);
                    line[strlen(line)]='\0';
                    line1[strlen(line1)]='\0';
                    //printf("\nClosed file %s",line);
                    
                    char* token;
                    
                    char first[200]="";
                    char first1[200]="";

                    //Storing passwords for u1 and u2 respectively in first and first1
                    int j=0,t=0;
                    for(int i=0;i<strlen(line);i++){
                        if(j==0 && line[i]!=' '){

                        }
                        if(j==0 && line[i]==' '){
                            j++;
                        }
                        else if(j>0 && line[i]=='\n' || line[i]==' '){
                            first[t] = '\0';
                            break;
                        }
                        else if(j>0) {
                            first[t] = line[i];
                            t++;
                        }
                    }

                    j=0,t=0;
                    for(int i=0;i<strlen(line1);i++){
                        if(j==0 && line1[i]!=' '){
                            
                        }
                        if(j==0 && line1[i]==' '){
                            j++;
                        }
                        else if(j>0 && line1[i]=='\n' || line1[i]==' '){
                            first1[t] = '\0';
                            break;
                        }
                        else if(j>0) {
                            first1[t] = line1[i];
                            t++;
                        }
                    }

                    //printf("\n From user input -() From txt file :):):)");
                    

                    /*printf("\n");
                    for(int t=0;t<strlen(backup);t++){
                        if(args[1][t]==backup[t]) printf("%c",backup[t]);
                        else printf("\n %c %c\n",args[1][t],backup[t]);
                    }
                    printf("\n");

                    printf("\n");
                    for(int t=0;t<strlen(first);t++){
                        if(args[1][t]==first[t]) printf("*%c*",first[t]);
                        else printf("\n %c %c\n",args[1][t],first[t]);
                    }
                    printf("\n");*/

                    if((!strcmp(args[1],first) && probable_user==1)|| (!strcmp(args[1],first1) &&probable_user==2)){
                        send(newSocket,code200,strlen(code200),0);
                        login_status=3;
                        //printf("Sent 200\n");
                        continue;
                    }
                    
                    else {
                        send(newSocket,code500,strlen(code500),0);
                        login_status=1;
                        probable_user=0;
                        //printf("Sent 500\n");
                        continue;
                    }
			}
            else if(login_status==3){
                //Succesfully logged in

                if(!strcmp(args[0],"cd")){
                    if(chdir(args[1])>-1){
                        strcpy(current_dir,args[1]);
                        send(newSocket,code200,strlen(code200),0);
                        printf("\nDirectory changed successfully.");
                    }
                    else{
                        send(newSocket,code500,strlen(code500),0);
                        printf("\nInvalid directory name.");
                    }
                    //"/home/ayush/Desktop/Networks Lab/Assn 3"
                    getcwd(current_dir,MAXLINE);

                }
                else if(!strcmp(args[0],"dir")){
                    get_dir(newSocket,current_dir);

                }
                else if(!strcmp(args[0],"get")){
                    if(num_args_recd!=3)
                    {
                        send(newSocket,"500",3,0);
                    }
                    else
                    {
                        get_file(newSocket,args[1]);
                    }

                }
                else if(!strcmp(args[0],"put")){
                    if(num_args_recd!=3)
                    {
                        send(newSocket,"500",3,0);
                    }
                    else
                    {
                        put_file(newSocket,args[2]);
                    }

                }

                else if((!strcmp(args[0],"user")) || (!strcmp(args[0],"pass"))){
                    printf("\nInvalid command sequence.");

                }

                //mget and mput are handled through get and put
               

            }
		}

	}

	close(newSocket);

	return 0;
}
}