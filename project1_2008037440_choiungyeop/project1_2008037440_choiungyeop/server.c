#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAXLINE 1024
#define SERVERPORT 4000
#define HEADERSIZE 1024
#define PROJECTFORDER "./"

int parseMessage( char *str, char **request, char **type, char **httpver, char **accepttype );
int sendPage( int sockfd, char *filename, char* httpver, char* accepttype );
int main( int argc, char **argv )
{
	/* ----- Variables ----- */
	int l_nPortNo;
	int l_nServerSocketFd, l_nClientSocketFd, l_nClientLen;
	struct sockaddr_in l_sClientAddr, l_sServerAddr;
	char l_cBuf[MAXLINE];
	char *l_pRequest;
	char *l_pType;
	char *l_pHttpVer;
	char *l_pAcceptType;

	/* ----- Print Port Number -----*/

	printf("SERVER <PORT:4000>\n\n");

	/* ----- Server Socket Open ----- */
	
	if ((l_nServerSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error : ");
		exit(0);
	}

	if ( argc < 2 )
	{
		perror("please set port number");
		exit(0);
	}
	bzero( &l_sServerAddr, sizeof(l_sServerAddr) );
	
	l_nPortNo = atoi( argv[1] );
	l_sServerAddr.sin_family = AF_INET;
	l_sServerAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	l_sServerAddr.sin_port = htons( l_nPortNo );

	/* ----- Bind Socket ----- */

	if( bind(l_nServerSocketFd , (struct sockaddr *)&l_sServerAddr, sizeof(l_sServerAddr)) == -1 )
	{
		perror("bind error : ");
		exit(0);
	}

	/* ----- Make 5 Queue Listener ----- */

	if(listen(l_nServerSocketFd, 5)  == -1)
	{
		perror("listen error : ");
		exit(0);
	}

	signal(SIGCHLD, SIG_IGN);

	/* ----- If Server Listens Message, Server Make Child Process And Waiting Them Finished   ----- */

	while(1)
	{
		l_nClientSocketFd = accept(l_nServerSocketFd, (struct sockaddr *)&l_sClientAddr, &l_nClientLen);
		if (l_nClientSocketFd == -1)
		{
			perror("Accept error : ");
			exit(0);
		}

		/* ----- Make Child Process  ----- */
		
		if (!fork())
		{
			memset(l_cBuf, 0x00, MAXLINE);
			if (read(l_nClientSocketFd, l_cBuf, MAXLINE-1) <= 0)
			{
				close(l_nClientSocketFd);
				exit(0);
			}
			parseMessage( l_cBuf, &l_pRequest, &l_pType, &l_pHttpVer, &l_pAcceptType );
			sendPage( l_nClientSocketFd, l_pRequest, l_pHttpVer, l_pAcceptType );
			free(l_pRequest);
			free(l_pType);
			free(l_pHttpVer);
			free(l_pAcceptType);
			close(l_nClientSocketFd);
			exit(0);
		}
		close(l_nClientSocketFd);

		/* ----- Waiting Child Process Finished  -----*/

		while(waitpid(-1,NULL,WNOHANG) > 0);
	}

	/* ----- Server Socket Close  ----- */

	close(l_nServerSocketFd);
}

/* ----- Parsing Message ----- */

int parseMessage( char *str, char **request, char**type, char**httpver, char**accepttype ){
	char *l_pLineToken;
	char *tl;
	char *td;
	char l_cToken[] = "\r\n";
	char *defaultPage = "/open.html";
	char *defaultAccept = "html";
	int i;
	l_pLineToken = strtok(str, l_cToken);
	strtok(NULL, l_cToken);
	strtok(NULL, l_cToken);
	tl = strtok(NULL, l_cToken);
	l_pLineToken = strtok(l_pLineToken," ");
	for( i = 0 ; i < 3 ; i++ ){
		if( l_pLineToken == NULL || i>2 ){
			break;
		}if(i==0){
			*type = (char*)malloc( strlen(l_pLineToken) );
			strcpy(*type,l_pLineToken);
		}else if(i==1){
			*request = (char*)malloc( strlen(l_pLineToken) );
			strcpy(*request,l_pLineToken);
		}else if(i==2){
			*httpver = (char*)malloc( strlen(l_pLineToken) );
			strcpy(*httpver,l_pLineToken);
		}
		l_pLineToken = strtok(NULL," ");
	}
	printf("%s,%s,%s\n",*type, *request, *httpver);
	
	/* ----- If Client Don't Send Get Message ( http://localhost ), Server Send Default Page ( open.html )  ----- */

	if(!strcmp(*request,"/")){
		memset(*request,0x00,strlen(defaultPage));
		strcpy(*request,defaultPage);
		*accepttype = (char*)malloc( strlen(defaultAccept) );
		strcpy(*accepttype,defaultAccept);	
	}
	else{
		td = (char*)malloc( strlen(*request) );
		strcpy(td,*request);
		td = strtok(td,".");
		td = strtok(NULL,".");
		*accepttype = (char*)malloc( strlen(td) );
		strcpy(*accepttype,td);	
	}
	return 1;
}

/* ----- Send Response ----- */

int sendPage( int sockfd, char *filename, char *httpver, char *accepttype ){
	int readn;
	char header[HEADERSIZE];
	int fd;
	char l_cBuf[MAXLINE];
	char* projectforder = PROJECTFORDER;
	char* projectroot;
	projectroot = (char*)malloc(strlen(projectforder)+strlen(filename));
	sprintf(projectroot,"%s%s",projectforder,filename);
	memset(header, 0x00, HEADERSIZE);
	if(filename != NULL){
		fd = open(projectroot, O_RDONLY);
		
		/* ----- If Opening File Successed, Sending Data  ----- */

		if(fd != -1){
			//lseek(fd,0,SEEK_END);
			if(!strcmp(accepttype,"html")){
				sprintf(header, "%s 200 OK \nServer: myserver(port:4000)\nContent-Length:"
						"%d\nConnection: close\nContent-Type: text/html\n"
						"charset=UTF8\n\n",
						httpver, (int)lseek(fd,0,SEEK_END) );
			}else if(!strcmp(accepttype,"mp3")){
				sprintf(header, "%s 200 OK \nServer: myserver(port:4000)\nContent-Length:"
						"%d\nConnection: close\nContent-Type: audio/mpeg3\n"
						"charset=UTF8\n\n",
						httpver, (int)lseek(fd,0,SEEK_END) );
			}else if(!strcmp(accepttype,"pdf")){
				sprintf(header, "%s 200 OK \nServer: myserver(port:4000)\nContent-Length:"
						"%d\nConnection: close\nContent-Type: application/pdf\n"
						"charset=UTF8\n\n",
						httpver, (int)lseek(fd,0,SEEK_END) );
			}else if(!strcmp(accepttype,"jpg")||!strcmp(accepttype,"jpeg")){
				sprintf(header, "%s 200 OK \nServer: myserver(port:4000)\nContent-Length:"
						"%d\nConnection: close\nContent-Type: image/jpeg\n"
						"charset=UTF8\n\n",
						httpver, (int)lseek(fd,0,SEEK_END) );
			}else if(!strcmp(accepttype,"gif")){
				sprintf(header, "%s 200 OK \nServer: myserver(port:4000)\nContent-Length:"
						"%d\nConnection: close\nContent-Type: image/gif\n"
						"charset=UTF8\n\n",
						httpver, (int)lseek(fd,0,SEEK_END) );
			}else{
				/* ----- But Not PDF,JPEG,MP3,HTML,GIF, Send Open.html  ----- */
				sprintf(header, "%s 200 OK \nServer: myserver(port:4000)\nContent-Length:"
						"%d\nConnection: close\nContent-Type: text/html\n"
						"charset=UTF8\n\n",
						httpver, (int)lseek(fd,0,SEEK_END) );

			}

			write(sockfd, header, strlen(header));
			lseek(fd,0,0);
			
			if(fd != -1){
				while((readn = read(fd,l_cBuf,MAXLINE))>0){
					write(sockfd, l_cBuf, readn);
				}
			}else{
				printf("File not found!\n");
			}
			close(fd);
		}
	}else{
		printf("Filename not found!\n");
	}
	return 1;
}

