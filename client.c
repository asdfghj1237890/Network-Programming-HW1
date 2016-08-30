#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>


#define MAXLINE 4096 /*max text line length*/

int create_socket(int port,char *addr)
{
 int sockfd;
 struct sockaddr_in servaddr;

//Create a socket for the client
//If sockfd<0 there was an error in the creation of the socket
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		printf("Problem in creating the socket\n");
		exit(0);
 	}

//Creation of the socket
memset(&servaddr, 0, sizeof(servaddr));
servaddr.sin_family = AF_INET;
servaddr.sin_addr.s_addr= inet_addr(addr);
servaddr.sin_port =  htons(port); //convert to big-endian order

//Connection of the client to the socket
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
  		printf("Problem in creating data channel\n");
  		exit(1);
 	}

	return(sockfd);
}

void printlist(){
	
	
	printf("[1]chdir type: 1+pathname\n");
	printf("[2]ls type: 2\n");
	printf("[3]upload type: 3+filename\n");
	printf("[4]download type: 4+filename\n");
	printf("[5]quit type: 5\n");
}
int
main(int argc, char **argv)
{
int sockfd;
struct sockaddr_in servaddr;
char sendline[MAXLINE], recvline[MAXLINE];
system("mkdir Download");
//basic check of the arguments
//additional checks can be inserted
if (argc !=3) {
  printf("Usage: ./a.out <IP address of the server> <port number>\n");
  exit(3);
}

//Create a socket for the client
//If sockfd<0 there was an error in the creation of the socket
if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
	printf("Problem in creating the socket\n");
	exit(4);
}

//Creation of the socket
memset(&servaddr, 0, sizeof(servaddr));
servaddr.sin_family = AF_INET;
servaddr.sin_addr.s_addr= inet_addr(argv[1]);
servaddr.sin_port =  htons(atoi(argv[2])); //convert to big-endian order

//Connection of the client to the socket
if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
	printf("Problem in connecting to the server\n");
	exit(5);
 }
printlist();
while (fgets(sendline, MAXLINE, stdin) != NULL) {
	send(sockfd, sendline, MAXLINE, 0);
	char *token,*dummy;
	dummy=sendline;
	token=strtok(dummy," ");
   
	if (strcmp("5\n",sendline)==0)  {
   		close(sockfd);
		return 0;
   	}
else if (strcmp("2\n",sendline)==0)  {
   	char buff[MAXLINE],check[MAXLINE]="1",port[MAXLINE];
	int data_port,datasock;
	recv(sockfd, port, MAXLINE,0);				//reciening data connection port
	data_port=atoi(port);
	datasock=create_socket(data_port,argv[1]);
	while(strcmp("1",check)==0){ 				//to indicate that more blocks are coming
		recv(datasock,check,MAXLINE,0);
		if(strcmp("0",check)==0)			//no more blocks of data
			break;
		recv(datasock, buff, MAXLINE,0);
		printf("%s",buff);
	}printf("~~ls data done~~\n\n");
	
   }
   else if (strcmp("1",token)==0)  {
	char check[MAXLINE];
	token=strtok(NULL," \n");
	printf("Path given is:%s\n",token);
	recv(sockfd,check,MAXLINE,0);
	if(strcmp("0",check)==0){
		printf("Directory doesn't exist. Check Path\n");
	}else printf("Path change success\n\n");
   }

   else if (strcmp("3",token)==0)  {
   	char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
	int data_port,datasock,lSize,num_blks,num_last_blk,i;
	FILE *fp;
	recv(sockfd, port, MAXLINE,0);				//receiving the data port
	data_port=atoi(port);
	datasock=create_socket(data_port,argv[1]);
	token=strtok(NULL," \n");
	if ((fp=fopen(token,"r"))!=NULL)
	{
		//size of file
		send(sockfd,"1",MAXLINE,0);
		fseek (fp , 0 , SEEK_END);
		lSize = ftell (fp);
		rewind (fp);
		num_blks = lSize/MAXLINE;
		num_last_blk = lSize%MAXLINE; 
		sprintf(char_num_blks,"%d",num_blks);
		send(sockfd, char_num_blks, MAXLINE, 0);

		for(i= 0; i < num_blks; i++) { 
			fread (buffer,sizeof(char),MAXLINE,fp);
			send(datasock, buffer, MAXLINE, 0);
		}
		sprintf(char_num_last_blk,"%d",num_last_blk);
		send(sockfd, char_num_last_blk, MAXLINE, 0);
		if (num_last_blk > 0) { 
			fread (buffer,sizeof(char),num_last_blk,fp);
			send(datasock, buffer, MAXLINE, 0);
		}
		fclose(fp);
		printf("File upload done.\n");
	}
	else{
		send(sockfd,"0",MAXLINE,0);
		printf("Error in opening file. Check filename\nUsage: put filename\n");
	}
   }

   else if (strcmp("4",token)==0)  {
   	char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE],message[MAXLINE];
	int data_port,datasock,lSize,num_blks,num_last_blk,i;
	FILE *fp;
	recv(sockfd, port, MAXLINE,0);
	data_port=atoi(port);
	datasock=create_socket(data_port,argv[1]);
	token=strtok(NULL," \n");	
	recv(sockfd,message,MAXLINE,0);
	if(strcmp("1",message)==0){
		chdir("Download");
		//system("ls -hl");
		if((fp=fopen(token,"w"))==NULL)
			printf("Error in creating file\n");
		else
		{
			recv(sockfd, char_num_blks, MAXLINE,0);
			num_blks=atoi(char_num_blks);
			for(i= 0; i < num_blks; i++) { 
				recv(datasock, buffer, MAXLINE,0);
				fwrite(buffer,sizeof(char),MAXLINE,fp);
			}
			recv(sockfd, char_num_last_blk, MAXLINE,0);
			num_last_blk=atoi(char_num_last_blk);
			if (num_last_blk > 0) { 
				recv(datasock, buffer, MAXLINE,0);
				fwrite(buffer,sizeof(char),num_last_blk,fp);
			}
			fclose(fp);
			printf("File download done.\n\n");
			chdir("..");
		}
	}
	else{
		printf("Error in opening file. Check filename\nUsage: put filename\n");
	}
   }
   else{
	printf("Error in command. Check Command\n");
   }
	printlist();
 }

 exit(0);
}



