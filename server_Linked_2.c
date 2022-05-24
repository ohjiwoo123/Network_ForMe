#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define MAX_CLIENT 5
#define BUF_SIZE 1024

void *t_function(void *data);
void *t_PrintUI(void *data);

int client_index = 0;
int g_sockList[5];

pthread_t thread_client[MAX_CLIENT];
pthread_t thread_PrintUI;
pthread_mutex_t mutex;

typedef struct socket_List_NODE
{
	int sock_Num;
	struct socket_List_NODE *next;
}socket_List_NODE;

socket_List_NODE* socket_List_Start;

void Add_socket_List(struct socket_List_NODE *target,int value)
{
	struct socket_List_NODE *new_node = malloc(sizeof(struct socket_List_NODE));
	new_node->next = target->next;
	new_node->sock_Num = value;
	target->next = new_node;
	client_index++;
};

typedef struct connect_List_NODE
{
	struct connect_List_NODE *pNext;
	char connect_List[50];
}connect_List_NODE;

typedef struct getHistory_List_NODE
{
	struct getHistory_List_NODE *pNext;
	char command_List[30];
}getHistory_List_NODE;

typedef struct disconnect_List_NODE
{
	struct disconnect_List_NODE *pNext;
	char disconnect_List[50];
}disconnect_List_NODE;

int main(int argc, char **argv)
{
	struct socket_List_NODE *pHead = malloc(sizeof(struct socket_List_NODE));
	pHead->next == NULL;

	printf("Server Start\n");
	pthread_mutex_init(&mutex,NULL);

	int nMenu = 0;
        if (argc != 2)
        {
                printf("Usage : %s [port]\n", argv[0]);
                return 1;
        }

        int server_sock, client_sock;

        if((server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
        {
                printf("socket create error\n");
                return -1;
        }

        int on = 1;
        if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        {
                printf("socket option set error\n");
                return -1;
        }

        struct sockaddr_in server_addr, client_addr;
        int client_addr_size = sizeof(client_addr);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(atoi(argv[1]));

        if(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 )
        {
                printf("bind error\n");
                return -1;
        }

        if(listen(server_sock, 5) < 0)
        {
                printf("listen error\n");
                return -1;
        }

        while(1)
        {
		if(pthread_create(&thread_PrintUI,NULL,t_PrintUI,(void*)&pHead) !=0 )
		{
			printf("PrintUI_Thread create error\n");
			continue;
		}
                printf("accept...\n");

                client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
                if(client_sock < 0)
                {
                        printf("accept error\n");
                }

                if(client_index == MAX_CLIENT)
                {
                        printf("client accept full(max client count : %d)\n", MAX_CLIENT);
                        close(client_sock);
                        continue;
                }

		// 링크드 리스트 위한 소켓리스트 목록에 추가 
		Add_socket_List(pHead,client_sock);
		//g_sockList[client_index] = client_sock;

                if(pthread_create(&thread_client[client_index], NULL, t_function, (void *)&client_sock) != 0 )
                {
                        printf("Client_Thread create error\n");
                        close(client_sock);
                        continue;
                }

                //client_index++;

                printf("client accepted(Addr: %s, Port: %d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		printf("연결 소켓번호 : %d\n",socket_List_Start->sock_Num);

        }

	pthread_mutex_destroy(&mutex);
        return 0;

}

void *t_function(void *arg)
{
        int client_sock = *((int *)arg);
        pid_t pid = getpid();      // process id
        pthread_t tid = pthread_self();  // thread id

        printf("pid:%u, tid:%x\n", (unsigned int)pid, (unsigned int)tid);

        char buf[BUF_SIZE];

        while(1)
        {
                memset(buf, 0x00, sizeof(buf));
                if (read(client_sock, buf, sizeof(buf)) <= 0)
                {
                        printf("Client %d close\n", client_sock);
                        client_index--;
                        close(client_sock);
                        break;
                }

                printf("read : %s\n", buf);

                if(write(client_sock, buf, sizeof(buf)) <= 0)
                {
                        printf("Client %d close\n", client_sock);
                        client_index--;
                        close(client_sock);
                        break;
                }

		for (int i=0; i<client_index;i++)
		{
			if (g_sockList[i] == client_sock)
			{
				continue;
			}
			if(write(g_sockList[i], buf, sizeof(buf)) <=0)
			{
				printf("Client %d close\n", g_sockList[i]);
				client_index--;
				close(g_sockList[i]);
				break;
			}
			printf("write : %s\n",buf);
		}
        }

}

void *t_PrintUI(void *arg)
{
	int nMenu = 0;
	socket_List_NODE* pHead = ((socket_List_NODE *)arg);
	pthread_mutex_lock(&mutex);
	while((nMenu = PrintUI()) !=0)
	{
		switch(nMenu)
		{
			case 1:
				getList(pHead);
				break;
			case 2:
				disConnect();
				break;
			case 3:
				getHistory();
				break;
			case 4:
				getMenu();
				break;
		}
	}
	pthread_mutex_unlock(&mutex);
}

int PrintUI()
{
	int nInput = 0;
	// system("cls");
	printf("===================================================\n");
	printf("서버 Start\n");
	printf("---------------------------------------------------\n");
	printf("[1] 연결현황출력\t [2] 연결종료\t [3] 명령어기록보기\t [4] 전체보기\t");
	printf("===================================================\n");

	// 사용자가 선택한 메뉴의 값을 반환한다.
	scanf("%d", &nInput);
	//getchar();
	// getchar();//버퍼에 남은 엔터 제거용
	return nInput;
}

void getList(struct socket_List_NODE* pHead)
{
	socket_List_NODE* curr = pHead->next;
	if (curr == NULL)
	{
		printf("연결 된 소켓이 없습니다.\n");
	}
	else
	{
		while(curr != NULL)
		{
			printf("연결된 소켓 번호 : %d\n", curr->sock_Num);
			curr = curr->next;
		}
	}
}

void disConnect()
{
	socket_List_NODE* pHead = socket_List_Start;
	int index;
	printf("삭제할 소켓 번호를 입력하세요 : ");
	scanf("%d",&index);
	if (pHead == NULL)
	{
		printf("연결 된 소켓이 없습니다.\n");
	}
	else
	{
		close(index);
		printf("%d 번 소켓 연결이 종료되었습니다.\n",index);
		client_index--;
	}

}

void getHistory()
{
	printf("명령어보낸 모음 ex)ls -al pwd cp A B 등 \n");
}

void getMenu()
{
	printf("위의 메뉴가 다 보이게 하자 \n");
	//getList();
	//disConnect();
	//getHistory();
}
