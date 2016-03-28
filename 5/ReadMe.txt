Assignment 5

----------------------
Files Provided:
------------------------
1. assignment5.c
2. app_dl1.c
3. appl_dl2.c
4. ReadMe.txt
5. MakeFile

----------------------------------------
Instructions to test module:-
----------------------------------------
1) Compile driver module : $ make

2) Load module : $ sudo insmod assignment5.ko

3) Test driver :
	1) Compile userapp : $ make app
	2) Run userapp : $ sudo ./app_dl1
					 $ sudo ./app_dl2
	
	Note : userapp has to be executed with sudo privilege as the device files
		   in /dev/ are created  in the driver with root privileges.
		   
4) Unload module : $ sudo rmmod assignment5

-----------------------------------------------------
DeadLocks
----------------------------------------------------
1) 	There are two threads in this user program
	Thread1 opens the device in Mode1 (default mode)
	Thread 2 opens the device in Mode1.
	Now, Thread1 tries to change to Mode2 using IOCTL system call
	But, It creates deadlock because Thread 1 grabbed sem2 and Thread 2 gets blocked on sem2
	and Thread1 waits for count1 == 1 condition. Since, count1 is 2 and It can be reduced only 
	in thread2 when it releases. But thread2 is blocked in sem2 creating deadlock.
	
	app_dl1.c has code for this scenario
	
2) 	There are two threads in this user program
	Thread1 opens the device in Mode1 (default mode). Thread1 changes to Mode2(Using IOCTL)
	Thread2 opens the device. Its in Mode2 now (Thread1 changed it to Mode2).
	Now, Thread1 and Thread2 both call IOCTL to change the mode to Mode1.
	Since count2 = 2, while (devc->count2 > 1) {..} is blocked and both threads are in 
	wait queue waiting for condition count2 ==1 to get true, Which doesn't happen because 
	both the threads can't release the device. Therefore, Deadlock is created in this situation.
	
	app_dl2.c has code for this scenario

-----------------------------------------------------
Race Conditions: 
----------------------------------------------------
We will do review for critical regions in open, release, read, write
and IOCTL and see if there are any race conditions, If at all, There are,
Will it affect the behaviour of device driver or not. Comments are given in code
to show the understanding

1) Open: 
	int e2_open(struct inode *inode, struct file *filp)
	{
	struct e2_dev *ev;
	dev = container_of(inode->i_cdev, struct e2_dev, cdev);
	filp->private_data = dev;
	down_interruptible(&dev->sem1);
		//Critical Region starts
		if (dev->mode == MODE1) { //dev->mode is a shared variable
		dev->count1++;  //Shared var
		up(&dev->sem1);
		down_interruptible(&dev->sem2); //This ensures, Only one thread enters in Mode1
		return;
		}
		else if (dev->mode == MODE2) {
		dev->count2++;
		}
		//Critical Region ends
	up(&dev->sem1);
	}
	Comments:
	--------
	As we can see, sem1 is locked before entering Critical Section. It ensures that count1, count2 and mode
	are accesed only by single thread before updating. If it is Mode1, Only one thread returns from 
	Open and Other threads are blocked on Sem2 In this, As we  can see, There are no race conditions at all


2) Release: 
	int e2_release(struct inode *inode, struct file *filp)
	{
	.....
	down_interruptible(&dev->sem1);
		//Critical Region begins
		if (dev->mode == MODE1) { //dev->mode is a shared variable
			dev->count1--; //count1 is shared variable, Hence in critical region
			if (dev->count1 == 1)
			wake_up_interruptible(&dev->queue1); //If there is only one device, Wake up thread waiting on queue1
			up(&dev->sem2); //Makes sure thread waiting on sem2 returns from open.
		}
		else if (dev->mode == MODE2) {
		dev->count2--; //Shared var
		if (dev->count2 == 1)
		wake_up_interruptible(&dev->queue2); //If there is only one device, Wake up thread waiting on queue2
		}
		//Critical Region ends

	up(&dev->sem1);
	}
	Comments:
	--------
	As we can see, sem1 is locked before entering Critical Section. It ensures that count1, count2 and mode
	are accesed only by single thread before updating. If it is Mode1, Decrement count1 and if only one device left,
	Wake up the thread waiting on queue1, So that it can procced in IOCTL, up the sem2 because next thread waiting on 
	Open can access the device. Sem2 makes sure that only thread accesses the device in mode1.In this, 
	As we  can see, There are no race conditions at all because of locking sem1 before entering Critical Section.
	
3) Read and Write:
	static ssize_t e2_read (...)
	{	
		struct e2_dev *dev = filp->private_data;
		//Critical Section starts
		down_interruptible(&dev->sem1);
		if (dev->mode == MODE1) { //mode is shared. 
			up(&dev->sem1); //up sem1, then read/Write because, we know only thread has access to device in mode1
			// read/write
		} else {
			// read/write
			up(&dev->sem1); //read/Write, then up sem1  because, we know multiple threads have access to device in mode2
							//This locking mechanism ensures that there is no race condition while reading and writing
		}
	}
	Comments:
	--------
	As we can see, sem1 is locked before entering Critical Section. It ensures that mode variable
	is accesed only by single thread before checking. If it is Mode1, up sem1, then read/Write because, 
	we know only thread has access to device in mode1, If it is in Mode2,  read/Write, then up sem1 
	because, we know multiple threads have access to device in mode2. It ensures that only one thread 
	reads/Writes in mode2. As we  can see, There are no race conditions at all because of locking sem1 
	before entering Critical Section.
	
	
	
4) IOCTL:

	Below code snippet is for changing to Mode2. Similar Analysis is applicable to Mode1
	down_interruptible(&dev->sem1);
	//Critical Region starts
		if (dev->mode == MODE2) {
			up(&dev->sem1);
			break;
		}
		//Mode1
		if (dev->count1 > 1) {
			while (dev->count1 > 1) {
			//Wait till dev->count1 becomes 1
				up(&dev->sem1);
				wait_event_interruptible(dev->queue1, (dev->count1 == 1));
				down_interruptible(&dev->sem1);
			}
		}
		dev->mode = MODE2;
		up(&dev->sem2);
	//Critical Region ends

	up(&dev->sem1);

	Comments:
	--------
	As we can see, sem1 is locked before entering Critical Section. It ensures that mode variable
	is accesed only by single thread before checking. If it is in Mode2, up sem1, and then exit
	If it is in Mode1,  wait for dev->count1 to become ==1, once count1 becomes 1, we can change the mode 
	to mode2 and count1 then becomes 0 and count2 becomes 1. All this operations are done while sem1 is locked,
	So, there are no race conditions. But within while loop, We do up(sem1). Here, When count1 is decremented by other thread 
	after putting in wait queue, then that condition will be checked in next iteration of while loop, 
	Though it might seem there is a race condition, It is benign and it doesnt exist for long time
	
	For changing mode2 to mode1, We have already discussed about the deadlock, If multiple threads try to do ioctl 
	to mode1 at same time.
	
	Therefore, There is one benign race condition in above code.
	
--------------------
Sample output for dl1:
------------------
:~/Desktop/asp/assignments/5$ sudo ./app_dl1
thread2:52 - Start of Function
thread1:24 - Function Start, Opening device
thread1:30 - Open Succesful, Callling IOCTL, change to Mode2
thread2:54 - Opening Device....
thread1:33 - Callling IOCTL, To change to mode2
.....................Stuck.............................

