/* Ryan Craig
 * Networks assignment 1
 */
#include <sys/types.h>
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
#include <string.h>
/* Modified code from course code */

#define MAXBUFFSIZE 512 // package size for transfer
#define FILENAMESIZE 32 //file name shound be to long

int main (int argc, char * argv[] )
{


	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int messageLength;                   //number of bytes we receive in our message

	//used to speify the desired inputs
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	unsigned short port = atoi(argv[1]);
	if(5000 > port){
		printf("Port must be larger then 5000 \n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = htonl(INADDR_ANY);           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		printf("unable to create socket \n");
		exit(2);
	}


	/******************
	  Once we've created a socket, we must bind that socket to the
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
		exit(3);
	}
  // cause an infinante loop so server is always on and listening for connections
	while(1){

		char buffer[MAXBUFFSIZE];             //a buffer to store our received message
		remote_length = sizeof(remote);
		bzero(buffer,sizeof(buffer));  // clear the buffer every time going through

		printf("\n Waiting on data from a client \n");
		//soc, buffer, size of buffer, flags(none), sockaddr struct empty, size of struct
		messageLength = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &remote_length);

		//check no errors on receiving
		if(messageLength == -1){
			//pass in function name to error.
			//This allows debugging to happen quicker if needed
			perror("recvfrom");
			exit(1);
		}


		printf("The client says %s\n", buffer);


		char msg[MAXBUFFSIZE];
		if(!strncmp(buffer,"ls",2)){
			printf("Runnning the ls command \n");
			FILE *ls;
			ls = popen("ls", "r");
			if(!ls){
				printf("Error starting ls command \n");
				return 5;
			}
			while(fread(msg, 1, MAXBUFFSIZE ,ls) > 0){
					//sending data to let user know it was received
					messageLength = sendto(sock, msg, sizeof(msg), 0,(struct sockaddr *) &remote, remote_length);
					if(messageLength == -1){
							printf("error sending back message");
							return 10;
				 }
				 bzero(msg,sizeof(buffer));
			}
			//Put command
		}	else if(!strncmp(buffer,"put",3)){

			printf("Running the put command\n");
			//get fileName
			int i = 0;
			printf("Getting file name\n");
			char fileName[FILENAMESIZE];
			while(strncmp(&buffer[i],"\0",1) != 0 && strncmp(&buffer[i],"\n",1) && i < FILENAMESIZE){
				fileName[i-4] = buffer[i];
				i++;
			}
			//add the ending of the name to char array holding it
			fileName[i-4] = '\0';

			FILE *file;
			file = fopen(fileName,"wb");
			if(file != NULL){
				bzero(msg,sizeof(msg)); //clear buffer befor sending
				bzero(buffer,sizeof(buffer)); //clear buffer to receive file size
				//get filesize from client
				messageLength = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &remote_length);
				if(messageLength == -1){
					perror("error receiving filesize");
					exit(-1);
				}

				char fileSize[MAXBUFFSIZE];
				bzero(fileSize,MAXBUFFSIZE);
				strcat(fileSize,buffer); //move size from buffer to var
				printf("File Size of transfer is: %s\n",fileSize);
				int transfered = 0;

				char * ptr;
				long transferSize = strtol(fileSize,&ptr,10); //convert from string to long


				int loop = 1;
				while(loop){
					bzero(buffer,sizeof(buffer));
					messageLength = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &remote_length);
					if(messageLength == -1){
						perror("error receiving some of the file");
						exit(-1);
					}
					transfered += sizeof(buffer);
					printf("Transfer Progress: %d/%s \n",transfered,fileSize);
					fwrite(buffer,sizeof(buffer),1,file);
					if(transfered >= transferSize){
						loop = 0;
						printf("file transfer Complete\n");
						bzero(buffer,sizeof(buffer));
						fclose(file);
					}
				}
			}

		}
		//Get command sends file to clients
		else if(!strncmp(buffer,"get",3)){
			printf("Runnning the get command \n");
			//get file name from buffer "get fileName"
			//given file name starts at forth spot in buffer
			int i = 4;
			printf("Getting file name\n");
			char fileName[FILENAMESIZE];
			while(strncmp(&buffer[i],"\0",1) != 0 && strncmp(&buffer[i],"\n",1) && i < FILENAMESIZE){
				fileName[i-4] = buffer[i];
				i++;
			}
			//add the ending of the name to char array holding it
			fileName[i-4] = '\0';

			//open then read file

			// check file exist
			if(access(fileName,F_OK) != -1){
				FILE *file;
				printf("Opening File\n");
				file = fopen(fileName, "rb"); //use rb instead of r because it helps with nontext Files
				if(file != NULL){
					//take file to end in order to get file siz
					fseek(file,0L, SEEK_END);
					//store file size then go back to the beginging of file for transfer
					int fileSize = ftell(file);
					rewind(file);
					//fseek(file,0L, SEEK_SET);

					//Let the client know the size of file it is exspectining
					char fileSizeSend[MAXBUFFSIZE];
					sprintf(fileSizeSend,"%d",fileSize);
					messageLength = sendto(sock, fileSizeSend, strlen(fileSizeSend), 0,(struct sockaddr *) &remote, remote_length);
					if(messageLength == -1){
						//error sending alert the message
						perror("sending to");
						exit(-3);
					}
					printf("File size sent!\n");
					bzero(msg,sizeof(msg));
					while(fread(msg, 1, MAXBUFFSIZE, file) > 0){
						messageLength = sendto(sock, msg, sizeof(msg), 0,(struct sockaddr *) &remote, remote_length);
						if(messageLength == -1){
							//error sending a buffer the message
							exit(-4);
						}
						//clear buffer to prevent data being sent by accident
						bzero(msg,sizeof(msg));
					}
					//ALWAYS CLOSE YOUR DAMN FILES
					fclose(file);
				}
			} else{
				printf("File Doesn't Exist");
				strcat(msg,"NOFILE");
					messageLength = sendto(sock, msg, sizeof(msg), 0,(struct sockaddr *) &remote, remote_length);
					if(messageLength == -1){
						//error sending a buffer the message
						exit(-4);
					}
					//clear buffer to prevent data being sent by accident
					bzero(msg,sizeof(msg));
			}
		}
	}
	close(sock);
	return 0;
}
