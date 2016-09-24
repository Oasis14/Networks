/* Ryan Craig
 * Networks assignment 1
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

void printUsage(){
	printf("\nUsage: \n");
	printf("help -Returns the list of command you can send \n");
	printf("ls -Returns a list of Files in the server directiory. ls flags do not work \n");
	printf("get <fileName> -Gets a file from server and saves it in client folder \n");
	printf("put <fileName> -Puts a file from client to server \n");
}

#define MAXBUFSIZE 512
#define FILENAMESIZE 32

/* You will have to modify the program below */

int main (int argc, char * argv[])
{

	int messageLength;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];

	struct sockaddr_in remote;              //"Internet socket address structure"

	//checks enough arguments have been passed in
	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}
	unsigned short port = atoi(argv[2]);
	if(port <= 5000){
		printf("Port must be larger then 5000");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		printf("unable to create socket");
		exit(2);
	}

	/******************
	  sendto() sends immediately.
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/





	 printUsage();
	int true = 1;
	while(true){
		printf(">");
		int remote_length = sizeof(remote);
		// receiving data set up
		struct sockaddr_in from_addr;
		int addr_length = sizeof(struct sockaddr);
		bzero(buffer,sizeof(buffer));


		char command[MAXBUFSIZE];
		fgets(command,sizeof(command)-1,stdin);

		//ls command
		if(!strncmp(command,"ls",2)){
			//send command to server
			messageLength = sendto(sock, command, sizeof(command), 0,(struct sockaddr *) &remote, remote_length);

			messageLength = recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &remote_length);
			printf(" %s\n", buffer);
		}
		//break the loop so the program ends
		else if (!strncmp(command,"exit",4)){
			close(sock);
			true = 0;
		}
		else if (!strncmp(command,"put",3)){
			printf("Running the put command\n");
			//get filename
			messageLength = sendto(sock, command, sizeof(command), 0,(struct sockaddr *) &remote, remote_length);
			int i = 0;
			char fileName[FILENAMESIZE];

			while(strncmp(&command[i+4],"\0",1) != 0 && strncmp(&command[i+4],"\n",1) !=0 && i < FILENAMESIZE){
				fileName[i] = command[i+4];
				i++;

			}
			fileName[i] = '\0';

			if(access(fileName,F_OK) != -1){
				FILE *file;
				printf("Opening File\n");
				file = fopen(fileName,"rb");

				if(file != NULL){
					//get file size
					fseek(file,0L,SEEK_END);
					int fileSize = ftell(file);
					rewind(file);

					//let server know the file size
					char fileSizeSend[MAXBUFSIZE];
					sprintf(fileSizeSend,"%d",fileSize);
					messageLength = sendto(sock, fileSizeSend, strlen(fileSizeSend), 0,(struct sockaddr *) &remote, remote_length);
					if(messageLength == -1){
						perror("error sending file size to server");
						exit(-1);
					}
					printf("File Size sent to server\n");
					bzero(command, sizeof(command));

					while(fread(command,1,MAXBUFSIZE,file)>0){
							messageLength = sendto(sock, command, sizeof(command), 0,(struct sockaddr *) &remote, remote_length);
							if(messageLength == -1){
								exit(-3);
							}
							bzero(command, sizeof(command));
					} //while sending data
					fclose(file);
				}
			}else{
				bzero(command,sizeof(command));
				printf("File Doesn't Exist\n");
				strcat(command,"NOFILE");
					messageLength = sendto(sock, command, sizeof(command), 0,(struct sockaddr *) &remote, remote_length);
					if(messageLength == -1){
						//error sending a buffer the message
						exit(-4);
					}
					//clear buffer to prevent data being sent by accident
					bzero(command,sizeof(command));

			}

		}

		// Get file data and save it to client
		else if (!strncmp(command,"get",3)){

				printf("Runnning the get command \n");
				//get filename for opening and saving the file
				int i = 0;
				char fileName[FILENAMESIZE];

				while(strncmp(&command[i+4],"\0",1) != 0 && strncmp(&command[i+4],"\n",1) !=0 && i < FILENAMESIZE){
					fileName[i] = command[i+4];
					i++;

				}
				fileName[i] = '\0';

				FILE *file;
				file = fopen(fileName, "wb");
				if(file != NULL){
						//send get command to server
						messageLength = sendto(sock, command, sizeof(command), 0,(struct sockaddr *) &remote, remote_length);
						if(messageLength == -1){
							perror("error at sending initial get");
							exit(1);
						}
						//clear buffer so its ready to send again
						bzero(buffer,sizeof(buffer));
						memset(buffer,'\0',MAXBUFSIZE);
						//wait until response is received
						messageLength = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *) &remote, &remote_length);
						if(messageLength == -1){
							perror("error receiving file size");
							exit(-1);
						}

						//check file exists
						if(!strncmp(buffer,"NOFILE",7)){
							remove(fileName);
							printf("\nFile doesnt exist on server. Run ls to find out file avaliable.\n");
						}	else{
							//get size of file
							char fileSize[MAXBUFSIZE];
							bzero(fileSize,MAXBUFSIZE);
							strcat(fileSize, buffer);
							printf("File Size: %s\n",fileSize);

							//keep track of size and transferd
							//https://www.tutorialspoint.com/c_standard_library/c_function_strtol.html
							char *ptr;
							//strol(<char holding number>, <function uses a char ptr to know where its at>, <desired base of number>)
							long transferSize = strtol(fileSize, &ptr,10);
							int transfered = 0;


							int loop = 1;
							while(loop){
								//clear buffer so its ready to send again
								//When in doubt clear the buffer!!
								bzero(buffer,sizeof(buffer));
								messageLength = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &remote_length);
								if(messageLength == -1){
									perror("error receiving some of the file data");
									exit(-1);
								}
								//update amount transfered from server
								transfered += sizeof(buffer);

								//write to file
								printf("Transfer Progress: %d/%s \n",transfered,fileSize);
								fwrite(buffer,sizeof(buffer),1,file);

								if(transfered > transferSize){
									//tranfer is done exit loop
									loop = 0;
									//print message that file has been transfered
									printf("File transfer Complete! \n");
									bzero(buffer,sizeof(buffer));
								}
							}
						}
					}
					fclose(file);
		}
		//if user needs help they just type help
		else if (!strncmp(command,"help",4)){
			printUsage();
		}else{
			printf("Command not found. For a list of commands enter: help\n");
		}
		//messageLength = sendto(sock, "ls", sizeof("ls"), 0,(struct sockaddr *) &remote, remote_length);
		//messageLength = recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &remote_length);
		//printf("Server says %s\n", buffer);


	}

	close(sock);

}
