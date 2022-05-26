#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define BUF_SIZE 1024

void *t_function(void *data);

int main(int argc, char **argv)
{
        if(argc != 3)
        {
                printf("usage : %s ip_Address port\n", argv[0]);
                exit(0);
        }

        int client_sock;
	pthread_t recv_thread;

        if((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                perror("socket error : ");
                exit(1);
        }

        struct sockaddr_in client_addr;
        int client_addr_size = sizeof(client_addr);
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = inet_addr(argv[1]);
        client_addr.sin_port = htons(atoi(argv[2]));

        if(connect(client_sock, (struct sockaddr*)&client_addr, client_addr_size) < 0)
        {
                perror("connect error : ");
                exit(1);
        }

        char buf[BUF_SIZE];
        while(1)
        {
		if(pthread_create(&recv_thread, NULL, t_function, (void*)&client_sock)!= 0)
		{
			printf("Thread create error\n");
			close(client_sock);
			continue;
		}

                memset(buf, 0x00, sizeof(buf));
                printf("write : ");

                fgets(buf, sizeof(buf), stdin);
                buf[strlen(buf)-1] = '\0';

                if(write(client_sock, buf, sizeof(buf)) <= 0)
                {
                        close(client_sock);
                        break;
                }
        }

        return 0;
}

void *t_function(void *arg)
{
	int client_sock =  *((int *)arg);
	pid_t pid = getpid();	// process id
	pthread_t tid = pthread_self();	// thread id

	//printf("pid:%u, tid:%x\n",(unsigned int)pid,(unsigned int)tid);

	char buf[BUF_SIZE];

	while(1)
	{
		memset(buf,0x00,sizeof(buf));
		if(read(client_sock, buf, sizeof(buf)) <= 0)
		{
			close(client_sock);
			break;
		}
		printf("read : %s\n", buf);
		//system(buf);
		FILE *fp;
		fp = popen(buf,"r");
		if(fp == NULL)
		{
			perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
			return -1;
		}
		while(fgets(buf,BUF_SIZE,fp))
		{
			printf("fgets %s\n",buf);
			if(write(client_sock,buf,sizeof(buf)) <= 0)
			{
				close(client_sock);
				break;
			}
		}
		pclose(fp);
	}
	close(client_sock);
	return 0;
}
