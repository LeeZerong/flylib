#include <stdio.h>
#include <stdlib.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define D_SIZE 128
#define B_SIZE 1024
#define F_SIZE 1024*10

int  read_data(char *data, int *len);	//从环形缓冲区中获取数据
int  write_file(char *buf);				//将缓冲区的数据写入磁盘
void ouch(int sig);						//信号处理函数

char buf[B_SIZE+1] = {0};	//缓冲区

int main()
{
	int len = 0;	//获取到的数据长度
	int cur = 0;	//指针的当前位置
	int n = 0;		//缓冲区所剩空间与数据长度的差值
	char data[D_SIZE+1] = {0};	//数据
	int res;
	
	int semid;
	struct sembuf sops;
	key_t key;
	
	key = ftok("/home", 1);				//创建键值
	semid = semget(key, 1, IPC_CREAT);	//创建信号量
	semctl(semid, 0, SETVAL, 0);		//设置信号量初始值
	
	while(1)
	{
		//清空数据存取空间
		memset(data, 0, D_SIZE + 1);
		
		//从环形缓冲区中获取数据
		if(0 > (res = read_data(data, &len)))
		{
			printf("read_data error!\n");
			return -1;
		}
		
		//缓冲区未满，且有足够空间存放整个数组
		if(cur + len <= B_SIZE)
		{
			strncpy(&(buf[cur]), data, len);
			printf("buf: %s\n", &buf[cur]);
			cur += len;
		}
		
		//缓冲区未满，但空间不足存放整个数组
		if(cur + len > B_SIZE)
		{
			n = cur + len - B_SIZE;
			
			//将上半段写入缓冲区
			strncpy(&(buf[cur]), data, len-n);
			//将缓冲区的数据写入文件
			write_file(buf);
			
			//清空缓冲区
			memset(buf, 0, sizeof(buf));
			cur = 0;
			
			//将下半段写入缓冲区
			strncpy(&(buf[cur]), &(data[len-n]), n);
		}
		
		//缓冲区已满
		if(cur == B_SIZE)
		{
			//将缓冲区的数据写入磁盘
			write_file(buf);
			//清空缓冲区
			memset(buf, 0, sizeof(buf));
			cur = 0;
		}
		printf("cur : %d\n", cur);
		
		//处理外部信号
		signal(SIGINT, ouch);
						
		//释放信号量
		sops.sem_num = 0;
		sops.sem_op  = 1;
		semop(semid, &sops, 1);
	}
}

int read_data(char *data, int *len)
{
	if(data == NULL)
	{
		printf("data erroc!\n");
		return -1;
	}
		
	
	if(0 > (*len = klogctl(2, data, D_SIZE)))
	{
		perror("klogctl error");
		return -2;
	}	
}

int write_file(char *wbuf)
{
	int n = 0;				//文件所剩空间与缓冲区大小的差值
	char info[50] = {0};	//配置文件的信息
	int offset = 0;			//文件指针的偏移量
	int len = strlen(wbuf);	//写入数据的长度
	int fd = open("./log.txt", O_RDWR | O_CREAT, 0777);
	int fd_info = open("./info.txt", O_RDWR | O_CREAT, 0777);
	int err = 0;
	
	//打开文件
	if(fd < 0 || fd_info < 0)
	{
		perror("open file error");
		err = -1;
		return err;
	}
	
	//读取配置文件
	read(fd_info, info, 50);
	sscanf(info, "the file port in %d", &offset);
	printf("offset = %d\n", offset);
	
	//已经达到文件的限定大小
	if(offset == F_SIZE || offset == 0)
	{
		//将缓冲区的数据写入文件
		if(0 > write(fd, wbuf, len))
		{
			perror("write file error");
			err = -1;
			goto end;
		}
	}else{	//还未达到文件的限定大小
		//移动文件指针
		lseek(fd, offset, SEEK_SET);
		
		//文件足够容纳缓存区的数据
		if(len + offset < F_SIZE)
		{
			//将缓冲区的数据写入文件
			if(0 > write(fd, wbuf, len))
			{
				perror("write file error");
				err = -1;
				goto end;
			}
		}
		else
		{	//文件不足以容纳整个缓冲区的数据
			n = len + offset - F_SIZE;
			if(0 > write(fd, wbuf, len-n))
			{
				perror("write file error");
				err = -1;
				goto end;
			}
			lseek(fd, 0, SEEK_SET);
			if(0 > write(fd, &(wbuf[len-n]), n))
			{
				perror("write file error");
				err = -1;
				goto end;
			}
		}
	}
	
	//记录文件指针的位置
	offset = lseek(fd, 0, SEEK_CUR);
	lseek(fd_info, 0, SEEK_SET);
	sprintf(info,  "the file port in %d", offset);
	write(fd_info, info, 50);
	
end:
	close(fd_info);
	close(fd);
	return err;
}

void ouch(int sig) 
{ 
	int semid;
	struct sembuf sops;
	key_t key;
	char wbuf[B_SIZE+1] = {0};
	
	key = ftok("/home", 1);				//创建键值
	semid = semget(key, 1, IPC_CREAT);	//创建信号量
	semctl(semid, 0, SETVAL, 0);		//设置信号量初始值
	
	//获取信号量
	sops.sem_num = 0;
	sops.sem_op  = -1;
	sops.sem_flg = SEM_UNDO;
	semop(semid, &sops, 1);
	
	strncpy(wbuf, buf, B_SIZE+1);
	printf("\n get signal %d, write file!\n", sig); 
	write_file(wbuf);
} 

