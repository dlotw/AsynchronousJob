#include <linux/linkage.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/moduleloader.h>
#include <linux/delay.h>
#include "submitjob.h"

asmlinkage extern long (*sysptr)(void *arg, int argslen);

/* global variables */
/* Job list queue */
struct list_head *jobQueue;
/* Total number of job in queue*/
int list_count;
/* Max number of job*/
int max_count;
/* Job ID*/
int job_id_list[MAX_BUF_LEN];
/* Mutex lock for job*/
DEFINE_MUTEX(job_mutex);
/* task struct to for kthread */
struct task_struct *consumerThread = NULL;

/* Init Global Variable */
int init_global_variable(void)
{
	int i;
	jobQueue = kmalloc(sizeof(struct list_head), GFP_KERNEL);
	printk("init job queue\n");
	INIT_LIST_HEAD(jobQueue);
	list_count = 0;
	max_count = MAX_QUEUE_SIZE;
	for (i=0; i<MAX_BUF_LEN; i++) 
		job_id_list[i]=0;
    	return 1;
}

int producer(AsynJob* job){
	int err=-EINVAL;
	JobList *jobNode;
	long id = -1;	

	if (mutex_is_locked(&job_mutex))
		printk("in producer begin:mutex is locked\n");
	else
		printk("in producer begin:mutex is not locked\n");
	
	mutex_lock(&job_mutex);
	if(job->job_type==LIST){
		err=copy_job_list_to_user(jobQueue, job->usr_addr);
		goto out;
	}
	else if (job->job_type==REMOVE){
		err=remove_job_from_queue(job,jobQueue);
		list_count --;
		err=kstrtol(job->job_target, 10, &id);
		job_id_list[id] = 0;
		goto out;
	}
	else if (job->job_type==PRIORITY ){
		err=change_job_priority(job,jobQueue);
		goto out;
	}

	set_current_state(TASK_INTERRUPTIBLE);
	if(list_count>max_count){	
		err=-EAGAIN;
		/*!!!add current thread to producer waitq*/
		// mutex_unlock(&job_mutex);
    		// schedule();
    		// mutex_lock(&job_mutex);
		
	}
	set_current_state(TASK_RUNNING);

	jobNode = kmalloc(sizeof(JobList), GFP_KERNEL);
	if (jobNode==NULL){	
		err=-ENOMEM;
		goto out;
	}

	err=get_job_id(job_id_list);

	if (err<0){
		printk("no free id available.\n");
		goto out;
	}
	job->job_id=err;
	jobNode->job = job;
	err = add_job(jobNode, jobQueue);
	
	if(err < 0){
		printk("error in add job.\n");
		goto out;
	}

	list_count++;
	//print_queue_list(jobQueue);
	printk("The total count is:%d\n",list_count);

	if (list_count==1){
		printk("wake up consumer from producer...\n");
		/*!!!wake up consumer*/
		mutex_unlock(&job_mutex);
		wake_up_process(consumerThread);
	}
	err=0;
	// set_current_state(TASK_INTERRUPTIBLE);
	// mutex_unlock(&job_mutex);
	// schedule();
out:
	if(mutex_is_locked(&job_mutex))
		mutex_unlock(&job_mutex);
	return err;
}

int consumer(void){
	int err=-EINVAL;
	AsynJob *job;
	job=kmalloc(sizeof(AsynJob),GFP_KERNEL);
	if (mutex_is_locked(&job_mutex))
		printk("in consumer begin:mutex is locked\n");
	else
		printk("in cousumer begin:mutex is not locked\n");
	mutex_lock(&job_mutex);
	while(1){
	set_current_state(TASK_INTERRUPTIBLE);
	if (list_count==0){
		printk("JobList is empty, add current thread to waitq\n");
		/*!!!add current thread to waitq*/	
		mutex_unlock(&job_mutex);
		schedule();
		mutex_lock(&job_mutex);
	}
	set_current_state(TASK_RUNNING);
	if (job==NULL){	
		err=-ENOMEM;
		mutex_unlock(&job_mutex);
		goto out;
	}
	err=dequeue_job(job, jobQueue);
	if(err<0){ 
		mutex_unlock(&job_mutex);
		goto freejob;
	}
	list_count--;
	job_id_list[job->job_id]=0;
	printk("The total count is:%d\n",list_count);
	if(list_count==0){
		printk("add current thread to consumer waitq, but have to process this job first\n");
		/*!!!add current thread to consumer waitq, but have to process this job first*/				
		set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&job_mutex);
	
		if (job->job_type==CHECKSUM){
			err = checksum(job);
			if(err < 0){
				goto onlyjobfail;
			}
			printk("checksum result %u\n",(unsigned int) job->job_info);
			// err = copy_to_user(job->usr_addr->job_info, job->job_info, job->job_info_size);
			// if(err < 0){
			//	goto freejob;
			// }
			// job->usr_addr->job_info_size = job->job_info_size;
		}
		else if (job->job_type==CONCATENATE){
			err = concatenate(job);
			if(err < 0){
				goto onlyjobfail;
			}
		}
		else if (job->job_type==COMPRESS){
			err = compress(job);
			if(err < 0){
				goto onlyjobfail;
			}
		}
		else if (job->job_type==DECOMPRESS){
			err = decompress(job);
			if(err < 0){
				goto onlyjobfail;
			}
		}
		printk("job done...consumer thread going to sleep...\n");
onlyjobfail:
		//printk("only job failed...\n");
		schedule();
		set_current_state(TASK_RUNNING);
		printk("consumer thread waken up...\n");
		goto out;
	}
	//else if (list_count<max_count){
	//	printk("wake up producer from producer waitq\n");
	//	/*!!!wake up producer form producer waitq*/
	//}

	mutex_unlock(&job_mutex);
	
	if (job->job_type==CHECKSUM){
		err = checksum(job);
		if(err < 0){
			goto freejob;
		}
		printk("checksum result %u\n",(unsigned int) job->job_info);
		//err = copy_to_user(job->usr_addr->job_info, job->job_info, job->job_info_size);
		//if(err < 0){
		//	goto freejob;
		//}
		//job->usr_addr->job_info_size = job->job_info_size;
	}
	else if (job->job_type==CONCATENATE){
		err = concatenate(job);
		if(err < 0){
			goto freejob;
		}
	}
	else if (job->job_type==COMPRESS){
		err = compress(job);
		if(err < 0){
			goto freejob;
		}
	}
	else if (job->job_type==DECOMPRESS){
		err = decompress(job);
		if(err < 0){
			goto freejob;
		}
	}
	printk("job done...\n");

freejob:
	
out:
	msleep(1000);
	continue;
	}
	if(!err) printk("Err code is:%d\n",err);
	return err;

}

asmlinkage long submitjob(void *arg, int argslen)
{
	int err=-EINVAL;
	AsynJob *job;
	
	job=kmalloc(sizeof(AsynJob), GFP_KERNEL);
	if (job==NULL){
		err=-ENOMEM;
		goto out;
	}
		
	err=copy_args_from_user(job, (AsynJob*)arg, argslen);
	if(err<0)
		goto freejob;	

	if (job->job_flag==0)
		err=producer(job);
	else
		consumer();
	goto out;

freejob:
	if (job) kfree(job);
out:
	return err;
}

static int __init init_sys_submitjob(void)
{
	int err = 0;
	printk("installed new sys_submitjob module\n");
	init_global_variable();
	consumerThread = kthread_create(consumer, NULL, "consumerThread");
	if(IS_ERR(consumerThread)){ 
      		printk("Unable to start kernel thread.\n");
      		err = PTR_ERR(consumerThread);
     		consumerThread = NULL;
		return err;
    	}
	if (sysptr == NULL)
		sysptr = submitjob;
	return 0;
}
static void  __exit exit_sys_submitjob(void)
{
	free_job_queue(jobQueue);
	if (consumerThread){
		printk("stop consumer thread\n");
		kthread_stop(current);
		printk("current thread stoped\n");
		//kthread_stop(consumerThread);
		//printk("consumer thread stoped\n");
	}
	if (sysptr != NULL)
		sysptr = NULL;
	printk("removed sys_submitjob module\n");
}
module_init(init_sys_submitjob);
module_exit(exit_sys_submitjob);
MODULE_LICENSE("GPL");
