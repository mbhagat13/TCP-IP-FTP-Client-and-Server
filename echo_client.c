#include <dirent.h>
#include <stdio.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/stat.h>


#define USER_INPUT_SIZE 4101 
#define ARG_SIZE 4097
#define SERVER_TCP_PORT 8080

struct pdu
{
 char type;
 int length;
 char data[ARG_SIZE];
} rpdu, tpdu;


void init_serv(int serv_sock) {
	char user_input[USER_INPUT_SIZE];
	char user_argument[ARG_SIZE];
	char command[3];
	int  i = 0, size = 0;
	char* token;
	FILE* fp;
	struct stat filestat;


	while(rpdu.type != 'R')
		read(serv_sock, &rpdu,sizeof(rpdu));
	printf("Server Ready, %c, recieved\n", rpdu.type);


	while(1){
		printf("\nPlease enter the command (cd 'dir_name', ls 'dir_name', U 'file_name', D 'file_name'). Enter Q to quit: \n>>");
	 	fgets(user_input, sizeof(user_input), stdin);

		if(strcmp("ls\n",user_input)  == 0){
			strcat(user_input, "current_dir");
		}
				
		token = strtok(user_input, " \n");

		if(strcmp("Q",token)  == 0){
			printf("Exiting Application\n");
			close(serv_sock);
			break;
		}

		if(token == NULL){	
			printf("Ensure command has 1 argument\n\n");
		}else{
			//split the command and the user argument:
			while ((token != NULL)) {	
				if(i==0){			
					strcpy(command,token);
					printf("%s\n",command);
				}else if (i==1){
					strcpy(user_argument,token);
					printf("%s\n",user_argument);
				}
				i++;
	       			token= strtok(NULL, "\n");			
			}
			i=0;


			//prepare message
			tpdu.length = strlen(user_argument);
			memcpy(tpdu.data,user_argument,tpdu.length);





			if(strcmp("cd",command)==0){

				//prepare type and send
				tpdu.type='P';
	    		write(serv_sock, &tpdu, sizeof(tpdu));//1B for type, 4B for length ,5B overhead					
				//read server response	
				read(serv_sock, &rpdu, sizeof(rpdu));
				if(rpdu.type =='E')
					printf("Directory Change Unsuccessful\nError message recieved: %s\n\n",rpdu.data);
				else if(rpdu.type =='R')
					printf("\nDirectory Changed Successfully\n\n");
			}else if(strcmp("ls",command)==0){

				//prepare type and send
				tpdu.type='L';
	    		write(serv_sock, &tpdu,sizeof(tpdu));//1B for type, 4B for length ,5B overhead	
				read(serv_sock, &rpdu, sizeof(rpdu));

				if(rpdu.type =='E')
					printf("Directory Change Unsuccessful\nError message recieved: %s\n\n",rpdu.data);
				else if(rpdu.type =='I')
					printf("\nList:\n%s\nListing successful, Server Ready\n\n",rpdu.data);
				
			}else if(strcmp("U",command)==0){
	 		
				fp = fopen(user_argument,"r");
      				 
				if(fp < 0)
					printf("\nFile does not exist\n");
				else{ 
					tpdu.type='U';
		    		write(serv_sock,&tpdu,sizeof(tpdu));//1B for type, 4B for length ,5B overhead
					read(serv_sock, &rpdu, sizeof(rpdu));
					
					if(rpdu.type='R'){ 		 	
						stat(user_argument, &filestat);
	      				size = filestat.st_size;
						tpdu.type='F';
						while(size>ARG_SIZE){
							fread(&tpdu.data, ARG_SIZE,1,fp);
							tpdu.length=strlen(tpdu.data);
							write(serv_sock, &tpdu, sizeof(tpdu));		 
							size = (size - (ARG_SIZE));
						}
						memset(&tpdu, 0, sizeof(tpdu));
						tpdu.type='F';
						fread(&tpdu.data, size,1,fp);
						tpdu.length=strlen(tpdu.data);
						write(serv_sock, &tpdu,sizeof(tpdu));
						fclose(fp);
					}else
						printf("\nServer not Ready, upload failed\n");									
				}

			}else if(strcmp("D",command)==0){
	 		

				tpdu.type='D';
	    		write(serv_sock,&tpdu,sizeof(tpdu));//1B for type, 4B for length ,5B overhead
				read(serv_sock, &rpdu, sizeof(rpdu));
				if(rpdu.type == 'E')
					printf("File not recieved!!! \nError message recieved: %s\n",rpdu.data);
				else if (rpdu.type == 'F') {
					fp = fopen(user_argument,"w");

					while (rpdu.type !='R'){
						fwrite(rpdu.data , 1 , rpdu.length, fp);
						read(serv_sock, &rpdu, sizeof(rpdu));
					}
					fclose(fp);
				}
	
			}else
				printf("Enter valid command!!\n\n");
			
			memset(&tpdu, 0, sizeof(tpdu));

		}
	}



}

int main (int argc, char *argv[]) {

    int sd; 
    struct sockaddr_in servaddr, cli; 
	int port;
  
  	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

  
    // socket create and varification 
    sd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    //servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(port); 
  
    // connect the client socket to server socket 
    if (connect(sd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(1); 
    } 
    else
        printf("connected to the server..\n"); 
  
    // function for chat 
    init_serv(sd); 
  
    // close the socket 
    close(sd); 
}
