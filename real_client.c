#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define BUF_SIZE 1024

void *t_read_function(void *data);
void *t_write_function(void *data);
pthread_mutex_t mutex;

typedef struct For_W_th_struct
{
	int sock_Num;
	char buf[BUF_SIZE];
}For_W_th_struct;

int main(int argc, char **argv)
{
	For_W_th_struct *for_w_th;
	pthread_mutex_init(&mutex,NULL);

        if(argc != 3)
        {
                printf("usage : %s ip_Address port\n", argv[0]);
                exit(0);
        }

        int client_sock;
	pthread_t recv_thread;
	pthread_t write_thread;
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
	void *retval;	// pthread join test용 변수 
        while(1)
        {
		if(pthread_create(&recv_thread, NULL, t_read_function, (void*)&client_sock)!= 0)
		{
			printf("Thread create error\n");
			close(client_sock);
			continue;
		}

                printf("write : \n");
		memset(buf,0x00,sizeof(buf));
		
		if(fgets(buf, sizeof(buf), stdin)!=NULL)
                {
                        pthread_mutex_lock(&mutex);
                        buf[strlen(buf)-1] = '\0';
                        memset(buf2,0x00,sizeof(buf2));
                        strcpy(buf2,"Command");
                        if(write(client_sock,buf2,sizeof(buf2)) <= 0)
                        {
                               close(client_sock);
                               break;
                        }
                        memset(buf2,0x00,sizeof(buf2));
                        if(read(client_sock, buf2, sizeof(buf2)) <= 0)
                        {
                                close(client_sock);
                                break;
                        }
                        printf("buf2 : %s\n",buf2);
                        printf("buf2 :%s\n",buf2);

                        if (strcmp(buf2,"Command_Enter")==0)
                        {
                                printf("buf :%s\n",buf);
                                // 유저가 입력한 명령어를 보낸다.
                                if(write(client_sock,buf,sizeof(buf)) <= 0)
                                {
                                       close(client_sock);
                                       break;
                                }
				pthread_mutex_unlock(&mutex);
                        }
                       //pthread_mutex_unlock(&mutex);
                }

		//if(fgets(buf, sizeof(buf), stdin)!=NULL)
		//{
		//	for_w_th = (For_W_th_struct*)malloc(sizeof(For_W_th_struct));
		//	for_w_th->sock_Num = client_sock;
		//	strcpy(for_w_th->buf,buf);
		//	if(pthread_create(&write_thread, NULL, t_write_function, (void*)for_w_th)!= 0)
                //	{
                //	     	 printf("Thread create error\n");
                //       	 close(client_sock);
                //       	 continue;
                //	}
		//}
        }
	pthread_mutex_destroy(&mutex);
        return 0;
}

void *t_read_function(void *arg)
{
	int client_sock =  *((int *)arg);
	pid_t pid = getpid();	// process id
	pthread_t tid = pthread_self();	// thread id

	//printf("pid:%u, tid:%x\n",(unsigned int)pid,(unsigned int)tid);

	char buf[BUF_SIZE];
	char buf2[BUF_SIZE];
	//pthread_mutex_lock(&mutex);
	while(1)
	{
		//pthread_mutex_lock(&mutex);
		// 처음 입력한 명령어는 여기로 들어오게 된다. ex) ls, pwd
		memset(buf,0x00,sizeof(buf));
		if(read(client_sock, buf, sizeof(buf)) <= 0)
		{
			close(client_sock);
			break;
		}
		printf("Client Command First read (from other client) : %s\n", buf);

		// 프린트 출력인 경우에만 , 
		if (strcmp(buf,"Print_Result_Enter")==0)
		{
			memset(buf,0x00,sizeof(buf));
                        if(read(client_sock, buf, sizeof(buf)) <= 0)
                        {
                                close(client_sock);
                                break;
                        }
                        printf("read Command Result[from other client] : %s\n",buf);
		}
		// 그 이외의 상황 
		else
		{
		//printf("Client First read : %s\n",buf);
		// Command 만 읽기 from other client (서버측에서 클라이언트 자기 자신 외에 보내게 되어있음) 
		FILE *fp;
		fp = popen(buf,"r");
		if(fp == NULL)
		{
			perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
			return -1; // return -1;
		}
		while(fgets(buf,BUF_SIZE,fp))
		{
			memset(buf2,0x00,sizeof(buf2));
			strcpy(buf2,"Print_Result");
			if(write(client_sock,buf2,sizeof(buf2)) <= 0)
			{
				close(client_sock);
				break;
			}
			//printf("%s\n",buf);
			if(write(client_sock,buf,sizeof(buf)) <= 0)
			{
				close(client_sock);
				break;
			}
		}
		pclose(fp);
		}
	}
	//pthread_mutex_unlock(&mutex);
	close(client_sock);
	return 0;
}

void *t_write_function(void *arg)
{
	//int client_sock =  *((int *)arg);
	For_W_th_struct *st = (For_W_th_struct *) arg;
	char buf[BUF_SIZE];
	char buf2[BUF_SIZE];
	int client_sock = st->sock_Num;
	strcpy(buf,st->buf);

	while(1)
	{
                pthread_mutex_lock(&mutex);
                buf[strlen(buf)-1] = '\0';
                memset(buf2,0x00,sizeof(buf2));
                strcpy(buf2,"Command");
                if(write(client_sock,buf2,sizeof(buf2)) <= 0)
	        {
                       close(client_sock);
                       break;
                }
                memset(buf2,0x00,sizeof(buf2));
                if(read(client_sock, buf2, sizeof(buf2)) <= 0)
                {
                        close(client_sock);
                        break;
                }
                printf("buf2 : %s\n",buf2);
                printf("buf2 :%s\n",buf2);
		pthread_mutex_unlock(&mutex);
                if (strcmp(buf2,"Command_Enter")==0)
                {
                        printf("buf :%s\n",buf);
                        // 유저가 입력한 명령어를 보낸다.
                        if(write(client_sock,buf,sizeof(buf)) <= 0)
                        {
                               close(client_sock);
                               break;
                        }
                }
                //pthread_mutex_unlock(&mutex);
	}
	close(client_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
