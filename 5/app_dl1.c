#include <linux/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>

#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IOWR(CDRV_IOC_MAGIC, 1, int)
#define E2_IOCMODE2 _IOWR(CDRV_IOC_MAGIC, 2, int)

#define MSG ""
#define PRINT_DEBUG(fmt) printf("%s:%d - %s\n",__func__, __LINE__, fmt);
#define DEV_PATH "/dev/t5"


void *thread1(void *arg)
{
	int fd;
	int rc;
	PRINT_DEBUG("Function Start, Opening device");
	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) { 
		perror("Unable to open device\n");
		return;
	}
	PRINT_DEBUG("Open Succesful, Callling IOCTL, change to Mode2");

	sleep(4);
	PRINT_DEBUG("Callling IOCTL, To change to mode2");

	rc = ioctl(fd, E2_IOCMODE2, 0);
	if (rc == -1) { 
		perror("IOCTL Error\n");
		return;
	}
	PRINT_DEBUG("IOCTL to mode2 succesful");
	//sleep(10);
	close(fd);
	PRINT_DEBUG("Device closed");
	return;
}

void *thread2(void *arg)
{
	int fd;
	int rc;

	PRINT_DEBUG("Start of Function");
	sleep(2);
	PRINT_DEBUG("Opening Device....");
	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0){ 
		perror("Unable to open device");
		return;
	}
	PRINT_DEBUG("Open Succesful, Calling Close");
	close(fd);
	PRINT_DEBUG("Closed Device");
	return;
}



int main(int argc, char *argv[]) {

	pthread_t th_1;
	pthread_t th_2;

	pthread_create(&(th_1), NULL, thread1, NULL);
	pthread_create(&(th_2), NULL, thread2, NULL);
	pthread_join(th_2, NULL);
	pthread_join(th_1, NULL);
	
	
	return 0;
}
