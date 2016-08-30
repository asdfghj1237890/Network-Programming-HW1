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

# define MAXLINE  4096
# define LISTENQ 10

void(*signal(int signo,void(*func)(int)))(int);
typedef void Sigfunc(int);
/*struct sigaction{
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_sigaction)(int int,siginfo_t*,void*);
}*/
Sigfunc *signal(int signo, Sigfunc *func){
	struct sigaction act,oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if(signo == SIGALRM){
		#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
		#endif	
	}else{
		#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;
		#endif	
	}
	if (sigaction(signo, &act, &oact)<0)
		return(SIG_ERR);
	return (oact.sa_handler);
}
void sig_chld(int signo){
	pid_t pid;
	int stat;
	//char *ip;
	//ip = inet_ntoa(cliaddr.sin_addr);
	while((pid = waitpid(-1,&stat,WUNTRACED))>0){
		
		printf("child %d:127.0.0.1 terminated \n",pid);
	}
}/*
void comm_read(int sockfd){
	ssize_t n;
	char buf[MAXLINE],temp[MAXLINE];
	FILE *in;
	while((n = read(sockfd,buf,MAXLINE))>0)
		printf("Commend from client: %s \n",buf);
		if (strcmp("ls -hl\n",buf) == 0){
			if(!(in = popen("ls -hl\n","r"))){
				printf("-ls error!!!");
				exit(0);
			}
			while(fgets(temp,sizeof(temp), in) != NULL){
				write(sockfd,"1",MAXLINE);
				write(sockfd,temp,MAXLINE);
			}
			write(sockfd,"0",MAXLINE);
			pclose(in);
		}
		//write(sockfd,buf,n);
		
}*/
int create_socket(int port){
	int listenfd;
	struct sockaddr_in dataservaddr;

//Create a socket for the soclet
//If sockfd<0 there was an error in the creation of the socket
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		printf("Problem in creating the data socket\n");
		exit(0);
	}
//preparation of the socket address
	dataservaddr.sin_family = AF_INET;
	dataservaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	dataservaddr.sin_port = htons(port);

	if ((bind (listenfd, (struct sockaddr *) &dataservaddr, sizeof(dataservaddr))) <0) {
		printf("Problem in binding the data socket\n");
		exit(1);
	}
//listen to the socket by creating a connection queue, then wait for clients
listen (listenfd, 1);

return(listenfd);
}
int accept_conn(int sock){
	int dataconnfd;
	socklen_t dataclilen;
	struct sockaddr_in datacliaddr;

	dataclilen = sizeof(datacliaddr);
//accept a connection
	if ((dataconnfd = accept (sock, (struct sockaddr *) &datacliaddr, &dataclilen)) <0) {
		printf("Problem in accepting the data socket\n");
		exit(2);
}

return(dataconnfd);

}

int main(int argc,char **argv){\
	
	int listenfd,connfd,n;
	pid_t childpid = 0;
	socklen_t clilen;
	char * ip;
	char buf[MAXLINE];
	int data_port ;
	int preport ;

	struct sockaddr_in cliaddr,servaddr;

	system("mkdir Upload");
	if(argc != 2){
		printf("usage:tcpser<Port>\n");
	}
//Create a socket for the soclet
//If sockfd<0 there was an error in the creation of the socket
	if((listenfd = socket(AF_INET,SOCK_STREAM,0))<0){
		printf("Prolbem in creating socket\n");
		exit(3);
	}
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));
//bind socket
	bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
//listen to socket
	listen(listenfd,LISTENQ);
	//signal(SIGCHLD,sig_chld);
	printf("Server running\nwaiting for connections.\n");
	for( ; ; ){
		clilen = sizeof(cliaddr);
		//accept connection
		if ((connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen))< 0){
			if(errno == EINTR)
				continue;
			else
				printf("accept error");		
		}
		ip = inet_ntoa(cliaddr.sin_addr);
		printf("connection from %s port:%d\n",ip,cliaddr.sin_port);
		if((childpid = fork()) == 0){
		//close listening socket
			close(listenfd);
			//comm_read(connfd);
			//exit(0);
			
			while((n = recv(connfd, buf, MAXLINE, 0)) > 0){
				printf("Command received from client: %s\n",buf);
				char *token,*dummy;
				dummy = buf;
				//printf("dummy:%s\n",dummy);
				token = strtok(dummy," ");
				//char curr_dir[MAXLINE];
				//getcwd(curr_dir,MAXLINE-1);
				//send(connfd, curr_dir, MAXLINE, 0);
				//system("chdir ");

				//printf("token:%s\n",token);
				if (strcmp("5\n",buf) == 0)
					printf("client %s port:%d has quit\n",ip,cliaddr.sin_port);
				if(strcmp("2\n",buf) == 0){
					sleep(1);
					FILE *in;
					char temp[MAXLINE],port[MAXLINE];
					int datasock;
					srand(time(NULL));
					data_port+=1024+(rand()%100)+1;
					if(data_port==atoi(argv[1])){
						data_port=data_port+1;
					}
					sprintf(port,"%d",data_port);
					//printf("Data port:%s\n",port);
					//create socket
					datasock = create_socket(data_port);
					//send data-connect port number to client
					send(connfd, port,MAXLINE,0);
					//accept connection from client
					datasock=accept_conn(datasock);
					if(!(in = popen("ls -hl", "r"))){
						printf("error ls\n");
					}
					while(fgets(temp, sizeof(temp), in)!=NULL){
						printf("~~Sending port:%d ls data~~\n",cliaddr.sin_port);
						send(datasock,"1",MAXLINE,0);
						send(datasock,temp,MAXLINE,0);
					}
					send(datasock,"0",MAXLINE,0);
					pclose(in);
					printf("~~Send ls data done~~\n");
				}

				if(strcmp("1",token) == 0){
					token = strtok(NULL," \n");
					printf("Path need to change:%s\n",token);
					if(chdir(token)<0){
						send(connfd,"0",MAXLINE,0);
					}else{
						send(connfd,"1",MAXLINE,0);
					}
				}
				if(strcmp("3",token) == 0){
					char port[MAXLINE],buf[MAXLINE],char_numblk[MAXLINE],char_numlastblk[MAXLINE],check[MAXLINE];
					int datasock,numblk,numlastblk,i;
					FILE *fp;
					
					token = strtok(NULL," \n");
					printf("File ready to upload:%s\n",token);
					srand(time(NULL));
					data_port+=1024+(rand()%100)+1;
					if(data_port == atoi(argv[1])){
						data_port+=(rand()%1000)+1;
					}
					printf("data_port:%d\n",data_port);
					sprintf(port,"%d",data_port);
					//printf("Port:%s\n",port);
					//creating socket
					datasock=create_socket(data_port);
					//sending data-connection port number to client
					send(connfd, port,MAXLINE,0);
					//accepting connection
					datasock=accept_conn(datasock);
					recv(connfd,check,MAXLINE,0);
					printf("%s",check);
					if(strcmp("1",check) == 0){
						if((fp=fopen(token,"w")) == NULL)
							printf("Error in creating file\n");
						else{
							recv(connfd,char_numblk,MAXLINE,0);
							numblk = atoi(char_numblk);
							for(i=0; i<numblk; i++){
								recv(datasock,buf,MAXLINE,0);
								fwrite(buf,sizeof(char),numlastblk,fp);
							}
							fclose(fp);
							printf("File download done.\n");
						}
					}
				}
				if(strcmp("4",token) == 0){
					char port[MAXLINE],buffer[MAXLINE],char_numblk[MAXLINE],char_numlastblk[MAXLINE];
					int datasock,lsize,numblk,numlastblk,i;
					FILE *fp;
					token = strtok(NULL," \n");
					printf("File ready to download:%s\n",token);
					srand(time(NULL));
					sprintf
					data_port+=1024+(rand()%100)+1;
					if(data_port == atoi(argv[1])){
						data_port+=(rand()%1000)+1;
					}(port,"%d",data_port);
					datasock = create_socket(data_port);
					send(connfd,port,MAXLINE,0);
					datasock = accept_conn(datasock);
					if((fp = fopen(token,"r")) != NULL){
					//size of file
						send(connfd,"1",MAXLINE,0);
						fseek(fp,0,SEEK_END);
						lsize = ftell(fp);
						rewind(fp);
						numblk = lsize / MAXLINE;
						numlastblk = lsize % MAXLINE;
						sprintf(char_numblk,"%d",numblk);
						send(connfd,char_numblk,MAXLINE,0);
						for(i=0; i< numblk; i++){
							fread(buffer,sizeof(char),MAXLINE,fp);
							send(datasock,buffer,MAXLINE,0);
						}
						sprintf(char_numlastblk,"%d",numlastblk);
						send(connfd,char_numlastblk,MAXLINE,0);
						if(numlastblk > 0){
							fread(buffer,sizeof(char),numlastblk,fp);
							send(datasock,buffer,MAXLINE,0);
						}
						fclose(fp);
						printf("File upload done.\n");
					}else{
						send(connfd,"0",MAXLINE,0);
					}
				}
			}
			if(n < 0) printf("Socket Read error!\n");
			exit(0);
		}
		//close socket of server
		close(connfd);
	}
}

c


