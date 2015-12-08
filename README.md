===========================================================
=                                                         =
=                      Index                              =
=                                                         =
===========================================================
1. Introduction
2. Instruction for installation
3. Test examples input
4. Userland design
5. Kernel design
6. Reference

===========================================================
=                                                         =
=                   Introduction                          =
=                                                         =
===========================================================

1. core.c
	Contains system calls for producer. In this file also includes initial modules, exist modules, consumer functions. This is the main file for kernel.	

2. function.c
	Help functions to support core.c in kernel. For examples, the queue operations in kernel and so on.

3. features.c
	All the features' functions that kernel can handle, like checksum, concatenate and compress/depress.

4. xproduce.c
	Usrland process. Handled all user input, throw error message and notification to user. Als return the processing results.

5. submitjob.h
	Declare queue structure in kernel, also with all the functions that kernel used.

6. struct_job.h
	Structure that kernel and userland both used, which defines the structure of each node in job queue. 

7. install_module.sh
	Script to install module.

8. Makefile
	Compile script to compile all the files.

9. kernel.config
	Kernel config file. User should use this file as the .config to make the whole kernel.

10. test folder
	This folder contains several input file for test cases. See details in "Test examples input"

11. err_xproducer.sh
	Script to test user-land error handling.
12. err_job.sh
	Script to test kernel error handling.
===========================================================
=                                                         =
=           Instruction for installation                  =
=                                                         =
===========================================================
1. Build kernel.
2. Add 
“
359	i386	submitjob		sys_submitjob
”
at the end of /arch/x86/syscall/syscall_32.tbl 
Add
“
asmlinkage long (*sysptr)(void *arg, int argslen) = NULL;

asmlinkage long sys_submitjob(void *arg, int argslen)
{
	if (sysptr != NULL)
		return (*sysptr)(arg, argslen);
	else
		return -ENOTSUPP;
}
EXPORT_SYMBOL(sysptr);
“
at the end of /fs/open.c


3. Go to ./hw3 folder which contains all the files for hw3.
4. Type the following command to build this module
make clean
make
sh install_module.sh
5. Before test, install the test script.
sh install_test.sh 
6. Then all the installation is done. User can input the result



===========================================================
=                                                         =
=                  Test examples input                    =
=                                                         =
===========================================================
Example commands:
ps. If you install the install_test.sh, you can directly 
run the following codes.

1. list all jobs in job queue:
./xproducer -j list

2. change priority for job 1 to 9 (Suppose there is a job with job_id=1 in the queue)
./xproducer -j priority -t 1 -p 9

3. remove job 1 (Suppose there is a job with job_id=1 in the queue)
./xproducer -j remove -t 1

4. compute checksum of input file /tmp/test/testChecksum
./xproducer -j checksum -t /tmp/test/testChecksum  

5. concatenate input file /tmp/test/concat1, /tmp/test/concat2, /tmp/test/concat3, and wirte result file to /tmp/test/OutputConcate
./xproducer -j concatenate -t /tmp/test/OutputConcat -x /tmp/test/concat1 -x /tmp/test/concat2 -x /tmp/test/concat3

6. compress file /tmp/test/compress.
The output file is optional, if choose not to specify the output file name, program will name the output file as “INPUTFILENAME.z”, the command is:
./xproducer -j compress -t /tmp/test/compress
if choose specific name, the output file must with suffix .z
./xproducer -j compress -t /tmp/test/compress -x /tmp/test/compressOutput.z 

7. decompress file /tmp/test/decompress.z, the decompress file would with a suffix .u. 
The output file is optional, if choose not to specify the output file name, program will name the output file as “INPUTFILENAME.u”, the command is:
./xproducer -j decompress -t /tmp/test/compress.z
if choose specific name, 
./xproducer -j decompress -t /tmp/test/decompress.z -x /tmp/test/decompressOutput 


===========================================================
=                                                         =
=                Userland design                          =
=                                                         =
===========================================================
1. User land command to 'produce' a job:

./xproducer -j JOBTYPE -t JOBTARGET -c JOBCONFIG 
-f JOBFLAG -x JOBINFO -p JOBPRIORITY

Arugments:

-j JOBTYPE: (Required)
	type of the job, must be one of the following:
	list: list current jobs in the job queue.
	remove: remove given job from job queue.
	priority: change priority of a given job.
	checksum: compute checksum of a given file.
	concatenate: concatenate multiple input files to a new output file.
	compress: compress given input file.
	decompress: decompress given .z file. 
-t JOBTARGET: (Required, except for 'list')
	file or job id that job works on:
	'list':
		-t shoud be skipped.
	'remove' and 'priority': 
		-t should be a valid job id.
	'checksum' , ‘concatenate’, 'compress' and 'decompress' 
		-t should be a valid file path.
		
-c JOBCONFIG: (Optional)
	job config for a job.
	'list', 'remove' or 'priority':
		-c should be skipped.
	'checksum':
		-c should be a vaild checksum algorithm.
		   if not specified, set default value
		   to 'md5'

-f JOBFLAG: (Optional)
	job flag for a job.
	'list', 'remove' or 'priority':
		-f should be skipped.
	'checksum':
		-f should be XXX
		   if not specified, set default value 
		   to XXX
		   
-x JOBINFO: (Optional, except for ‘concatenate’)
	other information a job may required.
	'list':
		job_info stores the returned job information
		in the queue. User should not set this argument.
	'remove' or 'priority':
		-x should be skipped.
	'checksum':
		job_info stores the returned checksum.
		User should not set this argument.
	‘concatenate’:
		job_info stores input files path. There should be 
at least ONE input file. Option flag -x is required 
for each input file.
	‘compress’:
		job_info stores output file name. It’s optional, but
if output file name is given, it must be end
		with ‘.z’. 
	‘decompress’:
		job_info stores output file name. Optional.

-p JOBPRIORITY: (Optional, except for 'priority')
	priority for job. range: 0-9, priority low to high.
	default value: 4.
	'list' or 'remove':
		-p should be skipped, since these two job types
		   will not be inserted into queue.
	'priority':
		-p must have a valid integer value [0,9].
	'checksum', ‘concatenate’, ‘compress’ and ‘decompress’:
		-p should be a single digit integer.
		  if not specified, set default value to 4.

If argument is missing, or job type and job target mismatch, or has more arguments than expected, or comes without option flag, program will return error.		  
===========================================================
=                                                         =
=                  Kernel design                          =
=                                                         =
===========================================================
1. Global variable 'jobQueue' stores job to be processed.

2. struct for each node in job queue -- AsynJob:

	a. int job_id;             
	unique job id in the jobQueue, generated only when a 
	job is added to the jobQueue,  

	b. int job_type;	
	type of job, e.g. 'list', 'checksum' 

	c. char *job_target;
	pathname or id works on

	d. char *job_config;
	additional config for different job type
	will set to default value if user not set.

	e. int job_flag;

	f. void *job_info;
	other info may required for a job.
	e.g. 'list' need a buffer to store returned job 
	information, program copies the result to this field, 
	so user can read from user land.

	g. int job_info_size;
	size of job_info
	
	h. int job_priority;
	priority of a job, can be changed though job 'priority' 
	

3. Producer:

Run user land excutable xproducer from command line would call the syscall to submit
a job to jobQueue.
Once user provides a valid job, xproducer creates a user AsynJob in user land,
pass to kernel by calling long submitjob(void *arg, int argslen).
kernel calls int produce(AsynJob *job) to copy user AsynJob to kernel 
add kernel AsynJob to jobQueue sorted by priority after mutex lock, but if 
job_type is 'list', 'remove' or 'priority', produce() will manipulate jobQueue 
directly without adding this job to jobQueue, which is designed for high latency
job as device I/O, network stream, etc.

Once the job is successfully submitted, the thread of producer ends. We could adding
jobs as much as number of max_cnt jobs exist in the jobQueue, if it grows bigger than
max_cnt, put that thread in to WAIT and wait to be waken up by consumer when jobs
decrease in jobQueue.

4. Consumer:

Start the consumer thread when install module and stop it when remove module.
During this two point, the consumer thread is either at RUNNING or WAIT state.

Wrap all the jobQueue operations and job processing inside a loop. Inside this
loop, we carefully check the jobQueue status and decide what to do.

a. jobQueue is empty, put thread to sleep
b. jobQueue size changes from 1 to 0, process this only job and put thread to sleep
c. jobQueue size changes from max_cnt > n > 1 to n-1, process this job and continue
   to the start of loop
d. jobQueue size falls below max_cnt, wake up producer in wait queue and process 
   this job
e. when waken up by producer, continue to the start of loop

When remove the module, clean jobQueue and stop consumer thread before actually
removing it.

===========================================================
=                                                         =
=                     Reference                           =
=                                                         =
===========================================================


Reference:
1. linked list:
http://www.makelinux.net/ldd3/chp-11-sect-5
2. mutex lock:
https://www.kernel.org/doc/htmldocs/kernel-locking/Examples.html
3. thread + mutex locking:
http://cis.poly.edu/cs3224a/Code/ProducerConsumerUsingPthreads.c
4. userland example: producer consumer
https://macboypro.wordpress.com/2009/05/25/producer-consumer-problem-using-cpthreadsbounded-buffer/

