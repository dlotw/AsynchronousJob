/*
 * Userland implementation.
 * Take user's input, process input and request system call
 * to process with different jobs.
 */
#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "struct_job.h"

#ifndef __NR_submitjob
	#error submitjob system call not defined
#endif

/* Help message */
void help_message(){
	printf("xproducer HELP\n");
	printf("Usage:\n");
	printf(" -j job type. Required\n");
	printf(" -t target file or job id. Required except for 'list'\n");
	printf(" -c config. Optional, Default: \n");
	printf(" -f flag. Optional, Default: \n");
	printf(" -x info. Optional.\n");
	printf(" -p priority. Optional, Default: 4\n");
	printf(" -h help message\n");
	exit(1);
}

void print_job(AsynJob *job){
	printf("	{ID(sizeof~%d):%d\n", sizeof(job->job_id), job->job_id );
	printf("	Type(sizeof~%d):%d\n", sizeof(job->job_type), job->job_type);
	printf("	Target(strlen~%d):%s\n", strlen(job->job_target), job->job_target);
	printf("	Config(strlen~%d):%s\n", strlen(job->job_config), job->job_config);
	printf("	Flag(sizeof~%d):%d\n", sizeof(job->job_type), job->job_flag);
	printf("	Info(sizeof~%d)\n",sizeof(job->job_info));
	printf("	InfoSize(sizeof~%d):%d\n", sizeof(job->job_info_size), job->job_info_size);
	printf("	Priority(sizeof~%d):%d\n", sizeof(job->job_priority), job->job_priority);
	printf("	SizeofAsynJob(%d)}\n", sizeof(AsynJob));
	
	
}
/*
 * Sanity check for job type
 * return -1, if error.
 * job type, if no error.
*/
int validate_job_type(char* type){
	int err=-1;
	if (strcmp("list",type)==0){
		err=LIST;
	}
	else if (strcmp("remove",type)==0){
		err=REMOVE;
	}
	else if (strcmp("priority",type)==0){
		err=PRIORITY;
	}
	else if(strcmp("checksum",type)==0){
		err=CHECKSUM;
	}
	else if(strcmp("concatenate",type)==0){
		err=CONCATENATE;
	}
	else if(strcmp("compress",type)==0){
		err=COMPRESS;
	}
	else if(strcmp("decompress",type)==0){
		err=DECOMPRESS;
	}
	return err;
}

/*
Sanity check for target
return -1, if error
		0, if valid path
		1, if integer
*/
int validate_target(char* target){
	int err=-1;
	int i;
	if (strlen(target)<1 || strlen(target)>MAX_PATH_LEN){
		printf("Invalid file PathLength\n");
		goto out;
	}
	for (i=0;i<strlen(target);i++){
		if (!isdigit(target[i])){
			/*!!!check if file is valid file*/
			err=0;
			goto out;
		}
	}
	err=1;
out:
	return err;
}

int validate_config(char* config){
	int err=-1;
	/*!!!check config*/
	err=0;
	return err;
}

int validate_flag(char* flag){
	int err=-1;
	if(strcmp("1",flag)==0){
		err=1;
	}
	else
		err=0;
	return err;
}

int validate_priority(char* priority){
	int err=-1;
	if (strlen(priority)>1 || !isdigit(priority[0]))
		goto out;
	err=0;
out:
	return err;
}

int validate_info(char* path){
	int err=-1;
	/*!!!check path*/
	err=0;
	return err;	
}

int set_info(AsynJob *job, char* tmp){
	int err=-1;
	void *job_info;
	int job_info_size;
	char* info;
	if (job->job_type==LIST){
		job_info_size=MAX_BUF_LEN+1;
		job_info=malloc(job_info_size);
        
	}
	else if( job->job_type==CHECKSUM){
		job_info_size=16+1;
		job_info=malloc(job_info_size);
	}
	else if( job->job_type==CONCATENATE){
		job_info_size=strlen(tmp)+1;
		info=malloc(job_info_size);
		strcpy(info,tmp);
		job_info=(void*)info;
	}
	else if (job->job_type==COMPRESS || job->job_type==DECOMPRESS){
		job_info_size=strlen(tmp);
		if(job_info_size>0){
			info=malloc(job_info_size);
			strcpy(info,tmp);
			info[job_info_size-1]='\0';
			job_info=(void*)info;
			}
			
	}
	else{
		/*!!!set default*/
		job_info_size=0;
		//job_info=malloc(job_info_size);
				
	}
	job->job_info=job_info;
	job->job_info_size=job_info_size;
	
	err=0;
	return err;
}

int free_info(AsynJob *job){
	int err=-1;
	
	err=0;
	return err;
}

int validate_argv(int argc, char *argv[], AsynJob* job){
	int err=1;
	extern char *optarg;
	extern int optind, opterr, optopt;
	char *job_target;
	char *job_config;
	char *tmp;
	char *options= ":j:t:c:f:x:p:h";
	int opt=0;
	int job_type=-1;
	int target_type=-1;
	int job_id=-1;
	int job_flag=0;
	int job_priority=4;
	int has_job_type=0;
	int has_job_target=0;
	int has_job_config=0;
	int has_job_flag=0;
	int has_job_priority=0;
	int has_job_info=0;
	
	tmp=malloc(MAX_BUF_LEN+1);
	while((opt = getopt(argc,argv,options))!=-1){
		switch(opt){	
			case 'j':
				job_type=validate_job_type(optarg);
				if(job_type<0){
					printf("Invalid Job Type\n");
					err= -EINVAL;
				}
				else
					has_job_type++;
				break;
			case 't':
				target_type=validate_target(optarg);
				if(job_target<0){
					printf("Invalid File\n");
					err= -EINVAL;
				}
				else{    
					job_target=malloc(strlen(optarg)+1);
					strcpy(job_target,optarg);	
					has_job_target++;
				}
				break;
			
			case 'c':
				if (validate_config(optarg)<0){
					printf("Invalid Config\n");
					err= -EINVAL;
				}
				else{
					job_config=malloc(strlen(optarg)+1);
					strcpy(job_config,optarg);
					has_job_config++;
				}
				break;
			case 'f':
				job_flag=validate_flag(optarg);
				if (job_flag<0){
					printf("Invalid Flag\n");
					err= -EINVAL;
				}
				break;
			case 'x':
				if(validate_info(optarg)<0){
					printf("Invalid in_file path\n");
					err= -EINVAL;	
				}
				else{
					strcat(tmp,optarg);
					strcat(tmp,";");
					has_job_info++;
				}
				break;
				
			case 'p':
				if (validate_priority(optarg)<0){
					printf("Invalid Priority. 0<=priority<=9\n");
					err= -EINVAL;
				}
				else{
					job_priority=optarg[0] - '0';
					has_job_priority++;
					}
				break;
			
			case 'h':
				help_message();
				break;
			case ':':
				fprintf(stderr,"Option -%c needs argument. use -h for help\n",optopt);
				err= -EINVAL;
				break;
			case '?':
				fprintf(stderr,"Unknown option: -%c\n",optopt);
				err= -EINVAL;
				break;
			default:
				err= -EINVAL;
				break;
		}
	}
	
	if (err<0)
		goto out;
	err=-EINVAL;
	if(optind < argc){
		printf("Unknown Command. Option Flag Required.\n");
		goto out;
	}
	
	if (!has_job_type){
		printf("Job type required.\n");
		goto out;
	}
	
	if (has_job_type>1 || has_job_target>1 || has_job_priority>1){
		printf("Duplicate Option Flags.\n");
		goto out;
	}
	
	if (job_type==LIST && has_job_target+has_job_priority+has_job_info>0){
		printf("Job Type 'list' Takes Only -j Argument\n");
		goto out;
	}
	if (job_type>LIST && !has_job_target){
		printf("Job Target -t Argument Is Missing.\n");
		goto out;
	}
	if (job_type==REMOVE){
		if (target_type==0){
			printf("Job Type 'remove' Takes Only Integer For -t Argument.\n");
			goto out;
			}
		if (has_job_priority+has_job_info>0){
			printf("Job Type 'remove' Takes Only -j -t Argument.\n");
			goto out;
			}
	}
	if (job_type==PRIORITY){
		if (!has_job_priority){
			printf("Job Type 'priority' -p Argument Is Missing.\n");
			goto out;
		}
		if (target_type==0){
			printf("Job Type 'priority' Takes Only Integer For -t Argument.\n");
			goto out;
			} 
		if (has_job_flag+has_job_info){
			printf("Job Type 'priority' Takes Only -j -t -p Argument.\n");
			goto out;
		} 
	}
	if (job_type>PRIORITY && target_type==1){
		printf("Feature Takes Only Char For -t Argument\n");
		goto out;
	}
	if (job_type==CHECKSUM){
		if (has_job_info){
			printf("Job Type 'checksum' Takes Only -j -t [-p] Argument.\n");
			goto out;
		}
	}
	else if (job_type==CONCATENATE){
		if (!has_job_info){
			printf("Job Type 'concatenate' -x Argument Is Missing.\n");
			goto out;
		}
	}
	else if (job_type==COMPRESS || job_type==DECOMPRESS){
		if (has_job_info>1){
			printf("Duplicate -x Argument For 'compress/decompress'.\n");
			goto out;
		}
		
	}
	
	
	/*set default target for job type 'list'*/
	if (!has_job_target){
		job_target=malloc(2);
		strcpy(job_target,"0");
	}
	
	/*!!!set default job config. move after job type assignment if in functon*/
	if (!has_job_config){
		job_config=malloc(2);
		strcpy(job_config,"0");
	}	
	/*!!!set default job_flag*/

	
	job->job_id=job_id;
	job->job_type=job_type;
	job->job_target=job_target;
	job->job_config=job_config;
	job->job_flag=job_flag;
	job->job_priority=job_priority;
	job->usr_addr=job;
	set_info(job,tmp);
	
	err=1;
out:
	if(tmp) free(tmp);
    return err;
}

int main(int argc,  char *argv[])
{
	int rc;
	AsynJob job;
	rc=validate_argv(argc,argv,&job);
	
	if(rc<1)
	exit(rc);
	
	rc = syscall(__NR_submitjob, &job, sizeof(AsynJob));
	if (job.job_type==LIST)
		printf("%s\n",(char*)job.job_info);
	if (rc == 0)
	printf("Submit job succeed!\n");
	else
	{
		printf("syscall returned %d (errno=%d)\n", rc, errno);
		switch(errno){
			case EFAULT:
				printf("Bad Memory Address Error.\n");
				break;
			case EINTR:
				printf("Kernel System Call Error.\n");
				break;
			case ENOENT:
				printf("Source File Not Exist.\n");
				break;
			case EPERM:
				printf("No Read/Write Permission.\n");
				break;
			case EISDIR:
				printf("File Path Is Directory.\n");
				break;
			case ENOSPC:
				printf("Source And Target File Are The Same.\n");
				break;
			case EEXIST:
				printf("Target File Already Exist.\n");
				break;
			case EIO:
				printf("Bytes Write Is Different From Bytes Read.\n");
				break;
			case ENOMEM:
				printf("No Enough Memory.\n");
				break;
			case EINVAL:
				printf("Wrong Function Arguments.\n");
				break;
			case ENOMSG:
				printf("Unexpected File Type (*.z expected).\n");
				break;
			case ENFILE:
				printf("Job ID Is Full.\n");
				break;
			case ESPIPE:
				printf("Delete Job From An Empty Job Queue.\n");
				break;
			case ENOCSI:
				printf("Job Not Found In Job Queue.\n");
				break;
			case EAGAIN:
				printf("Job Queue Is Full. Try Again Later\n");
				break;
			
			
			default:
				printf("Unknown Error!\n");
				break;
		}
	}
	
	if (job.job_type==LIST)
		printf("%s\n",(char*)job.job_info);
	
	/*!!!free job content*/
	if(job.job_target) free(job.job_target);
	if(job.job_config) free(job.job_config);
	
	exit(rc);
}
