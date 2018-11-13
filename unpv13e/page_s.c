/* include fig01 */
#include	"./lib/unp.h"
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
        return;
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
        return;
    
    /* 我們要把 GET /index.html HTTP/1.0 後面的 HTTP/1.0 用空字元隔開 */
    for(i=4;i<BUFSIZE;i++) {
        if(buffer[i] == ' ') {
            buffer[i] = 0;
            break;
        }
    }

    

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

    
}

void e_s(const char* x) 
{ 
    perror(x); 
    exit(EXIT_FAILURE); 
}

int main(int argc, char **argv)
{
	int					i, maxi, maxfd, listenfd, connfd, sockfd;
	int					nready, client[FD_SETSIZE];
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);

	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
/* end fig01 */

/* include fig02 */
	for ( ; ; ) {
		rset = allset;		/* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
#ifdef	NOTDEF
			printf("new client: %s, port %d\n",
					Inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL),
					ntohs(cliaddr.sin_port));
#endif

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			if (i == FD_SETSIZE)
				e_s("too many clients");

			FD_SET(connfd, &allset);	/* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
					str_echo(sockfd);
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				}
			if (--nready <= 0)
					break;
		}
	}
}
/* end fig02 */
