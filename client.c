#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define BUF_SIZE 1024

void *t_function(void *data);
void *t_function2(void *data);

pthread_mutex_t mutex;

int main(int argc, char **argv)
{
//	pthread_mutex_init(&mutex,NULL);

        if(argc != 3)
        {
                printf("usage : %s ip_Address port\n", argv[0]);
                exit(0);
        }

        int client_sock;

	pthread_t recv_for_popen_thread;
	pthread_t recv_for_print_thread;

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
	char buf2[BUF_SIZE];
        while(1)
        {
		memset(buf, 0x00, sizeof(buf));
                printf("write : ");
		
		if(pthread_create(&recv_for_print_thread, NULL, t_function2, (void*)&client_sock)!=0)
		{
			printf("Thread Create error\n");
			close(client_sock);
			continue;
		}
		
                if (fgets(buf, sizeof(buf), stdin)!=NULL)
		{
	             	buf[strlen(buf)-1] = '\0';

	                if(write(client_sock, buf, sizeof(buf)) <= 0)
        	        {
                	        close(client_sock);
                       		break;
             		}

			if(pthread_create(&recv_for_popen_thread, NULL, t_function, (void*)&client_sock)!= 0)
			{
				printf("Thread create error\n");
				close(client_sock);
				continue;
			}
		}
        }
	//pthread_mutex_destroy(&mutex);
        return 0;
}

void *t_function(void *arg)
{
	int client_sock =  *((int *)arg);
	pid_t pid = getpid();	// process id
	pthread_t tid = pthread_self();	// thread id

	//printf("pid:%u, tid:%x\n",(unsigned int)pid,(unsigned int)tid);

	char buf[BUF_SIZE];
	char buf2[BUF_SIZE];
	while(1)
	{
		//pthread_mutex_lock(&mutex);
		memset(buf,0x00,sizeof(buf));
		memset(buf2,0x00,sizeof(buf2));
		strcpy(buf2,"Command");
		if(write(client_sock, buf2, sizeof(buf2)) <= 0)
		{
			close(client_sock);
			break;
		}

		if(read(client_sock, buf, sizeof(buf)) <= 0)
		{
			close(client_sock);
			break;
		}

		printf("Client First read : %s\n",buf);
		// Command 만 읽기 from other client (서버측에서 클라이언트 자기 자신 외에 보내게 되어있음) 
		if (strcmp(buf,"Command_Enter")==0)
		{
			memset(buf,0x00,sizeof(buf));
			if(read(client_sock, buf, sizeof(buf)) <= 0)
			{
				close(client_sock);
				break;
			}
			printf("Client Command read (from other client) : %s\n", buf);
		}
	}
	close(client_sock);
	return 0;
}

void *t_function2(void *arg)
{
	int client_sock =  *((int *)arg);
	pid_t pid = getpid();	// process id
	pthread_t tid = pthread_self();	// thread id
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

		FILE *fp;
		fp = popen(buf,"r");
		if(fp == NULL)
		{
			perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
			return -1; // return -1;
		}
		while(fgets(buf,BUF_SIZE,fp))
		{
			//memset(buf2,0x00,sizeof(buf2));
			//strcpy(buf2,"Print_Result");
			//if(write(client_sock,buf2,sizeof(buf2)) <= 0)
			//{
			//	close(client_sock);
			//	break;
			//}
			printf("%s\n",buf);
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
