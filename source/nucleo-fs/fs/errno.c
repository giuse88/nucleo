
#include "errno.h" 

//funzione che stampa la variabile errno dopo la stringa passata
//come argomento

void perror(const char *str) { 
	flog(LOG_WARN, "%s : %s", str, errno); 
} 

//Setta la variabile errno con la stringa passata 
void set_errno(const char *str) { 

	int size=0; 


	if(str == NULL) 
		return; 
	
	size=strlen(str); 
	memset(errno,0, SIZE_ERRNO); 
	strncpy(errno, str,(( size < SIZE_ERRNO-1) ? size : SIZE_ERRNO-1)); 


}

void reset_errno() { 
  
  
  memset(errno,0, SIZE_ERRNO); 
  
}