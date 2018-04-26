#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

void main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("usage: %s, PID\n",argv[0]);
		return;
	}
	pid_t pid;
	pid = atoi(argv[1]);
	
	//发送信号
	kill(pid, SIGINT);
}
