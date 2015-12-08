#include "submitjob.h"

/*
 * Get checksum result from input file content.
 * @job including input file name, also contain
 *	the output value in job->jon_info
 */
int checksum(AsynJob *job){
	int err = 0;
	char dst[16];
	char src[MAX_BUF_LEN];
	int len = 16;

	// Input file
	err = open_read_file(job->job_target, src, MAX_BUF_LEN);

	if(err < 0){
		goto out;	
	}

	// checksum
	err = ecryptfs_calculate_md5_simplify(dst,src,len);	
	if(err < 0){
		goto out;
	}

	strcpy(job->job_info, (void *)dst);
	job->job_info_size = strlen(dst);

	out:
	return err;
}

/* Read file from input file name */
int open_read_file(char *file_name, char *content, int content_len)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int bytes;
	int err = 0;


	filp = filp_open(file_name, O_RDONLY, 0);
	err = validate_file(filp,0);	

	if(err != 0){
		return err;
	}       
 
	filp->f_pos = 0;
        oldfs = get_fs();
        set_fs(KERNEL_DS);

        bytes = filp->f_op->read( filp , content, content_len , &filp->f_pos);

	set_fs(oldfs);

	/* close the file */
    	if(filp != NULL){
		filp_close(filp, NULL);
	}
	return err;
}


/*
 * This function is to generate md5 encrypt key based on user input key value.
 * Reference document: /fs/ecryptfs/crypto.c
 *                     /Documentation/crypto/api-intro.txt
 * dst: encrypt key value
 * src: user input key value
 * len: user input key length
 * return: 0 succeed, nagetive failed with erro numbers
*/
int ecryptfs_calculate_md5_simplify(char* dst,char* src, int len){
	struct scatterlist sg;
	struct hash_desc desc;
	struct crypto_hash *tfm;
	int rc = 0;

	sg_init_one(&sg,(u8 *)src,len);
	tfm = crypto_alloc_hash("md5", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)){
			goto error_exit;
	}
	desc.tfm = tfm;
	desc.flags = 0;
	rc = crypto_hash_digest(&desc, &sg, len, dst);

error_exit:
	crypto_free_hash(tfm);
	return rc;
}

/*
	validate if filp is valid file
	@filp, struct file that needs to be validated.
	@flag, flag which indicates if filp is infilp or filp. 0 for infilp, 1 for outfilp.
	@err_code, actual error code.
	return 0 if no error,
		   
*/
int validate_file(struct file *filp, int flag){
	int err=0;
	if (!filp || IS_ERR(filp)){
		//printk("open file err %d\n", (int) PTR_ERR(filp));
		err=-ENOENT; 
		goto out;
	}
	if (!S_ISREG(filp->f_path.dentry->d_inode->i_mode)){
		err=-EISDIR;
		goto closefile;
	}
	if (!flag && !filp->f_op->read){
		err= -EACCES; 
		goto closefile;
	}
	if (flag && !filp->f_op->write){
		err= -EACCES; 
		goto closefile;
	}
	goto out;
closefile:
	if(filp) filp_close(filp, NULL);
out:
	return err;
}

/*
	write content in in_path to out_path for concatenate feature
	@out_path: output of concatenated file.
	@in_path: source file(s).
	return 0 if no error.
*/
int write_to_out_file(char* out_path, char* in_path){
	int err=-EINVAL;
	char *in_path_copy;
	char *token;
	char *buf;
	int bytes_read;
	int bytes_write;
	mm_segment_t oldfs;
	struct file *outfilp;
	struct file *infilp=NULL;
	buf=kmalloc(PAGE_SIZE,GFP_KERNEL);
	
	outfilp = filp_open(out_path, O_RDONLY, S_IRUSR);
	err=validate_file(outfilp, 0);
	if (err==0){
		if(outfilp) filp_close(outfilp, NULL);
		err=-EEXIST;
		goto out;
	}
		
	outfilp = filp_open(out_path, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	err=validate_file(outfilp, 1);
	if (err<0)
		goto out;
	
	outfilp->f_pos = 0;
	in_path_copy=kmalloc(strlen(in_path)+1,GFP_KERNEL);
	strcpy(in_path_copy,in_path);
	while (in_path_copy != NULL && strlen(in_path_copy)>0) {
        token = strsep(&in_path_copy, ";");
		infilp = filp_open(token, O_RDONLY, S_IRUSR);
		err=validate_file(infilp, 0);
		if (err<0)
			goto out;
		
		if(infilp->f_path.dentry->d_inode->i_ino == outfilp->f_path.dentry->d_inode->i_ino){
			err = -ENOSPC;
			goto out;
		}
				
		infilp->f_pos = 0;
		
		oldfs = get_fs();
	
		set_fs(KERNEL_DS);
		while ((bytes_read = infilp->f_op->read(infilp, buf, PAGE_SIZE, &infilp->f_pos))>0){
		bytes_write=outfilp->f_op->write(outfilp, buf, bytes_read, &outfilp->f_pos);
		if (bytes_read!=bytes_write){
			err=-EIO;
			set_fs(oldfs);
			goto out;
			}
		}
		
		set_fs(oldfs);
	}
		err=0;
	if(infilp) filp_close(infilp, NULL);
	if(outfilp) filp_close(outfilp, NULL);
out:
	if (buf) kfree(buf);
	return err;
}

/*
 * Concatenate files.
 */
int concatenate(AsynJob *job){
	int err=-EINVAL;
	err=write_to_out_file(job->job_target, (char*)job->job_info);
	if (err<0)
		goto out;
	
	err=0;
out:
	return err;
	
}

/*
	use crypto api to compress/decompress file.
	@job: job 
	@flag: indicate compress or decopress. 1 for compress, 0 for decompress.
	return 0 if no error.
	Note: When file is >4K, Crypto function crypto_comp_compress(...) returns error after 'some' iteration.  I tried 10K file, it returns error at last chunk which is <4K, but for a 70K file, it returns error at the 4th chunk. Probaliy I passed some wrong parameters.  
*/
int crypto_comp(AsynJob *job, int flag){
	int err=-EINVAL;
	char *src;
	char *dst;
	mm_segment_t oldfs;
	struct crypto_comp *tfm;
	struct file *infilp=NULL;
	struct file *outfilp=NULL;
	char *default_out_path;
	size_t dstlen=PAGE_SIZE;
	size_t bytes_read=0;
	size_t bytes_write=0;
	infilp = filp_open(job->job_target, O_RDONLY, S_IRUSR);
	err=validate_file(infilp, 0);
	if (err<0)
		goto out;	
	default_out_path=kmalloc(MAX_PATH_LEN+3,GFP_KERNEL);
	if (job->job_info_size>0){
		outfilp = filp_open(job->job_info, O_RDONLY, S_IRUSR);
		err=validate_file(outfilp, 0);
		if (err==0){
			if(outfilp) filp_close(outfilp, NULL);
			err=-EEXIST;
			goto out;
		}
		outfilp = filp_open(job->job_info, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	
	}
	else
		{	
		strcpy(default_out_path,job->job_target);
		if (flag)
			strcat(default_out_path,".z");
		else{
			if (strcmp(default_out_path+strlen(default_out_path)-2,".z")==0){
				default_out_path[strlen(default_out_path)-2]='\0';
				strcat(default_out_path,".u");
				printk("default out path %s\n",default_out_path);
			}
			else {
				err=-ENOMSG;
				goto out;
			}
		}
		outfilp = filp_open(default_out_path, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		}
	
		
	err=validate_file(outfilp, 1);
	if (err<0)
		goto out;	
	outfilp->f_pos = 0;		
	infilp->f_pos = 0;
	tfm = crypto_alloc_comp("deflate", 0, CRYPTO_ALG_ASYNC);

	src=kmalloc(PAGE_SIZE, GFP_KERNEL);
	dst=kmalloc(PAGE_SIZE, GFP_KERNEL);
	
	
	oldfs = get_fs();
	set_fs(KERNEL_DS);		
	while ((bytes_read = infilp->f_op->read(infilp, src, PAGE_SIZE, &infilp->f_pos))>0){
			memset(dst,0,PAGE_SIZE);
			if (flag)
				err= crypto_comp_compress(tfm, src, bytes_read, dst, &dstlen);
			else
				err= crypto_comp_decompress(tfm, src, bytes_read, dst, &dstlen);
					
			if(err) 
				{	err=-EINTR;
					goto freevar;
				}
			bytes_write=outfilp->f_op->write(outfilp, dst, dstlen, &outfilp->f_pos);
			
	}
	set_fs(oldfs);
	err=0;
freevar:
	if(src) kfree(src);
	if(dst) kfree(dst);
	if(default_out_path) kfree(default_out_path);
	if(infilp) filp_close(infilp, NULL);
	if(outfilp) filp_close(outfilp, NULL);
	crypto_free_comp(tfm);
out:
	return err;
}

/*
 * Compress file.
 */
int compress(AsynJob *job){
	return crypto_comp(job,1);
}

/*
 * Decompress file.
 */
int decompress(AsynJob *job){
	return crypto_comp(job,0);
}

