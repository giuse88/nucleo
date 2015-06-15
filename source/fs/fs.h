#ifndef FS_H 
#define FS_H 

#include "type.h" 
#include "direntry.h" 


#define MAX_FILE 5 		// numero massimo di file gestibili dal sistema
#define SIZE_FCB sizeof(FCB) 

#define MAX_NAME 		255	// lunghezza massima nome lungo 
#define MAX_PATH 		260 	// lunghezza massima path 

#define SIZE_VOLUME 4 
#define MUTEX 1			
#define ROOT_CLUSTER 2
/*struttre che mi permettono l'astrazzione del file system*/ 


#define N_ATA    2 
#define N_DISK   2
#define SECT_SIZE 512 

#define ERROR -1

#endif 

