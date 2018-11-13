#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include "lame.h"

#define MAX_SIZE 				256
#define PCM_SIZE 				1024 * 8
#define MP3_SIZE 				1024 * 8
#define MAX_THREADS 		256
#define FILE_EXTENSION  5
#define FILE_FORMAT 		".mp3"

int is_wav(char* ptr);
void convert_wav2mp3(char * str);
char save_dir[MAX_SIZE]={'0'};

char sbuf[MAX_SIZE]={'0'};
char tbuf[MAX_SIZE]={'0'};
char zbuf[MAX_SIZE]={'0'};

int main(int argc , char ** argv)
{

	pthread_t  			th[MAX_THREADS]={0};
	DIR 			 			*file_dir;
	struct dirent 	*in_file;
	int 			 			ret;
	int    		 			i=0;			
	FILE  		 			*entry_file;
	size_t 					len;
	
	// Check for proper usage
	if(argc < 2 || argc > 2){
		printf("Invalid arguments--Usage: ./<exe> <folder>\n");
		return 0;
	}
	
	// Print Version
	printf("LAME Version - %s \n",get_lame_version());
	 	 
	len = strlen(argv[1]); 
	strncpy(save_dir,argv[1],len );

	if(save_dir[len-1] != '/') {
		save_dir[len] = '/';	
	}
			
	file_dir = opendir(argv[1]);
	  
	if(file_dir == NULL) {
		fprintf(stderr, "Error opening dir\n");
		return 0;
  }
 	

  // scan the directory for wav files
	while(in_file= readdir(file_dir)) {
		//Ignore current and previous directory
		if (!strcmp (in_file->d_name, "."))
            continue;
		if (!strcmp (in_file->d_name, ".."))    
            continue;
	
		strcpy(tbuf,sbuf);
		strcpy(tbuf,save_dir);
		strcat(tbuf,in_file->d_name);
		

		entry_file = fopen(tbuf, "rw");
	
		if (entry_file == NULL) {
			fprintf(stderr, "Error : Failed to open entry file %s\n",tbuf);
			perror("fopen failed");
			continue;
		}
		
		if(is_wav(tbuf)){
			// wav file found, spawn a thread for encoding
			strcpy(zbuf,tbuf);
			pthread_create(&th[i],NULL,(void*) convert_wav2mp3, zbuf );	
		}
	
		fclose(entry_file);
	} 
	pthread_exit(NULL);	
	return 0;
}

int is_wav(char* ptr )
{
    int 			pfds[2];
    char 			buf[MAX_SIZE]={'0'};
    int 			nread;
		char* 		pptr;
		int 			ret ;
	  
		pipe(pfds);		
		
		ret = fork();	
		if (ret == -1 ) {
			fprintf(stderr,"Error forking the process \n");
			abort();
		}
		
    if (!ret) {
        close(1);       
        // Redirecting the output to pipe
        dup(pfds[1]);   
        close(pfds[0]); 
        execlp("file", "file",ptr,NULL);        
    } else {
        close(0);     
        //Reading from the pipe  
        dup(pfds[0]); 
        close(pfds[1]);       
        nread= read(pfds[0],buf,256);
        
				// Check if the file is Wave audio
				pptr=strstr(buf,"RIFF");
				if(pptr==NULL) {
					return 0;
				}
				else{
					return 1;
				}
    }

}


void convert_wav2mp3(char * str)
{
  char 					*ptr;
 	int  					ret;
	char 					*token;
	short 				pcm_buf[PCM_SIZE *2];
	unsigned char mp3_buf[MP3_SIZE];
	int 					nread;
	int 					nwrite;
	lame_t 				lame;
	size_t 				len;
 	FILE 					*pcm ; 
	FILE 					*mp3 ; 
 	
 	pcm = fopen(str, "rb");
	if(!pcm){
		fprintf(stderr,"Error opening PCM file -%s \n", str);
		abort();
	}
	
	len = strlen(save_dir);
	// save the file name
	token = strtok(str,".");

	len = strlen(token);
	ptr = malloc(sizeof(char) * len + FILE_EXTENSION);
	strncpy(ptr,token,len);
	strcat(ptr,FILE_FORMAT);
	
	mp3 = fopen(ptr, "wb");
 	if(!mp3){
 		fprintf(stderr,"Error opening Mp3 file -%s \n", ptr);
		abort();
 	}

 //initialising the file
	lame = lame_init();

 //set params for different settings
	lame_set_VBR(lame,vbr_default);
//	lame_set_mode(lame,3);
//	lame_set_in_samplerate(lame,48000);
// lame_set_quality(gfp,5); 

	ret= lame_init_params(lame);
	
	if(ret<0){
		printf("Lame Error , Err Code-%d\n", ret);
  }

	//do lame encoding
	do
	{
		nread = fread(pcm_buf,2*sizeof(short),PCM_SIZE,pcm);
		if(nread !=0){
			int samples = nread;
		//	int samples ;
			short buffer_l[samples];
			short buffer_r[samples];
			int i =0 , j=0;
			for(i=0; i< samples; i++)
			{
				buffer_l[i] = pcm_buf[j++];
				buffer_r[i] = pcm_buf[j++];
			}        
			nwrite = lame_encode_buffer (lame, buffer_l,buffer_r, nread,mp3_buf,MP3_SIZE);
		}
		else{
				nwrite = lame_encode_flush(lame,mp3_buf,MP3_SIZE);
		}

		if(nwrite<0)
		{
			printf("Error in write - %d \n",nwrite);
		}


		fwrite(mp3_buf,nwrite ,1 ,mp3);
	
	} while(nread !=0);


	lame_close(lame);

	fclose(mp3);
	fclose(pcm);
  
}

