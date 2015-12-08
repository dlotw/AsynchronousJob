#ifndef SUBMITJOB_H_INCLUDED
#define SUBMITJOB_H_INCLUDED

#include <linux/kernel.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include "struct_job.h"

/* Queue list of job */
typedef struct jobList
{
        struct list_head list;
        AsynJob* job;
}JobList;

/*functions for core*/
int get_job_id(int job_id_list[]);

int copy_args_from_user(AsynJob *job, AsynJob *user_job, int argslen);
int copy_job_list_to_user(struct list_head *jobQueue, AsynJob *job);
int copy_checksum_reuslt_to_user(char *result, AsynJob *user_job);

int add_job(JobList *job, struct list_head *jobQueue);
int dequeue_job(AsynJob *job, struct list_head *jobQueue);
int remove_job_from_queue(AsynJob *job, struct list_head *jobQueue);
int change_job_priority(AsynJob *job, struct list_head *jobQueue);

void printk_job(AsynJob *job);
void print_queue_list(struct list_head *jobQueue);

int free_job_content(AsynJob *job);
int free_job_queue(struct list_head *jobQueue);

/*features*/
int checksum(AsynJob *job);
int open_read_file(char *file_name, char *content, int content_len);
int ecryptfs_calculate_md5_simplify(char* dst,char* src, int len);
int validate_file(struct file *filp, int flag);
int write_to_out_file(char* out_path, char* in_path);
int concatenate(AsynJob *job);
int compress(AsynJob *job);
int decompress(AsynJob *job);
#endif

