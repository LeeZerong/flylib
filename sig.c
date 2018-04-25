#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

void main(int argc, char *argv[])
{
	pid_t pid;
	pid = atoi(argv[1]);
	
	//发送信号
	kill(pid, SIGINT);
}
