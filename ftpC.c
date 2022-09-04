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
#include<sys/stat.h>
#include <stdbool.h>

#define MAXLINE 1000
#define FILE_COUNT_LIMIT 100
#define USER_INPUT_MAX 200
#define BUFFER_SIZE 4
#define MAX_DIR_SIZE 100

int argument_separator(char **list,char *str,const char *delim){
    /*This function breaks each char array str into several tokens,
     which are separated by any character in the argument delim.
    */
	int i=0;
    //printf("Arguments:\n");
    char* token;
    token = strtok(str, delim);
    list[i] = token;
   
    /* walk through other tokens */
    while( token != NULL ) {
        i++;
        //printf( "-%s-\n", token );
        
        token = strtok(NULL, delim);
        list[i] = token;
    }

    //printf("%d\n",i);
	return i;
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
void files_in_dir(int sock)
{

    send(sock,"dir ",4,0); 
    printf("\nContents of the server directory : \n");
    char buffer[MAX_DIR_SIZE];
    char dir_names[MAX_DIR_SIZE];
    int ind=0;
    int flag=0;  
    while(1)
    {
        
        int valread = read(sock,buffer,MAX_DIR_SIZE);
        //printf("%s ",buffer);
        for(int i=0;i<valread;i++)
        {
            if(buffer[i]=='\0'&&ind==0)
            {
                flag=1;
                break;
            }
            else if(buffer[i]=='\0')
            {
                dir_names[ind]='\0'; 
                printf("-- %s\n",dir_names);  
                ind=0;
                memset(dir_names,0,MAX_DIR_SIZE);
            }
            else
            {
                dir_names[ind]=buffer[i];
                ind++;
            }
        }
        if(flag==1)
            break;
        
    }
    
}	
void get_file(int sock,char current_dir[],char remote_file[],char local_file[])
{
    //get implementation
    struct stat st;
    int fd = open(local_file,O_WRONLY|O_CREAT);
    if( fd == -1)
    {
        printf("\nError executing command - Cannot open the local file for writing\n");
        close(fd);
        return;
    }
    else
    {
        //send(sock,remote_file,strlen(remote_file),0);
        char code[4]={0};
        int valread = read(sock,code,4);
        //printf("%s \n",code);
        if(strcmp(code,"500")==0)
        {
            printf("\nError executing command - Remote file doesn't exist in the server.\n");
            return;
        }
        else 
        {
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
            printf("\nCommand executed successfully.(get)");

        }
        close(fd);

    }
}
void put_file(int new_socket,char current_dir[],char remote_file[],char local_file[],char user_input[])
{
    //put implementation
    //printf("%s\n",remote_file);

    if(access( local_file, F_OK|R_OK ) == -1)
    {
		printf("Error executing command - Local file does not exist or does not have reading permission \n");
        return;
    }
    else
    {
        send(new_socket,user_input,USER_INPUT_MAX,0);
        char server_response[4];
        read(new_socket,server_response,4);
        if(strcmp(server_response,"500")==0)
        {
            printf("Error executing command - Server unable to open the remote file \n");
            return;
        }
        printf("\nCommand executed successfully(put)");
		int fd = open(local_file,O_RDONLY);
		struct stat sb;
		if (stat(local_file, &sb) == -1) 
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
int main() {
    char user_input[USER_INPUT_MAX] = "",server_response[BUFFER_SIZE];
    char copy_input[USER_INPUT_MAX] = "";
    char current_dir[MAXLINE];
    bool exit_command = false;
    int socket_status=0,num_args_recd,valread;
    size_t input_size = USER_INPUT_MAX;
    const char delimiter[5] = " ,\n\0";
    //const char cmduser[5]="user",cmdpass[5]="pass",cmdopen[5]="open";
    //char* args[FILE_COUNT_LIMIT],args1[FILE_COUNT_LIMIT];

    char** args = (char**)malloc(FILE_COUNT_LIMIT*sizeof(char *));

    getcwd(current_dir,MAXLINE);

    int sockfd;
    struct sockaddr_in servaddr; 

    //printf("FTP communication\n\n");

    while(!exit_command){
        printf("\nmyFTP> ");
        
        char * str = user_input;
        char ** inp_dpoint = &str;
        //memset(user_input,0,USER_INPUT_MAX);
        size_t characters = getline(inp_dpoint,&input_size,stdin);
        //printf("\nBEFORE TOKENIZATION %s",user_input);
        strcpy(copy_input,user_input);
        num_args_recd = argument_separator(args,strdup(copy_input),delimiter);

        //user_input[(int)strlen(user_input)]='\0';
        //printf("AFTER DEFINITION - %s",user_input);
        
        if(socket_status==0 && strcmp(args[0],"open")!=0){
            printf("\nNo TCP connection exists. First command must be 'open'.");
            continue;
        }

        else if(socket_status==0 && strcmp(args[0],"open")==0){
            
            if(num_args_recd!=3){
                printf("\nERROR(Invalid command format)");
                continue;
            }

            int port = atoi(args[2]);
            if(port<20000 || port > 65535){
                printf("\nERROR(Port number is outside of range)");
                continue;
            }

            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                printf("\n ERROR(Socket creation error) ");
                return -1;
            }
        
            memset(&servaddr, 0, sizeof(servaddr)); 
            
            // Server information
            servaddr.sin_family = AF_INET;
            
            servaddr.sin_port = htons(port);

            if(inet_pton(AF_INET, args[1], &servaddr.sin_addr)<=0)
            {
                printf("\nERROR(Invalid address/ Address not supported) ");
                continue;
            }

            if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                printf("\nERROR(Connection Failed) ");
                continue;
            }
            else
            {
                printf("Connection with Server Established. ");
                socket_status = 1;
                continue;
            }
            
        }

        else if(socket_status==1 && !strcmp(args[0],"lcd")){
            if(num_args_recd!=2){
                printf("\nERROR(Invalid command format)");
                continue;
            }

            if(chdir(args[1])>-1){
                strcpy(current_dir,args[1]);
                printf("\nDirectory changed successfully");
            }
            else{
                printf("\nERROR(Invalid directory name)");
            }
            //"/home/ayush/Desktop/Networks Lab/Assn 3"
            //getcwd(current_dir,MAXLINE);

        }
        else if(socket_status==1 && !strcmp(args[0],"quit")){
            if(num_args_recd==1){
                exit_command = true;
                close(sockfd);
                socket_status = 0;
                exit(0);
            }
            else{
                printf("\nERROR(Invalid command format)");
            }
        }
        else if(!strcmp(args[0],"user")){
            if(num_args_recd!=2){
                printf("\nERROR(Invalid command format)");
                continue;
            }
            //printf("\nSending MESSAGE: %s\n",user_input);
            memset(server_response,0,BUFFER_SIZE);
            send(sockfd, user_input, USER_INPUT_MAX, 0);
            valread = read( sockfd , server_response, 4);
            
            /*printf("%s\n",server_response);
            for(int t=0;t<strlen(server_response);t++){
                printf("*%c*",server_response[t]);
            }
            printf("\n");
            printf("\nVal of %d\n",strcmp(server_response,"200"));*/
            if(server_response[0]=='2'&&server_response[1]=='0'&&server_response[2]=='0'){
                printf("\nCommand Executed Successfully - Username accepted.");
                
            }
            else if(server_response[0]=='5'&&server_response[1]=='0'&&server_response[2]=='0'){
                printf("\nError executing command - Username not accepted.");
            }
            else if(server_response[0]=='6'&&server_response[1]=='0'&&server_response[2]=='0'){
                printf("\nError executing command - User Command not sent in order.");
            }
            else{
                printf("\nError executing command");
            }

        }

        else if(!strcmp(args[0],"pass")){
            if(num_args_recd!=2){
                printf("Error executing command - Invalid command format.\n");
                continue;
            }
            //printf("\nSending MESSAGE: %s\n",user_input);
            send(sockfd, user_input, USER_INPUT_MAX, 0);
            valread = read( sockfd , server_response, 4);
            
            /*printf("\n");
            for(int t=0;t<strlen(server_response);t++){
                printf("*%c*",server_response[t]);
            }
            printf("\n");
            printf("\nVal of %d\n",strcmp(server_response,"200"));*/
            if(server_response[0]=='2'&&server_response[1]=='0'&&server_response[2]=='0'){
                printf("\nCommand Executed Successfully - Password accepted.");
                
            }
            else if(server_response[0]=='5'&&server_response[1]=='0'&&server_response[2]=='0'){
                printf("\nError executing command - Password not accepted.");
            }
            else if(server_response[0]=='6'&&server_response[1]=='0'&&server_response[2]=='0'){
                printf("\nError executing command - Password Command not sent in order.");
            }
            else{
                printf("\nError executing command(pass)");
            }

        }
        else if(!strcmp(args[0],"dir"))
        {
            files_in_dir(sockfd);

        }
        else if(!strcmp(args[0],"cd"))
        {
            if(num_args_recd!=2)
            {
                printf("Invalid command Format\n");
                continue;
            }
            else
            {
                memset(server_response,0,4);
                send(sockfd,user_input,USER_INPUT_MAX,0);
                int valread = read(sockfd,server_response,4);
                if(server_response[0]=='2')
                    printf("Server Directory Changed successfully. \n");
                else
                    printf("Not able to change server directory :( \n");
            }
            
        }
        else if(!strcmp(args[0],"get"))
        {
            if(num_args_recd!=3)
            {
                printf("Invalid command format\n");
            }
            else
            {
                memset(server_response,0,4);
                send(sockfd,user_input,USER_INPUT_MAX,0);
                get_file(sockfd,current_dir,args[1],args[2]);
            }

        }
        else if(!strcmp(args[0],"put"))
        {
            if(num_args_recd!=3)
            {
                printf("Invalid Command Format\n");
            }
            else
            {
                //memset(server_response,0,4);
                //send(sockfd,user_input,USER_INPUT_MAX,0);
                put_file(sockfd,current_dir,args[2],args[1],user_input);
                /*int valread = read(sockfd,server_response,4);
                if(server_response[0]=='2')
                    printf("Command Executed Successfully - Remote File(s) opened \n");
                else if(server_response[0]=='5')
                    printf("Error executing command - Unable to open Remote file \n");*/
            }
        }
        else if(!strcmp(args[0],"mget"))
        {
            if(num_args_recd<2)
            {
                printf("Invalid Command Format\n");
            }
            else
            {
                for(int i=1;i<num_args_recd;i++)
                {
                    char cmd_send[USER_INPUT_MAX];
                    sprintf(cmd_send,"get %s %s",args[i],args[i]);
                    send(sockfd,cmd_send,USER_INPUT_MAX,0);
                    get_file(sockfd,current_dir,args[i],args[i]);

                }


            }
        }
        else if(!strcmp(args[0],"mput"))
        {
            if(num_args_recd<2)
            {
                printf("Invalid Command Format\n");
            }
            else
            {
                for(int i=1;i<num_args_recd;i++)
                {
                    char cmd_send[USER_INPUT_MAX];
                    sprintf(cmd_send,"put %s %s",args[i],args[i]);
                    put_file(sockfd,current_dir,args[i],args[i],cmd_send);

                }


            }
        }
        else if(!strcmp(args[0],"quit"))
        {
            close(sockfd);
            exit(1);
        }
        else{
            printf("\n-----Command not interpreted------\n");
        }

    }




     
   
	return 0;
} 