#define DEV_NAME "mycdrv"

#define ramdisk_size (size_t) (16 * PAGE_SIZE) // ramdisk size 

//data access directions
#define REGULAR 0
#define REVERSE 1

//NUM_DEVICES defaults to 3 unless specified during insmod
static int NUM_DEVICES = 10;

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CHGACCDIR _IOW(CDRV_IOC_MAGIC, 1, int)

struct asp_dev {
	struct list_head list;
	struct cdev dev;
	char *ramdisk;
	struct semaphore sem;
	int devNo;
	int dir; // 1 for reverse, 0 for normal

};
