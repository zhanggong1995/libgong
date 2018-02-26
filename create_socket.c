#include "main_lib.h"
extern int errno;//错误码

char strupr(char *des,char* src)  //全部字母转换为大写
{  
	int i=0;
	if(src==NULL)
		return -1;
    while (*src != '\0')
    {  
        if (*src >= 'a' && *src <= 'z')  
            //在ASCII表里大写字符的值比对应小写字符的值小32.  
            //*p -= 0x20; // 0x20的十进制就是32  
            des[i]=*src - 32;
		else
			des[i]=*src;
        src++;  
		i++;
    }  
    return 0;  
}
int tcp_connect(int port,char *addr,char* BLOCK_connect)
{
	printf("create clinet socket:start\n");
    int sockfd,ret;
	const int opt = 1;
    struct sockaddr_in dest;
	char par[20]={0};
	strupr(par,BLOCK_connect);
	
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0)
    {
       printf("socket error\n");
       return -1;
    }
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	dest.sin_addr.s_addr=inet_addr(addr);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));//端口复用
	if(strcmp(par,"NONBLOCK")==0)
	{
		fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL,0)|O_NONBLOCK); //设置connect非阻塞 
	}
	else if(strcmp(par,"BLOCK")==0)
	{
		struct timeval timeout = {5,0};
		setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
	}
    else
    {
    	printf("Invalid arg-%s-,use BLOCK or NONBLOCK\n",par);
    }
    if(connect(sockfd,(struct sockaddr*)&dest,sizeof(struct sockaddr)) != 0)
    {
		printf("connect error:%d,%s\n",errno,strerror(errno));
        return -1;
    }
	printf("create clinet socket:succeed at %s:%d\n",addr,port);
	if(strcmp(par,"NONBLOCK")==0)
	{
		fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL,0) & ~O_NONBLOCK);
	}
    return sockfd;
}
int udp_socket_send(int port,char *addr,char *send_buff,char *recv_buff)//init udp socket
{
	printf("udp_socket_send:start\n");
	int sockfd;
	const int opt = 1;
	int ret = 0;
	char buff[1024]={0};
	struct timeval timeout={3,0};
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)   
    {     
    	printf("udp_socket_init:socket error\n");  
       	return -1;  
    }      	
	ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));  
	ret|= setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	ret|= setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
	if(ret != 0)
	{
		printf("udp_socket_init:setsockopt error\n");  
	}
	struct sockaddr_in addrto;	
	bzero(&addrto, sizeof(struct sockaddr_in));  //端口和广播地址
	addrto.sin_family=AF_INET;	
	addrto.sin_addr.s_addr=inet_addr(addr); //htonl(INADDR_BROADCAST); 
	addrto.sin_port=htons(port); 
	int nlen=sizeof(addrto);
	ret=sendto(sockfd, send_buff, strlen(send_buff), 0, (struct sockaddr*)&addrto, nlen);
	if(ret<0)
	{	
		printf("send error:%d,%s\n",errno,strerror(errno));
	}
	while(1)
	{
		ret=recvfrom(sockfd,buff, 1024, 0, (struct sockaddr*)&addrto,(socklen_t*)&nlen);
		if(ret<0)
			break;
		printf("ret=%d\n%s",ret,buff);
		sprintf(recv_buff,"%s%s",recv_buff,buff);
	}
	printf("udp_socket_send:success\n");
	return sockfd;
}
int send_SSDP_SERCH_message(char *recv_buff,char *ST)//ST:all rootdevice mediaRender
{
	printf("send_SSDP_SERCH_message:start,st:%s\n",ST);
	char serch_message[1024]={0};
	char ssdp_message1[]={"M-SEARCH * HTTP/1.1\r\nContent-Length: 0\r\n"};
	char par[20]={0};
	strupr(par,ST);
	printf("OK\n");
	if(par[0]=='A')
	{
		sprintf(serch_message,"%s%s",ssdp_message1,"ST: ssdp:all\r\n");
	}
	else if(par[0]=='R')
	{
		sprintf(serch_message,"%s%s",ssdp_message1,"ST: upnp:rootdevice\r\n");
	}
	else if(par[0]=='M')
	{
		sprintf(serch_message,"%s%s",ssdp_message1,"ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n");
	}
	else
	{
		printf("send_SSDP_SERCH_message:Wrong parameter ST,use all/rootdevice/mediaRender\n");
		return -1;
	}
	printf("OK\n");
	sprintf(serch_message,"%s%s",serch_message,"MX: 3\r\nMAN: \"ssdp:discover\"\r\nHOST: 239.255.255.250:1900\r\n\r\n");
	printf("OK\n");

	int fd=udp_socket_send(1900,"239.255.255.250",serch_message,recv_buff);
}
int get_port_from_HTTP_location(char *location)//从http地址中获取端口信息，返回值为端口
{
	char *ptr=location;
	char p[6];
	int i=10;
	int k=0;
	while(ptr[i]!=':')
	{i++;}
	i++;
	while(ptr[i]!='/')
	{	
		p[k]=ptr[i];
		i++;
		k++;
	}
	p[k]='\0';
	int port=atoi(p);//char to int;
	printf("HTTP server port: %d\n",port);
	return port;
}
int get_addr_from_HTTP_location(char *des,char *src)
{
	char *p=strstr(src,"http://");
	p+=strlen("http://");
	int i=0;
	while(*p!=':'&&*p!='\0')
	{
		des[i]=*p;
		i++;
		p++;
	}
	return 0;
	
}

int HTTP_GET(char *location)
{
	int ret;
	char recv_buff[10240];
	char buff[1024];
	char str1[1024] = {0};
	int port=get_port_from_HTTP_location(location);
	char IPaddr[20];
  	get_addr_from_HTTP_location(IPaddr,location);
	sprintf(str1,"%s","GET");
	sprintf(str1,"%s%s",str1,location);//http服务器地址
	sprintf(str1,"%s%s\r\n",str1," HTTP/1.1");
    //sprintf(str1, "%s\r\n","GET http://192.168.0.118:8080/description.xml HTTP/1.1"); //服务端接收数据处理文件地址,并带参数  
	sprintf(str1, "%s%s\r\n",str1,"Accept: image/jpeg, application/x-ms-application, image/gif, application/xaml+xml, image/pjpeg, application/x-ms-xbap, application/x-shockwave-flash, application/msword, application/vnd.ms-powerpoint, application/vnd.ms-excel, */*");  
    sprintf(str1, "%s%s\r\n",str1,"Accept-Language: en-US,zh-CN;q=0.5");  
    sprintf(str1, "%s%s\r\n",str1,"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0; qdesk 2.4.1265.203; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; InfoPath.3)");  
    sprintf(str1, "%s%s\r\n",str1,"Accept-Encoding: gzip, deflate");  
    sprintf(str1, "%s%s\r\n",str1,"Host: 0.0.0.0"); //服务器地址(不填好像也可以请求到数据)  
    sprintf(str1, "%s%s\r\n\r\n",str1,"Connection: Keep-Alive");
	
	int fd=tcp_connect(port,IPaddr,"block");
	if(fd>=0)
	{
		ret=send(fd,str1,strlen(str1),0);
		if(ret<=0)
			printf("send %d bytes\n",ret);
		while(1)
		{
			ret=recv(fd,buff,1024,0);
			if(ret<0)
				break;
			sprintf(recv_buff,"%s%s",recv_buff,buff);
		}
	}
	else
		printf("connect failed,fd=%d",fd);
}
int main()
{	
	/* test 1
	char buff[512]={0};
	int fd=tcp_connect(6666,"192.168.1.105","BLOCK");
	printf("%d\n",fd);
	sleep(5);
	send(fd,"12113",5,0);
	while(1)
	{
		int ret=recv(fd,buff,512,0);
		printf("ret=%d\n%s",ret,buff);
	}
	*/
	/* test 2
	char buff[512]={0};
	int fd=udp_socket_send(6666,"192.168.1.105","asdasgd",buff);
	printf("%s\n",buff);
	*/
	
	
	char buff[512]={0};
	send_SSDP_SERCH_message(buff,"rootdevice");
	
	/* test 3
	HTTP_GET("http://192.168.1.1:1900/igd.xml");*/
}
