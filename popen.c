#include <stdio.h>

#define BUFF_SIZE 1024

int main(void)
{
	char buff[BUFF_SIZE];
	FILE *fp;

	fp = popen("ls -al","r");
	if (fp == NULL)
	{
		perror("popen() 실패");
		return -1;
	}
	while(fgets(buff,BUFF_SIZE,fp))
	{
		printf("%s\n",buff);
		printf("사이즈 : %d\n",sizeof(buff));
	}
	pclose(fp);
	return 0;
}
