#ifndef _SYSTEM_CALL_FS_ 
#define _SYSTEM_CALL_FS_ 


#define O_RDONLY             0x00
#define O_WRONLY             0x01
#define O_RDWR               0x02
#define O_CREAT              0x40  
#define O_DIRECTORY    	     0x10000

#define IS_DIRECTORY(X) (X & O_DIRECTORY)
#define IS_CREAT(X)     (X & O_CREAT)
#define IS_READ(X) 	!(X & 0x3    ) 			// i primi tre bit devono essere a zero 
#define IS_WRITE(X) 	(X & O_WRONLY)
#define IS_RDWR(X) 	(X & O_RDWR)

//si fa riferimento all'inizio del file: il valore (sempre positivo) di offset indica direttamente la nuova posizione corrente.
#define SEEK_SET 0		

//si fa riferimento alla posizione corrente del file: ad essa viene sommato offset (che può essere negativo e positivo) per ottenere la nuova posizione corrente.
#define SEEK_CUR 1

//si fa riferimento alla fine del file: alle dimensioni del file viene sommato offset (che può essere negativo e positivo) per ottenere la nuova posizione corrente.

#define SEEK_END 2

#define IS_SEEK_END(X)  (X==SEEK_END) 
#define IS_SEEK_SET(X)  (X==SEEK_SET)
#define IS_SEEK_CUR(X)	(X==SEEK_CUR)

// alineo la struttura a quattro byte
#define ADDR_VOL    0
#define ADDR_PATH   ADDR_VOL + 4
#define ADDR_CWD    ADDR_PATH + MAX_PATH
#define ADDR_FCB    ADDR_CWD + sizeof(FCB) 



int open (const char * path , dword ); 
int test_system_call(); 
size_t read(int fd, void *buf, size_t count);
size_t write(int fd, void *buf, size_t count); 
size_t lseek(int fd, size_t offset, int whence);

#endif 