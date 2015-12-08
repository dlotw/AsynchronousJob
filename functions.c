
#include "submitjob.h"

/*
	assign a job id to a job. 
	return unqiue free job id if success,
		-ENFILE if job_id_list is full.
*/
int get_job_id(int job_id_list[]){
	int err=-ENFILE;
	int i;
	for (i=0; i<MAX_BUF_LEN; i++){
		if (job_id_list[i]==0){
			job_id_list[i] = 1;
			return i;
		}
	}
	return err;
}

/*
	copy user land AsynJob to kernl.
	@job: job in kernel.
	@user_job: job copy from.
	@argslen: length of user_job.
	return 0 if no error.
*/
int copy_args_from_user(AsynJob *job, AsynJob *user_job, int argslen){
	int err=-EINVAL;
	if (user_job == NULL)
		return -EFAULT;
	job->job_id=user_job->job_id;
	job->job_type=user_job->job_type;
	job->job_flag=user_job->job_flag;
	job->job_info_size=user_job->job_info_size;
	job->job_priority=user_job->job_priority;
	job->usr_addr=user_job->usr_addr;

	job->job_target=kmalloc(strlen(user_job->job_target)+1,GFP_KERNEL);
	if (job->job_target==NULL){
		err=-ENOMEM;
		goto out;
	}
	if (copy_from_user(job->job_target, user_job->job_target, strlen(user_job->job_target) )!=0){
		err= -EFAULT;
		goto freejobtarget;
	}
	job->job_target[strlen(user_job->job_target)]='\0';
	
	job->job_config=kmalloc(strlen(user_job->job_config)+1,GFP_KERNEL);
	if (job->job_config==NULL){
		err=-ENOMEM;
		goto freejobtarget;
	}
	if (copy_from_user(job->job_config, user_job->job_config, strlen(user_job->job_config))!=0){
		err= -EFAULT;
		goto freejobconfig;
	}
	job->job_config[strlen(user_job->job_config)]='\0';
	
	if(job->job_info_size>0){
		job->job_info=kmalloc(job->job_info_size,GFP_KERNEL);
		if (copy_from_user(job->job_info, user_job->job_info, job->job_info_size)!=0){
			err= -EFAULT;
			goto freejobinfo;
		}
	}
	err=0;
	goto out;
freejobinfo:
	if(job->job_info) kfree(job->job_info);
freejobconfig:
	if(job->job_config) kfree(job->job_config);
freejobtarget:
	if(job->job_target) kfree(job->job_target);
out:
	return err;
}

/*
	copy job list information to user for job type 'list'
	@jobQueue: head of job queue where job stores.
	@user_job: return information buffer to job_info field of user_job
	return 0 if no error.
*/
int copy_job_list_to_user(struct list_head *jobQueue, AsynJob *user_job){
	int err=-EINVAL;
	struct list_head *ptr;
	JobList *entry;
	AsynJob *currentJob;
	int c=0;
	char buf[sizeof(int)];
	char* str=kmalloc(MAX_BUF_LEN,GFP_KERNEL);
	strcpy(str,"JobList:\n");

	for(ptr = jobQueue->next; ptr != jobQueue; ptr = ptr->next){
		entry = list_entry(ptr, JobList, list);
		currentJob=entry->job;
		sprintf(buf, "%d. ", ++c);
		strcat(str,buf);
		sprintf(buf, "%d ", currentJob->job_id);
		strcat(str,"{");
		strcat(str, "id=");
		strcat(str, buf);
		strcat(str, ", j=");
		sprintf(buf, "%d", currentJob->job_type);
		strcat(str, buf);
		/*if t is too long, should truncate i, otherwise may buffer over flow*/
		strcat(str, ", t=");
		strcat(str, currentJob->job_target);
		
		/*!!!strcat other part*/
		strcat(str, ", ...");

		strcat(str, ", p=");
		sprintf(buf, "%d", currentJob->job_priority);
		strcat(str, buf);
		strcat(str,"}\n");

	}
	user_job->job_info_size=strlen(str);
	err=copy_to_user(user_job->job_info,str,user_job->job_info_size);
	err=0;
	if(str) kfree(str);
	return err;
}


/*
	Added job to sorted job queue.
	jobQueue sorted by priority, then by time added.
	@jobNode: new jobNode needs to be added.
	@jobQueue: head of job queue where job stores. 
	return 0 if no error.
*/
int add_job(JobList *jobNode, struct list_head *jobQueue){
	struct list_head *ptr;
	JobList *entry;

	for (ptr = jobQueue->next; ptr != jobQueue; ptr = ptr->next) {
		entry = list_entry(ptr, JobList, list);
		if (entry->job->job_priority < jobNode->job->job_priority) {
			list_add_tail(&jobNode->list, ptr);
			goto out;
		}
	}
	list_add_tail(&jobNode->list, jobQueue);
	print_queue_list(jobQueue);
out:
	return 0;
}


/* 
	Get the first job in the list and remove it 
	@job: store the dequeued job.
	@jobQueue: head of job queue where job stores.
	return 0 if no error,
		-ENOCSI if job not found,
		-ESPIPE if jobQueue is empty.
*/
int dequeue_job(AsynJob *job, struct list_head *jobQueue)
{
	int err = -ESPIPE;
	struct list_head *ptr = jobQueue->next;
	struct list_head *cur, *tmp;
	JobList *entry;
	if(ptr == jobQueue){
		goto out;
	}
	list_for_each_safe(cur,tmp,jobQueue){
		entry = list_entry(cur, JobList, list);
		if(cur == ptr){
			printk("Dequeue=");
			printk_job(entry->job);
			list_del(cur);
			err=0;
			goto entry_value;
		}
	}
	err = -ENOCSI;
	goto out;
entry_value:
	(*job) = (*entry->job);
out:
	return err;
}

/*
	remove job from jobQueue.
	@job: for job_type 'remove', job->job_target stores the job id needs to be removed.
	@jobQueue: head of job queue where job stores.
	search by job->job_target (cast to int), if found, remove the job from jobQueue.
	return 0 if no error, 
		-ENOCSI if job not found in the list, 
		-ESPIPE queue is empty.
*/
int remove_job_from_queue(AsynJob *job, struct list_head *jobQueue){
	int err=-ESPIPE;
	struct list_head *cur, *tmp;
	JobList *entry;
	AsynJob *currentJob;
	long id;
	if(list_empty(jobQueue) != 0){
		goto out;
	}		
	err=kstrtol(job->job_target, 10, &id);
	list_for_each_safe(cur,tmp, jobQueue){
		entry = list_entry(cur, JobList, list);
		currentJob=entry->job;
		if (currentJob->job_id==id){
			list_del(cur);
			err=0;
			if(entry->job)
				err=free_job_content(entry->job);
			kfree(entry);
			goto out;
		}
	}
	err=-ENOCSI;
out:
	return err;
}

/*
	change job priority.
	@job: for job_type 'priority', 
		job->job_target stores the job id needs to be changed for priority.
	@jobQueue: head of job queue where job stores.
	search by job->job_target (cast to int). 
	If find, change currentjob->job_priority to job->job_priority.
	return 0 if no error, 
		-ENOCSI if job not found,
		-ESPIPE if jobQueue is empty.
*/
int change_job_priority(AsynJob *job, struct list_head *jobQueue){
	int err=-ESPIPE;
	struct list_head *cur, *tmp;
 	JobList *entry;
	long id;
	if(list_empty(jobQueue) != 0){
		goto out;
	}
	err=kstrtol(job->job_target, 10, &id);
	list_for_each_safe(cur,tmp, jobQueue){
		entry = list_entry(cur, JobList, list);
		if (entry->job->job_id==id){
			list_del(cur);
			entry->job->job_priority=job->job_priority;         
			err = add_job(entry, jobQueue);	
			goto out;
		}
	}
	err=-ENOCSI;
out:
	return err;
}


/*
	print a job.
	@job: job that needs to be printed.
*/
void printk_job(AsynJob *job){ 
	char* str=kmalloc(MAX_BUF_LEN,GFP_KERNEL);
	char buf[sizeof(int)];
	sprintf(buf, "%d", job->job_id);
	strcpy(str,"{");
	strcat(str, "id=");
	strcat(str, buf);
	strcat(str, ", j=");
	sprintf(buf, "%d", job->job_type);
	strcat(str, buf);
	strcat(str, ", t=");
	strcat(str, job->job_target);
	
	/*!!!strcat other part*/
	strcat(str, ", ...");
	
	strcat(str, ", p=");
	sprintf(buf, "%d", job->job_priority);
	strcat(str, buf);
	strcat(str,"}\n");
	printk(str);
	
	if(str) kfree(str);
}

/*
	print queue list.
	@jobQueue: head of job queue where job stores. 
*/
void print_queue_list(struct list_head *jobQueue)
{
	struct list_head *ptr;
	JobList *entry;	
	printk("Print queue list: \n");
	for(ptr = jobQueue->next; ptr != jobQueue; ptr = ptr->next){
		entry = list_entry(ptr, JobList, list);
		printk_job(entry->job);
	}
	printk("\n");
}

/*
	free job content.
	@job: job that needs to be freed.
	return 0 if no error.
*/
int free_job_content(AsynJob *job){
	//if(job->job_info) kfree(job->job_info);
	if(job->job_config) kfree(job->job_config);
	if(job->job_target) kfree(job->job_target);	
	return 0;
}

/*
	free job queue.
	@jobQueue: head of job queue.
	return 0 if no error.
	
*/
int free_job_queue(struct list_head *jobQueue)
{
	int err=0;
	JobList *entry;
	struct list_head *cur, *tmp;
	printk("free job queue\n");
	/* is empty */
	if(list_empty(jobQueue) != 0){
		printk("The job is empty!\n");
		jobQueue = NULL;
	}
	else{
		list_for_each_safe(cur,tmp, jobQueue){
		entry = list_entry(cur, JobList, list);
		list_del(cur);
		if(entry->job)
			err=free_job_content(entry->job);
			kfree(entry);
		}
	}
	/* is not empty*/
	// do something
	return err;
}

