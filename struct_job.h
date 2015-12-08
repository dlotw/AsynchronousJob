/*
*/
#ifndef _struct_job_h
#define _struct_job_h


#define MAX_BUF_LEN 2048
#define MAX_PATH_LEN 256
#define MAX_QUEUE_SIZE 12
/*MAX_BUF_LEN should hold all return info for the elements in queue with MAX_QUEUE_SIZE*/

#define LIST 0
#define REMOVE 1
#define PRIORITY 2
#define CHECKSUM 3
#define CONCATENATE 4 
#define COMPRESS 5
#define DECOMPRESS 6

typedef struct job
{
	/* primary key of job, maintenance the bitmap */
	int job_id;
	/* job type 
	 	0=list job queue
		1=remove job from job queue
		2=change priority of job
		3=checksum
	*/
	int job_type;
	/* job target, depends on job_type, 
	   usually the file name to process in this job
	 */
	char *job_target;
	/* configuration processing 
	(e.g., cipher name, checksum alg, compression, etc.)
	*/
	char *job_config;
	/* flags that control processing 
	(e.g. atomicity, error handling, priority, etc.)
	*/
	int job_flag;
	/* shared memery b/t user and kernel for callback function */
	void *job_info;
	/* size of job_info */
	int job_info_size;	
	int job_priority;
	/* user land job address, stored for writing consumer result back to user*/
	struct job *usr_addr;
	
}AsynJob;

#endif


