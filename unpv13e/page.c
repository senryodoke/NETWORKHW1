#include "./lib/unp.h"

#define BUFSIZE 4096

struct {
    char *ext;
    char *filetype;
} extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0} };

void str_echo(int fd)
{
    int j, file_fd, buflen, len;
    long i, ret;
    char * fstr;
    static char buffer[BUFSIZE+1];

    ret = read(fd,buffer,BUFSIZE);   /* 讀取瀏覽器要求 */
    if (ret==0||ret==-1) {
     /* 網路連線有問題，所以結束行程 */
        exit(3);
    }

    /* 程式技巧：在讀取到的字串結尾補空字元，方便後續程式判斷結尾 */
    if (ret>0&&ret<BUFSIZE)
        buffer[ret] = 0;
    else
        buffer[0] = 0;

    /* 移除換行字元 */
    for (i=0;i<ret;i++) 
        if (buffer[i]=='\r'||buffer[i]=='\n')
            buffer[i] = 0;
    
    /* 只接受 GET 命令要求 */
    if (strncmp(buffer,"GET ",4)&&strncmp(buffer,"get ",4))
        exit(3);
    
    /* 我們要把 GET /index.html HTTP/1.0 後面的 HTTP/1.0 用空字元隔開 */
    for(i=4;i<BUFSIZE;i++) {
        if(buffer[i] == ' ') {
            buffer[i] = 0;
            break;
        }
    }

    /* 檔掉回上層目錄的路徑『..』 */
    for (j=0;j<i-1;j++)
        if (buffer[j]=='.'&&buffer[j+1]=='.')
            exit(3);

    /* 當客戶端要求根目錄時讀取 index.html */
    if (!strncmp(&buffer[0],"GET /\0",6)||!strncmp(&buffer[0],"get /\0",6) )
        strcpy(buffer,"GET /index.html\0");

    /* 檢查客戶端所要求的檔案格式 */
    buflen = strlen(buffer);
    fstr = (char *)0;

    for(i=0;extensions[i].ext!=0;i++) {
        len = strlen(extensions[i].ext);
        if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
            fstr = extensions[i].filetype;
            break;
        }
    }

    /* 檔案格式不支援 */
    if(fstr == 0) {
        fstr = extensions[i-1].filetype;
    }

    /* 開啟檔案 */
    if((file_fd=open(&buffer[5],O_RDONLY))==-1)
  write(fd, "Failed to open file", 19);

    /* 傳回瀏覽器成功碼 200 和內容的格式 */
    sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    write(fd,buffer,strlen(buffer));


    /* 讀取檔案內容輸出到客戶端瀏覽器 */
    while ((ret=read(file_fd, buffer, BUFSIZE))>0) {
        write(fd,buffer,ret);
    }
    exit(1);
   
}
void sig_chld(int signo)
{
	while(waitpid(-1,NULL,WNOHANG)>0);
	
}

void e_s(const char* x)
{
	perror(x);
	exit(1);
}

/*void str_echo(int sockfd)
{
	char a[]="hello net!\n";
	char b[]="how about a magic treat!\n";
	char c[]="HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\r\n";
	char buffer[1024]={0};
	size_t result;
	char *filename="44888418.jpeg";
	FILE *fp=fopen(filename,"rb");
	if(fp==NULL)
		printf("cannot open file\n");
	
	if(send(sockfd,a,sizeof(a)-1,0)==-1)
		perror("send");
	send(sockfd,"\n\n\n\n\n\n\n\n",8,0);
	if(send(sockfd,b,sizeof(b)-1,0)==-1)
		perror("send");
	send(sockfd,"\n",1,0);

	write(sockfd,c,sizeof(c));
	while((result=fread(buffer,1,1024,fp))>0){
		send(sockfd,buffer,result,0);
	}
	if(result>0){
		if(feof(fp))
			send(sockfd,buffer,result,0);
		else
			perror("read error\n");
	}
	fclose(fp);
	
}*/

int main(int argc, char **argv)
{
	int	listenfd, connfd;
	pid_t	childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	
	//void	sig_chld(int);
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);
	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	
	listen(listenfd, LISTENQ);
	
	signal(SIGCHLD, sig_chld);	/* must call waitpid() */

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				e_s("accept error");
		}

		if ( (childpid = fork()) == 0) {	/* child process */
			close(listenfd);	/* close listening socket */
			str_echo(connfd);	/* process the request */
			exit(0);
		}
		close(connfd);			/* parent closes connected socket */
	}
}
