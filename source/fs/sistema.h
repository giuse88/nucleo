/*SISTEMA.H 
 *FIle di interfaccia con in modulo sistema
 */


#ifndef INTERFACCIA_SISTEMA_H 
#define INTERFACCIA_SISTEMA_H 

#include "tipo.h"
#include "type.h" 
#include "costanti.h"

int   get_partizione ( dword ata, natl disco , natl tipo, int indice); 
int   read_part_n  (natl ata, natl disco, natl indice_partizione, natl indice_settore, natl n_blocchi, void * buf);
int   write_part_n (natl ata, natl disco, natl indice_partizione, natl indice_settore, natl n_blocchi, void * buf); 
void  flog(log_sev, cstr fmt, ...);
void* mem_alloc(natl dim, natl balign );
void  mem_free(void *p);
natl  sem_ini(int);	
void  sem_wait(natl);	
void  sem_signal(natl);
dword io_space_init(natb init[], natl size); 
dword io_space_write(natl offset, const natb vetto[], natl quanti); 
dword io_space_read(natl offset, natb * vetti, natl quanti); 
#endif

