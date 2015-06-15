#include "errno.h"
#include "stdarg.h"
#include "string.h"
#include "type.h" 

extern void flog(log_sev sev, const char* fmt, ...);

char str_errno[SIZE_ERRNO]; 	// stringa che contiene una descrizione dell'errore
word errno=0;	


//funzione che stampa la variabile errno dopo la stringa passata
//come argomento

void perror(const char *str) { 
	flog(LOG_WARN, "%s : %s", str, str_errno); 
} 

//Setta la variabile errno con la stringa passata 
void set_errno(word err, const char * str, ... ) { 

  va_list p=NULL; 

	errno=err; 
	memset(str_errno,0, SIZE_ERRNO);

	if(!str) 
	  return; 
	
	va_start(p,str);
	vsnprintf(str_errno, SIZE_ERRNO, str, p); 
	va_end(p); 

}

void reset_errno() { 
  errno=0; 
  memset(str_errno,0, SIZE_ERRNO); 
  
}
