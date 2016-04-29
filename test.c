#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/delay.h>


MODULE_LICENSE("GPL");


#define MS_TO_NS(x) 	(x * 1E6L) //because ktime_set works with ns and user input is in ms

/* 	I need to set up four timers to monitor tasks at different frequencies 	*/
static struct hrtimer timer1Hz, timer10Hz, timer100Hz;

static struct task_struct *task1Hz, *task10Hz, *task100Hz, *exitThread;

static char dispatchFlag; 

enum DISP_FLAGS {

	ONE_HZ_FLAG = 0,
	TEN_HZ_FLAG,
	HUN_HZ_FLAG,
	OVERRUN_FLAG

};


/* User Functions */
static void oneHzFunc( void* p ) {

	printk( "One Hertz Task\n" );
	msleep(2000);
	return;

}

static void tenHzFunc( void* p ) {

	printk( "Ten Hertz Task\n" );
	return;
	
}

static void hunHzFunc( void* p ) {

	printk( "Hundred Hertz Task\n" );
	return;
	
}

/* Semaphore Definition */
DEFINE_SEMAPHORE( oneHzSemph );
DEFINE_SEMAPHORE( tenHzSemph );
DEFINE_SEMAPHORE( hunHzSemph );
DEFINE_SEMAPHORE( exitSemph );

/* Timer Callbacks */

/* 1Hz Timer Callback */
enum hrtimer_restart timer1HzCallback( struct hrtimer* sample ) {

	ktime_t k1Hz;

	if(dispatchFlag & (1 << ONE_HZ_FLAG) && !(dispatchFlag & (1 << OVERRUN_FLAG))) {

		dispatchFlag &= ~(1 << ONE_HZ_FLAG);

		up( &oneHzSemph );
		/* Check if my 1Hz Task is complete. If it is complete, handover the semaphore and restart timer*/
		//printk( "1 Hz Timer Callback initiated\n");
		k1Hz  = ktime_set( 0, MS_TO_NS(1000) ); 	//1000ms -> 1hz
		hrtimer_forward_now(sample, k1Hz);
		return HRTIMER_RESTART;
		}

	else
		{
			dispatchFlag |= (1 << OVERRUN_FLAG);	
			up ( &exitSemph );
			return HRTIMER_NORESTART;
		}

}

/* 10Hz Timer Callback */
enum hrtimer_restart timer10HzCallback( struct hrtimer* sample ) {

	ktime_t k10Hz;

	if(dispatchFlag & (1 << TEN_HZ_FLAG) && !(dispatchFlag & (1 << OVERRUN_FLAG))) {

		dispatchFlag &= ~(1 << TEN_HZ_FLAG);

		up( &tenHzSemph );
		/* Check if my 1Hz Task is complete. If it is complete, handover the semaphore */
		//printk( "10 Hz Timer Callback initiated\n");
		k10Hz = ktime_set( 0, MS_TO_NS(100) ); 	//100ms -> 10hz
		hrtimer_forward_now(sample, k10Hz);
		return HRTIMER_RESTART;
	}
	
	else {
			dispatchFlag |= (1 << OVERRUN_FLAG);	
			up ( &exitSemph );
			return HRTIMER_NORESTART;
	}

}

/* 100Hz Timer Callback */
enum hrtimer_restart timer100HzCallback( struct hrtimer* sample ) {

	ktime_t k100Hz;

	if(dispatchFlag & (1 << HUN_HZ_FLAG) && !(dispatchFlag & (1 << OVERRUN_FLAG))) {

		dispatchFlag &= ~(1 << HUN_HZ_FLAG);

		up( &hunHzSemph );

		/* Check if my 1Hz Task is complete. If it is complete, handover the semaphore */
		//printk( "100 Hz Timer Callback initiated\n");
		k100Hz = ktime_set( 0, MS_TO_NS(10) ); 	//10ms -> 100hz
		hrtimer_forward_now(sample, k100Hz);
		return HRTIMER_RESTART;
	}
	else {
		dispatchFlag |= (1 << OVERRUN_FLAG);
		up ( &exitSemph );
		return HRTIMER_NORESTART;
	}
	
}


/* 	I need to set up four threads for each period, and ensure that they 
	have the highest priorities so that they run at all times.				*/

/* Task Declation for threads */

/* Function Declaration for threads */
int oneHzDispatch(void* data) {

	int retval = 0;

	struct sched_param param = { .sched_priority = 95 };
	sched_setscheduler(current, SCHED_FIFO, &param);

	/* Wait for semaphore. Get it*/
	while(1) {
		if( ( retval = down_killable( &oneHzSemph ) ))
			return retval;
		
		oneHzFunc( NULL );
		dispatchFlag |= (1 << ONE_HZ_FLAG);
	}

}

int tenHzDispatch(void* data) {

	int retval = 0;

	struct sched_param param = { .sched_priority = 96 };
	sched_setscheduler(current, SCHED_FIFO, &param);

	/* Wait for semaphore. Get it*/
	/* Call User function here */
	while(1) {
		if( ( retval = down_killable( &tenHzSemph ) ))
			return retval;

		tenHzFunc( NULL );
		dispatchFlag |= (1 << TEN_HZ_FLAG);
	}
	
}

int hunHzDispatch(void* data) {

	/* Wait for semaphore. Get it*/
	/* Call User function here */
	int retval = 0;

	struct sched_param param = { .sched_priority = 97 };
	sched_setscheduler(current, SCHED_FIFO, &param);

	while(1) {
	if( ( retval = down_killable( &hunHzSemph ) ))
		return retval;

	hunHzFunc( NULL );
	dispatchFlag |= (1 << HUN_HZ_FLAG);
	}

}

int exitSched(void *data){

	struct sched_param param = { .sched_priority = 98 };
	sched_setscheduler(current, SCHED_FIFO, &param);

	down_killable( &exitSemph );

	printk( KERN_ERR "Task overrun!!\n");

	if( !(dispatchFlag & (1 << ONE_HZ_FLAG)))
		printk(KERN_ERR "Possibly 1Hz Task\n");

	if( !(dispatchFlag & (1 << TEN_HZ_FLAG)))
		printk(KERN_ERR "Possibly 10Hz Task\n");


	if( !(dispatchFlag & (1 << HUN_HZ_FLAG)))
		printk(KERN_ERR "Possibly 100Hz Task\n");

	kthread_stop( task1Hz );
	kthread_stop( task10Hz ); 
	kthread_stop( task100Hz ); 
	return 0;

}

static int __init p_scheduler_init( void ) {


	/* Create Semaphores here */
	int retval = 0;
	ktime_t k1Hz, k10Hz, k100Hz;

	sema_init( &oneHzSemph, 1 );
	sema_init( &tenHzSemph, 1 );
	sema_init( &hunHzSemph, 1 );
	sema_init( &exitSemph, 1 );

	dispatchFlag = 0xFF;
	dispatchFlag &= ~( 1 << OVERRUN_FLAG );
	


	if( (retval = down_killable( &oneHzSemph)) ) {
		printk(KERN_ERR "Cannot acquire one hertz semaphore\n");
	 	return retval;
	 }

	if( (retval = down_killable( &tenHzSemph)) ) {
		printk(KERN_ERR "Cannot acquire ten hertz semaphore\n");
	 	return retval;
	 }

 	if( (retval = down_killable( &hunHzSemph)) ) {
 		printk(KERN_ERR "Cannot acquire hundred hertz semaphore\n");
	 	return retval;
	 }

	 if( (retval = down_killable( &exitSemph)) ) {
	 	printk(KERN_ERR "Cannot acquire exit semaphore\n");
	 	return retval;
	 }



	/* Start and initialize your timers */

	/* Thread Creation */
	exitThread = kthread_run( exitSched, NULL, "CleanupThread");
	task100Hz = kthread_run( hunHzDispatch, NULL, "Thread100Hz");
	task10Hz = kthread_run( tenHzDispatch, NULL, "Thread10Hz");
	task1Hz = kthread_run( oneHzDispatch, NULL, "Thread1Hz");

	k1Hz   	= ktime_set( 0, MS_TO_NS(1000) ); 	//1000ms -> 1hz
	k10Hz  	= ktime_set( 0, MS_TO_NS(100) ); 	//100ms -> 10hz
	k100Hz 	= ktime_set( 0, MS_TO_NS(10) ); 	//10ms -> 100hz

	hrtimer_init( &timer100Hz, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	hrtimer_init( &timer10Hz, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	hrtimer_init( &timer1Hz, CLOCK_MONOTONIC, HRTIMER_MODE_REL );

	timer1Hz.function 	= 	&timer1HzCallback;
	timer10Hz.function 	=	&timer10HzCallback;	
	timer100Hz.function =	&timer100HzCallback;

	printk("Starting Timers\n");

	hrtimer_start( &timer100Hz, k100Hz, HRTIMER_MODE_REL);
	hrtimer_start( &timer10Hz, k10Hz, HRTIMER_MODE_REL);
	hrtimer_start( &timer1Hz, k1Hz, HRTIMER_MODE_REL);

	return 0;

}

static void __exit p_scheduler_cleanup( void ) {

	int retAllTimers;

	retAllTimers = hrtimer_cancel( &timer1Hz ) && hrtimer_cancel( &timer10Hz ) && hrtimer_cancel( &timer100Hz );

	if( retAllTimers )
		printk("Timers still in use\n");

	printk("Cancelling All Timers\n");
	up( &exitSemph );

	return;

}

module_init(p_scheduler_init);
module_exit(p_scheduler_cleanup);
