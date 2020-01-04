#include"wrapper.h"
#define PORT 8080
#define MAXSIZE 2048

void *process_run(void *);//服务器运行的主要函数 （改为了指针型，实现多线程时需要使用）
void uri_fileName(char *uri, char *fileName);//提取uri上的文件名
void static_html(int fd,char *fileName);//uri查找静态页面处理
void dynamic_html_get(int fd, char *fileName, char *query_string);//动态处理页面,加载cgi程序,get方法
void wrong_req(int fd, char *msg); //错误http事务




int main()
{
	int listen_sock,*conn_sock,clientlen;
	struct sockaddr_in clientaddr;
	pthread_t pid;
	clientlen = sizeof(clientaddr);
	listen_sock = open_listen_sock(PORT);
	while(1)
	{
		conn_sock = malloc(sizeof(int));
		*conn_sock = accept(listen_sock,(struct sockaddr *)&clientaddr,&clientlen);
		if(conn_sock < 0)
		{
			perror("accept");
			exit(1);
		}
		/* test(conn_sock); */
		pthread_create(&pid,NULL,process_run,conn_sock);
		/* process_run(conn_sock); */
		//多线程的实现
	}
}



void wrong_req(int fd,char *msg)
{
	char buf[MAXLINE],body[MAXBUF];
	char fileName[MAXLINE];
	char err[MAXLINE];
	FILE *resource = NULL; 
	
	strcpy(err,msg);
	
	 if(strcmp(msg,"404")==0)
	{
		strcpy(fileName,"./error404.html");
 		strcat(err," not found"); 
	}
	else if(strcmp(msg,"501")==0)
	{
		strcpy(fileName,"./error501.html");
		strcat(err," error method");
	}
	 /*打开 filename 的文件*/  
    resource = fopen(fileName,"r"); 	
	
   	
     
        // 写 HTTP header 
	/*服务器信息*/
	sprintf(buf, "HTTP/1.0 %s \r\n",err);
	sprintf(buf, "%ssmall web\r\n",buf);
	sprintf(buf, "%sContent-Type: text/html\r\n\r\n",buf);
	send(fd,buf,strlen(buf),0);

        /*复制文件*/  
     fgets(buf, sizeof(buf), resource);  
    while (!feof(resource))  
    {  
        send(fd, buf, strlen(buf), 0);  
        fgets(buf, sizeof(buf), resource);  
    }  
    fclose(resource);   
	
}

void * process_run(void *v) 
{
	//多线程的实现
	int fd =  *((int *)v);
	pthread_detach(pthread_self());
	free(v);
	//多线程的实现
	
	struct stat sbuf;//用来描述一个linux系统文件系统中的文件属性的结构。
	/*可以有两种方法来获取一个文件的属性：
		1、通过路径：
		int stat(const char *path, struct stat *struct_stat);
		int lstat(const char *path,struct stat *struct_stat);
		两个函数的第一个参数都是文件的路径，第二个参数是struct stat的指针。返回值为0，表示成功执行。
	*/
	//缓冲区，方法，uri，版本，文件名
	char buf[MAXSIZE], method[MAXSIZE], uri[MAXSIZE], version[MAXSIZE];
	char fileName[MAXSIZE];
	rio_t rio;
	char *query_string;
	char cgiargs[MAXSIZE];
	

	rio_readinitb(&rio,fd);
	rio_readlineb(&rio,buf,MAXSIZE);
	//得到方法，uri，和版本信息
	sscanf(buf,"%s%s%s",method,uri,version);

    rio_readlineb(&rio, buf, MAXSIZE);
    while(strcmp(buf, "\r\n")) {
	    printf("%s", buf);
	    rio_readlineb(&rio, buf, MAXSIZE);
    }

	
	//判断方法，不是GET也不是POST则直接跳出
	if(strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		//错误处理，方法错误 
		wrong_req(fd,"501");
		return NULL;
	}
	else if(strcasecmp(method, "POST") == 0)
	{
		uri_fileName(uri, fileName);
		static_html(fd, fileName);
	}
	else if(strcasecmp(method, "GET") == 0)
	{
		
		query_string = uri;
		//遍历uri，找出是否有?后的条目
		while((*query_string != '?') && (*query_string != '\0'))
			query_string++;
		//uri如果有?
		if(*query_string == '?')
		{
			*query_string = '\0';
			query_string++;
			uri_fileName(uri, fileName);
			dynamic_html_get(fd,fileName,query_string);
		}
		else//静态页面
		{
			uri_fileName(uri, fileName);
			static_html(fd,fileName);			
		}
	}
	
		close(fd);
		return NULL;
}

void static_html(int fd,char *fileName)
{
	FILE *resource = NULL;  
    char buf[MAXSIZE]; 

    /*打开 filename 的文件*/  
    resource = fopen(fileName,"r"); 	
	if (resource == NULL)
	{
		wrong_req(fd, "404");
		return;
	}       
        /*写 HTTP header */  
    strcpy(buf, "HTTP/1.0 200 OK\r\n");  
    send(fd, buf, strlen(buf), 0);  
    /*服务器信息*/  
    strcpy(buf,"small web ");  
    send(fd, buf, strlen(buf), 0);  
    sprintf(buf, "Content-Type: text/html\r\n");  
    send(fd, buf, strlen(buf), 0);  
    strcpy(buf, "\r\n");  
    send(fd, buf, strlen(buf), 0);  
	
	
        /*复制文件*/  
     fgets(buf, sizeof(buf), resource);  
    while (!feof(resource))  
    {  
        send(fd, buf, strlen(buf), 0);  
        fgets(buf, sizeof(buf), resource);  
    }  
    fclose(resource);  
}


void uri_fileName(char *uri, char *fileName)
{
	char *p;
	strcpy(fileName,".");
	strcat(fileName,uri);
	if(uri[strlen(uri)-1] == '/')
		strcat(fileName,"index.html"); //index.html为服务器的初始页面
}




void dynamic_html_get(int fd, char *fileName, char *query_string) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };
    int pfd[2];
	FILE *resource = NULL;
	resource = fopen(fileName, "r");
	if (resource == NULL)
	{
		wrong_req(fd, "404");
		return;
	}
		
    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: weblet Web Server\r\n");
    rio_writen(fd, buf, strlen(buf));
 
    pipe(pfd);
    if (fork() == 0) {             /* child */
		close(pfd[1]);
		dup2(pfd[0],STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */
		execve(fileName, emptylist, environ);    /* Run CGI program */
    }

    close(pfd[0]);
    write(pfd[1], query_string, strlen(query_string)+1);
    wait(NULL);                          /* Parent waits for and reaps child */
    close(pfd[1]);
	fclose(resource);
}


