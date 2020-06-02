#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/mman.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


#define MMAP_ARGS 4
#define RW_ARGS 3

int copy_read_write(int fd_from, int fd_to); // copy file method read/write
int copy_mmap(int fd_from, int fd_to);		// copy file using mmap
void validateNumberOfArguments(int argc, bool memoryCopy); // method to check if the methods arguments are correct
const char **readFilesPath(int argc, char *argv[]);  // method which read the files names from argument inputs
void help(); // method which printf help info

int main(int argc, char *argv[])  
{ 
    int opt; 
    bool memoryCopy = false;   // flag which decide if it is mmap copy method or read/write copy method   

    while((opt = getopt(argc, argv, ":mh")) != -1)  // read params using getopt
    {  
        switch(opt){
	    case 'h': // if -h print help
		help();
		exit(0);
	    case 'm': // if -m set flag for ture and it will be mmap copy method
		memoryCopy = true;	
		break;
	    case '?': // other params means it is wrong input
		perror("FAILURE: unknown operation\n");
		exit(-1);
	}  
    }

  
   
   validateNumberOfArguments(argc, memoryCopy); // validate arguments

   const char **filesPath; // variable for files paths
   filesPath = readFilesPath(argc, argv); // read files paths from arguments and return array when index 0 -> source file, 1-> destination file

	
   int src_file = open(filesPath[0], O_RDONLY); // open source file which path is under index 0
   if(src_file < 0){ // check if file was correctly opened
		perror("FAILURE: failed to open src file\n");
		exit(-1);
   }
    
   struct stat src_stat; // stat structure contains many fields for instance file type and mode
   if(fstat(src_file, &src_stat) == -1){ // obtain/check the file status, Upon successful completion, 0 shall be returned. Otherwise -1
	perror("FAILURE: failed to load src file mode information\n");
	exit(-1);
   }

   int dst_file = open(filesPath[1], O_RDWR | O_CREAT, src_stat.st_mode); // st_mode File type and mode, O_RDWR-read/write, O_CREAT-create file when not exist
   if(dst_file < 0){  	// obtain/check the file status, Upon successful completion, 0 shall be returned. Otherwise -1
		perror("FAILURE: failed to open dst file\n");
		exit(-1);
   }  

   int result = memoryCopy ? copy_mmap(src_file, dst_file) : copy_read_write(src_file, dst_file); // if is -m flag make copy_mmap else make copy_read_write
   return result; 
} 


int copy_read_write(int fd_from, int fd_to){
	static const int BUFFER_SIZE = 4096; // define buffer size for copy file
	char buffer[BUFFER_SIZE]; // chars array with the size of buffer

	int readFile, writeFile;

	//loop for coping file
	while((readFile = read(fd_from, buffer, BUFFER_SIZE)) > 0){ // read source file to the buffer
		writeFile = write(fd_to, buffer, readFile); // write to destination file, On success, the number of bytes written is returned.  On error -1 returned
		if(writeFile <= 0){ 
			perror("FAILURE: RW copy method has failed\n");
			return -1;
		}
	}
	
	return 1;
}

int copy_mmap(int fd_from, int fd_to){
	struct stat src_stat; // stat structure contains many fields for instance file type and mode
	if(fstat(fd_from, &src_stat) == -1){ // obtain/check the file status, Upon successful completion, 0 shall be returned. Otherwise -1
		perror("FAILURE: failed to load src file mode information\n");
		exit(-1);
	}

	char *src_buf;
	//PROT_READ  Pages may be read, PROT_WRITE Pages may be written, MAP_SHARED Updates to the mapping are visible to other processes mapping the same region
	src_buf = mmap(NULL, src_stat.st_size, PROT_READ, MAP_SHARED, fd_from, 0); 
	if(src_buf == (void*)-1){
		perror("FAILURE: failed to map memory\n");
		exit(-1);
	}

	if(ftruncate(fd_to, src_stat.st_size)){
		perror("FAILURE: failed to change output file size\n");
		exit(-1);
	}

	char *dst_buf;
	//PROT_READ  Pages may be read, PROT_WRITE Pages may be written, MAP_SHARED Updates to the mapping are visible to other processes mapping the same region
	dst_buf = mmap(NULL, src_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_to, 0); // On error mmap will return the integer -1 cast to a pointer to void.
	if(dst_buf == (void*)-1){
		perror("FAILURE: failed to map memory\n");
		exit(-1);
	}

	dst_buf = memcpy(dst_buf, src_buf, src_stat.st_size); // On error mmap will return the integer -1 cast to a pointer to void.
	if(dst_buf == (void*)-1){
		perror("FAILURE: failed to copy memory\n");
		exit(-1);
	}

	return 1;
}

void validateNumberOfArguments(int argc, bool memoryCopy)
{
  if(memoryCopy == true && argc != MMAP_ARGS){ // if mmap method and argc != 4 then number of arguments is not enought
	printf("Wrong number of arguments\n");
	help();
	exit(-1);
  }
  
  if(memoryCopy == false && argc != RW_ARGS){ // if RW method and argc != 4 then number of arguments is not enought
	printf("Wrong number of arguments\n");
	help();
	exit(-1);
  }

}

const char **readFilesPath(int argc, char *argv[]){ // read paths to files
   static const char *filesPath[2]; // index 0 source file, index 1 destination file
   int fileIndex = 0;
   for(; optind < argc; optind++){ // iterate throw all inputs arguments
	filesPath[fileIndex] = argv[optind];    // save inputs arguments as a files names
	fileIndex++;
   } 

return filesPath;
}

// help info printf
void help()
{
    printf("The structure of command is as bellow:\n");
    printf("copy [-m] <file_name> <new_file_name>\n");
    printf("-m => copy using mmap method, otherwise simple read/write method\n");
}
