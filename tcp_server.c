#include <dirent.h>
#include <stdio.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <dirent.h>
#include <sys/stat.h>

#define FILENAME_SIZE 255
#define ARG_SIZE 4097
#define SERVER_TCP_PORT 8080

struct pdu
{
 char type;
 int length;
 char data[ARG_SIZE];
} rpdu, tpdu;


int init_client(int cli_sock){

	char type_recv;	
    DIR *folder;
	char current_folder[ARG_SIZE];
	char filename [ARG_SIZE+FILENAME_SIZE];    	
	struct dirent *dir;
	int i=0,size=0,flag =0;
	FILE* fp;
	struct stat filestat;
	
	strcpy(current_folder,".");
	
	tpdu.type='R';
    write(cli_sock, &tpdu, 1);//1B

	while(1){

		if(flag==0)
			read(cli_sock, &rpdu, sizeof(rpdu));
		else{
			flag=0;
			printf("flag removed\n");
			}		
		type_recv = rpdu.type;
		
		switch(type_recv){
	
		case 'P':
			folder = opendir(rpdu.data);
					
			if(folder == NULL){
				printf("cd: Unable to read directory\n");
				tpdu.type = 'E';
				strcpy(tpdu.data,"Cannot change to specfied directory");
				tpdu.length=strlen(tpdu.data);
				write(cli_sock, &tpdu,sizeof(tpdu));

	      		}else{
				printf("cd: Current directory has been changed!\n");
				strcpy(current_folder,rpdu.data);
				tpdu.type='R';
				write(cli_sock, &tpdu,sizeof(tpdu));
	    		    	closedir(folder);
			}
			break;
		case 'L':
			if(strcmp("current_dir",rpdu.data)==0)
				folder = opendir(current_folder);
			else
				folder =opendir(rpdu.data);

			if(folder == NULL){

        		printf("ls: Cannot list specified directory\n");
				tpdu.type = 'E';
				strcpy(tpdu.data,"Cannot list specified directory");
				tpdu.length=strlen(tpdu.data);
				write(cli_sock, &tpdu,sizeof(tpdu));

      			}else{
        			printf("ls: Compiling and sending list:\n");
					i=0;
					tpdu.type='I';
					 while ((dir = readdir(folder)) != NULL){
          	  				//printf("%s\n", dir->d_name);
						
						//if the directory file names is longer than the data size , then send the compiled file and folder names and create new pdu to be sent
						if((strlen(dir->d_name)+i+1)>ARG_SIZE){
							tpdu.length=strlen(tpdu.data);
							write(cli_sock, &tpdu,sizeof(tpdu));
							memset(&tpdu, 0, sizeof(tpdu));
							tpdu.type='I';
							i=0;
						}
						//copy the data to tpdu and append new line after each entry
						memcpy(tpdu.data+i,dir->d_name,strlen(dir->d_name));
						memcpy(tpdu.data+strlen(dir->d_name)+i,"\n",1);
						i=i+strlen(dir->d_name)+1;
        				}

				tpdu.length=strlen(tpdu.data);
				write(cli_sock, &tpdu,sizeof(tpdu));
    				closedir(folder);
    			}

			break;
		case 'D':
			memcpy(filename,current_folder,strlen(current_folder));
			strcat(filename, "/");
			strcat(filename, rpdu.data);
			fp = fopen(filename,"r");			 
			
				if(fp < 0){
					printf("\nFile does not exist\n");
					tpdu.type = 'E';
					strcpy(tpdu.data,"File not found!!!");
					tpdu.length=strlen(tpdu.data);
					write(cli_sock, &tpdu,sizeof(tpdu));	
				}else{ 
					printf("\nSending File, %s, to Client\n",rpdu.data);
					stat(filename, &filestat);
	      				size = filestat.st_size;

					
					tpdu.type='F';
					while(size>ARG_SIZE){
						fread(&tpdu.data, ARG_SIZE,1,fp);
						tpdu.length=strlen(tpdu.data);
						write(cli_sock, &tpdu, sizeof(tpdu));		 
						size = (size - (ARG_SIZE));

					}
					//send final data packet
					memset(&tpdu, 0, sizeof(tpdu));
					tpdu.type='F';
					fread(&tpdu.data, size,1,fp);
					tpdu.length=strlen(tpdu.data);
					write(cli_sock, &tpdu,sizeof(tpdu));	
					//send ready data packet
					memset(&tpdu, 0, sizeof(tpdu));
					tpdu.type='R';
					write(cli_sock, &tpdu,sizeof(tpdu));
				}
			break;
		case 'U':
				
			memcpy(filename,current_folder,strlen(current_folder));
			strcat(filename, "/");
			strcat(filename, rpdu.data);
			
			fp = fopen(filename,"w");
      				 
				if(fp < 0){
					printf("\nError writing to file\n");
					tpdu.type = 'E';
					strcpy(tpdu.data,"Error writing to file");
					tpdu.length=strlen(tpdu.data);
					write(cli_sock, &tpdu,(tpdu.length+1+4+5));
				}else{ 
					printf("\nRecieving File, %s, from Client\n",rpdu.data);
					tpdu.type='R';
    				write(cli_sock, &tpdu,1);//1B for type, 4B for length, 5B overhead
					read(cli_sock, &rpdu, sizeof(rpdu));
					
					while (rpdu.type =='F'){
						if(rpdu.length<ARG_SIZE){
							fwrite(rpdu.data , 1 , rpdu.length, fp);
							break;
						}else{
							fwrite(rpdu.data , 1 , rpdu.length, fp);
							read(cli_sock, &rpdu, sizeof(rpdu));
						}
					}				
					if(rpdu.type !='F'){
						flag=1;
					}
					fclose(fp);
				}
			break;
        
		default: 
			printf("Client closed connection!\n\n");
			exit(0); 
			break; 
		}
		memset(&tpdu, 0, sizeof(tpdu));
	}
	
}

int main (int argc, char *argv[]) {

    int server_fd, sd, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen;
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
	
	
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        fprintf(stderr, "socket failed\n"); 
        exit(EXIT_FAILURE); 
    } 
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        fprintf(stderr,"setsockopt\n"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port ); 
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        fprintf(stderr,"bind failed\n"); 
        exit(EXIT_FAILURE); 
    } 
    if (listen(server_fd, 5) < 0) 
    { 
        fprintf(stderr,"Listen failed\n"); 
        exit(EXIT_FAILURE); 
    } 
    printf("Listening on %d\n", port);
    while(1) {
        if ((sd = accept(server_fd, (struct sockaddr *)&address,  
                           (socklen_t*)&addrlen))<0) 
        { 
            fprintf(stderr,"Accept Error"); 
            exit(EXIT_FAILURE); 
        } 
        switch(fork()) {
            case 0:
                close(server_fd);
                printf("Received connection\n");
                exit(init_client(sd));
			break;
            default:
                close(sd);
                break;
            case -1:
                fprintf(stderr,"Fork Error\n");
                break;
        }
    }
    return 0; 
}


