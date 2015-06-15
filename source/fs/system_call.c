/* SYSTEM CALL */ 

#include "string.h" 
#include "direntry.h" 
#include "fs.h" 
#include "errno.h" 
#include "volumi.h" 
#include "system_call.h"
#include "type.h"


extern TABELLA_VOLUMI *tabella; 





/****************************************************************
 * Funzione che apre un entry individuata da path. 		*
 * La ricerca della nuova entry parte da cwd e il risultato lo  *
 * inserisce in new. 						*
 ****************************************************************/

BOOL open_path(const char * path, FCB* cwd , FCB * new) {
 
  dword size_path=0; 
  char *token=NULL, *my_path=NULL; 
  FCB  * padre=NULL , *figlio=NULL; 
  VOL_PTR vol=NULL; 
  
  if ( !path || !cwd || !new ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE;
  }
  
 size_path=strlen(path);
  
  if (size_path < 1 || size_path > MAX_PATH) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE;
  }
  
  if(!(my_path=mem_alloc(MAX_PATH,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }  
  
  if(!(figlio=mem_alloc(sizeof(FCB),1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    mem_free(my_path); 
    return FALSE; 
  } 
  
  if(!(padre=mem_alloc(sizeof(FCB),1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    mem_free(my_path);
    mem_free(padre); 
    return FALSE; 
  }
  
  memset(padre, 0, sizeof(FCB)); 
  memset(figlio, 0, sizeof(FCB)); 
  memset(my_path, 0, MAX_PATH); 
  
  // GESTIONE PATH ASSOLUTO + VOLUME 
  if (size_path >= 3 && isalpha(*path) && *(path+1)==':' &&  *(path+2) =='/') { 

	if(!(vol=get_volume(*path))) { 
	    set_errno(EINVAL,"Volume errato %c (%s-line%d)", *path,  __FILE__, __LINE__); 
	    goto error_open_path; 
	}
	strncpy(my_path, (path+2), strlen(path+2)); 
	memcpy(padre, (FCB*)(vol->fcb_root), sizeof(FCB)); 
  } // GESTIONE PATH ASSOLUTO 
  else if ( *path=='/') { 
      	strncpy(my_path, path, strlen(path)); 
	vol=cwd->volume; 
	memcpy(padre, (FCB*)(vol->fcb_root), sizeof(FCB)); 
  } // GESTIONE PATHRELATIVO  
  else {
	strncpy(my_path, path, strlen(path)); 
	memcpy(padre, cwd, sizeof(FCB)); 
  }
  
   memcpy(figlio, padre, sizeof(FCB)); 
  
  
  
  token=strtok(my_path, "/"); 
   
   while(token) {
//      flog(LOG_INFO, "tok : %s", token); 
      if((!open_entry(token,padre,figlio))) {	// prima fa la fuznione  
	perror("Open Entry"); 
	
	if (strtok(NULL, "/"))
	  set_errno(EINVAL,"Errore path (%s-line%d)", __FILE__, __LINE__); 
	 else 
	  set_errno(ENOENT, "Non presente %s", token); 
	 
	goto error_open_path; 
      }
      
    token=strtok(NULL, "/"); 
    memcpy(padre, figlio, sizeof(FCB)); 

   }
      
    memcpy(new, figlio, sizeof(FCB));
      
    mem_free(my_path);
    mem_free(padre); 
    mem_free(figlio); 
    return TRUE;
  
error_open_path : 
    mem_free(my_path);
    mem_free(padre); 
    mem_free(figlio); 
    new=NULL; 
    return FALSE; 
}





BOOL create_pathassoluto(const char *path, const char * pwd, char *path_assoluto, size_t size_buffer) {

   char * double_dot=NULL; 
   char path_temp[MAX_PATH];  
   char * token=NULL; 
  
   if ( !path  || !pwd || !path_assoluto) {
     set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
     return FALSE;
   } 
 
    memset(path_temp,0, MAX_PATH); 
    memset(path_assoluto,0, size_buffer); 

   
    
    
    if ((double_dot=strchr(path, ':'))) {  // se è zero non è stato specificato nessun volume 
	
	 flog(LOG_DEBUG,"PATH %s", double_dot); 

	if(*(double_dot+1) != '/' || double_dot==path) {
	  set_errno(EINVAL,"Path errato (%s-line%d)", __FILE__, __LINE__); 
	  return FALSE;
	}
	
	strncpy(path_temp, (double_dot+1), MAX_PATH); 
    }else if ( *path== '/')  { 
	strncpy(path_temp, path, MAX_PATH); 
    } else 
    { 
	strncpy(path_temp, pwd, MAX_PATH);
	strncat(path_temp, path, MAX_PATH-strlen(path_temp)); 
    }
    
#ifdef DEBUG_FS    
    flog(LOG_DEBUG,"PWD            : %s",pwd);
    flog(LOG_DEBUG,"PATH           : %s",path_temp);
#endif    
    // ora creiamo il path assoluto 
    
    token=strtok(path_temp, "/"); 
    
    while(TRUE) {
    
      if (!token) { 				// FINE
	   strcat(path_assoluto, "/"); 
	   break; 
      }	
      else if(!strncmp(token, ".", 2)) {		// DOT 
	  token=strtok(NULL, "/"); 
	  continue; 
      }
      else if(!strncmp(token, "..", 3)) { 	// DOT DOT 
	  // devo eliminare il token precedente 
	  // devo fare in modo che alla prossima scrittura si sovrascriva 
	  char *chr=NULL; 
	  int size=0; 
	  
	  chr=strrchr(path_assoluto, '/');
	  flog(LOG_DEBUG, "CHR %s", chr); 
	  size=strlen(chr); 
	  memset(chr, 0, size);		//elimino  token  
	  token=strtok(NULL, "/"); 
      }
      else { 
	  strncat(path_assoluto, "/",MAX_PATH-strlen(path_assoluto)); 
	  strncat(path_assoluto, token, MAX_PATH-strlen(path_assoluto));   
	  token=strtok(NULL, "/"); 
      }
	
    }// while 
    
#ifdef DEBUG_FS
    flog(LOG_DEBUG, "Path assoluto : %s", path_assoluto); 
#endif     

    return TRUE; 
}






//////////////////////////////////////////////////////////////////////////
//////////////////// GESTIONE IOSPACE ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////




// funzione che inserisce un file control block nello spazio IOSPACE 
int list_fcb() {

  
  dword size=(sizeof(FCB) *MAX_FILE); 
  dword offset=SIZE_VOLUME+MAX_PATH; 
  byte * buf = NULL; 
  int i=0,r=0; 
  FCB * fcb=NULL; 
  
  flog(LOG_INFO,"LIST FCB "); 
  

    
  
  buf=mem_alloc(size, 1); 
  
  if (!buf) { 
   set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
   return -1; 
  }
  
  memset(buf,0,size); 
 
  //leggo il buffer
  if ((r=io_space_read(offset, buf, size))) { 
    set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
    mem_free(buf); 
    return -1; 
  }
     
   fcb=(FCB*)buf; 
  
   for ( i=0; i< MAX_FILE; i++, fcb++) {
    
	if(!fcb->volume)
	 continue; 
	print_fcb(fcb); 
	
    }
  
   flog(LOG_INFO," FINE LIST FCB "); 
  return 0; 
  
}

//funzione che rimuove un fcb 

int remove_fcb( int indice) {

  
  dword size=sizeof(FCB); 
  byte * buf = NULL; 
  int r=0; 

 // flog(LOG_WARN, "Remove "); 
  
  if ( indice < 0 || indice >=MAX_FILE) {
    set_errno(EINVAL, "Parametro non corretto (%s-line%d)", __FILE__, __LINE__);
    return -1; 
  }
    
  
  buf=mem_alloc(size, 1); 
  
  if (!buf) { 
   set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
   return -1; 
  }
  
  memset(buf,0,size); 
 
  if ((r=io_space_write(ADDR_FCB+(indice*SIZE_FCB),buf, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    mem_free(buf); 
    return -1; 
  }

  mem_free(buf); 
   
  return 0; 
  
}



/************************************************************************************
 * funzione che inserisce un FCB nell'area di memoria IOSPACE nella memoria sistema *
 * Se ha successo riporta l'indice, altrimenti riporta un valore negativo e setta   *
 * errno.									    *
 ************************************************************************************/

int insert_fcb(const FCB * file) {
  
  dword size=(sizeof(FCB)*MAX_FILE); 
  byte * buf = NULL; 
  int i=0,r=0; 
  FCB * fcb=NULL, *fcb_tmp=NULL; 
  
 
  if (!file) {
    set_errno(EINVAL, "Parametro non corretto (%s-line%d)", __FILE__, __LINE__);
    return -1; 
  }
    
  
  buf=mem_alloc(size, 1); 
  
  if (!buf) { 
   set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
   return -1; 
  }
  
 
  //leggo il buffer
  if ((r=io_space_read(ADDR_FCB, buf, size))) { 
    set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
    mem_free(buf); 
    return -1; 
  }
  
  fcb=(FCB*)buf; 
  fcb_tmp=(FCB*)buf; 
  
  
  //scorro finché non trovo una posizione libera 
  for ( i=0; i< MAX_FILE; i++, fcb++) {
    
    if (fcb->volume == 0)  {
	if ((r=io_space_write(ADDR_FCB+(i*SIZE_FCB),(const natb *)file , SIZE_FCB))) { 
	    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
	    mem_free(buf); 
	    return -1; 
	}else {
	   // print_fcb(file); 
	    return i; 
	}
    } //if
  }
  
  set_errno(EMFILE,"Troppi File aperti"); 
  mem_free(buf); 
  return -1; 
  
}


// funzione che verifica che ci siano poszioni libere prima di una creazione 


BOOL free_fcb() {
  
  dword size=(sizeof(FCB) *MAX_FILE); 
  dword offset=SIZE_VOLUME+MAX_PATH; 
  byte * buf = NULL; 
  int i=0,r=0; 
  FCB * fcb=NULL, *fcb_tmp=NULL; 
  
   
  buf=mem_alloc(size, 1); 
  
  if (!buf) { 
   set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
   return FALSE; 
  }
  
  //leggo il buffer
  if ((r=io_space_read(offset, buf, size))) { 
    set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
    mem_free(buf); 
    return FALSE; 
  }
  
  fcb=(FCB*)buf; 
  fcb_tmp=(FCB*)buf; 
  
  //poichè non sono accorpati possono esistere dei buchi per evitare di aprire lo 
  // stesso file lo cerco 
  
  for ( i=0; i< MAX_FILE; i++, fcb_tmp++)     
    if (fcb_tmp->volume == 0)  
	return TRUE;      
   
   return FALSE; 
}
  

BOOL create_file (const char * path, FCB* cwd, FCB * new) {
  
    char  name_file[MAX_NAME];
    char  my_path[MAX_PATH]; 
    char * chr=NULL; 
    FCB  * temp=NULL; 
    
    if ( !path || !new || !cwd ) {
      set_errno(EINVAL, "Parametro non corretto (%s-line%d)", __FILE__, __LINE__);
      return FALSE; 
    }
    
    if (!(temp=mem_alloc(sizeof(FCB), 1))) { 
	 set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	 return FALSE; 
    }
    
    memset(name_file,0, MAX_NAME); 
    memset(my_path, 0, MAX_PATH); 
    memset(temp, 0, sizeof(FCB));
    
    chr=strrchr(path, '/'); 
    
    if ( chr == NULL ) { 
	strncpy(name_file, path, strlen(path)); 
	memcpy(temp, cwd, sizeof(FCB)); 
    }else if ( chr == path) { 
	chr++; 
	memcpy(temp, cwd->volume->fcb_root, sizeof(FCB));
	strncpy(name_file,chr, strlen(chr)); 
    }
    else 
    { 
	chr++;
	strncpy(my_path, path, chr-path); 
	strncpy(name_file,chr, strlen(chr)); 
 
	if (!open_path(my_path, cwd, temp)){ 
	      new=NULL; 
	      mem_free(temp); 
	      return FALSE; 
	 } 
    }

  flog(LOG_DEBUG, "Creo il file %s", name_file); 
  flog(LOG_DEBUG, "PATH %s", my_path); 


     if (!create_entry(name_file,ATTR_ARCHIVE, 0,0, temp, new)) 
     { 
       new=NULL; 
       mem_free(temp); 
       return FALSE; 
     }
      
      mem_free(temp); 
      return TRUE; 
}


 
int lseek_fcb( FCB * file, int offset, int whence){ 
   
  if ( file == NULL  || whence <0) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL;
  }
  
  if ( !file->volume || !file->cluster_father) { 
    set_errno(EINVAL,"FCb errato (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL;
  }
    
   
  if (IS_SEEK_SET(whence)) { 
    
      //riferimento all'inizio del file 
      if (offset < 0 ){ 
	   set_errno(EINVAL,"Offset errato (%s-line%d)", __FILE__, __LINE__); 
	  return -EINVAL;
      }
      // può essere maggiore della grandezza del file 
      // è in quel caso vien esteso dalla write successiva 
      file->pos_corr=offset; 
    
  }else if ( IS_SEEK_CUR (whence)) {
      //riferimento alla posizione corrente 
      file->pos_corr+=offset; 
  }
  else if ( IS_SEEK_END(whence)) {
    //riferimento alla fine 
   file->pos_corr=file->size; 
   file->pos_corr+=offset; 
  }
    
  else {   
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL;
  }
 
   return file->pos_corr; 
  
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////     SYSTEM CALL 	////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/*OPEN **************************************************************************
 * Apre il file indicato da path nella modalità indicata da mode.		*	
 * La funzione ritorna il file descriptor in caso di successo e -1 in caso di	*
 * errore. In questo caso la variabile errno assumerà uno dei valori:		*
 *										*
 *   EEXIST pathname esiste e si è specificato O_CREAT e O_EXCL.		*
 *   EISDIR pathname indica una directory e si è tentato l'accesso in scrittura.*
 *   ENOTDIR si è specificato O_DIRECTORY e pathname non è una directory.	*
 * 										*
 * Mode puo assumere i seguenti valori : 					*
 *	 O_RDONLY	apre il file in sola lettura.				*
 *	 O_WRONLY	apre il file in sola scrittura.				*
 *	 O_RDWR	        apre il file in lettura/scrittura.			*
 *	 O_CREAT	se il file non esiste verrà creato.			*
 *	 O_DIRECTORY	se path non è una directory la chiamata fallisce        *
 ********************************************************************************/




int c_open (const char * path, dword mode) { 

   FCB *file_fcb=NULL, *cwd=NULL; 
   dword r=0; 
   BOOL  ret=FALSE; 
   int fd=0;   

    if (!tabella) {
	set_errno(EINVAL, "Non è inizializzato nessun file system"); 
	return ERROR; 
    }
   
    if (!path) {
	set_errno(EINVAL, "Parametro non corretto (%s-line%d)", __FILE__, __LINE__);
	return ERROR; 
    }
   
    if (!(cwd=mem_alloc(sizeof(FCB), 1))) { 
	 set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	 return ERROR; 
    }
  
    if (!(file_fcb=mem_alloc(sizeof(FCB), 1))) { 
	 set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	 mem_free(cwd); 
	 return ERROR; 
    }
    
   // prelevo informazioni dall'area privata del processo   
   if ((r=io_space_read(ADDR_CWD, (natb *)cwd, sizeof(FCB)))){
      set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
      mem_free(cwd); 
      return -1; 
   }
   
 
  ret=open_path(path, cwd, file_fcb); 

  
  if ( !ret && errno!=ENOENT) 
      goto errore_open; 

  // se il file non esiste 
  if ( !ret && IS_CREAT(mode) && !IS_DIRECTORY(mode)) { 
     // gestisco creazione del file 
      if (create_file(path, cwd, file_fcb)) 
	  goto successo_open; 
      else 
	  goto errore_open;; 
  }
  
    // se non esiste ritorno errore 
    if (!ret)  {
	      set_errno(ENOENT,"No such file or directory"); 
	      goto errore_open; 
    }
    
    // se è settato create error 
   if (IS_CREAT(mode)) {
	      set_errno(EEXIST, "File Exist"); 
	       goto errore_open; 
    }
    
   if ( IS_DIRECTORY(mode) &&  file_fcb->type != ATTR_DIRECTORY) {
	      set_errno(ENOTDIR, "Is not Directory");  
	      goto errore_open;
    }
    
    if(!IS_DIRECTORY(mode) && file_fcb->type == ATTR_DIRECTORY) {
	      set_errno(EISDIR, "Is  Directory"); 
	      goto errore_open;
    }
  
    if ( IS_DIRECTORY(mode) &&  ( IS_WRITE(mode) ||  IS_RDWR(mode) ) ) {
	      set_errno(EISDIR, "Is  Directory "); 
	      goto errore_open;
    }
    
    if (IS_READ(mode) && ( IS_WRITE(mode) ||  IS_RDWR(mode) )) {
	      set_errno(EINVAL, "Bad param"); 
	      goto errore_open;
   }
   
   if (IS_WRITE(mode) && ( IS_READ(mode) ||  IS_RDWR(mode) )) {
	      set_errno(EINVAL, "Bad param"); 
	      goto errore_open;
   }
   

 
successo_open :

   file_fcb->mode=mode; 

   fd=insert_fcb(file_fcb);   
   if ( fd < 0) {
     goto errore_open; 
   } 
 
  //0ist_fcb(file_fcb);
   
  reset_errno();
  mem_free(file_fcb);
  mem_free(cwd);
  return fd;
	  


errore_open:
  mem_free(file_fcb);  
  mem_free(cwd);
  return ERROR;

}

/**** CLOSE ****************************************
 * Funzione che chiude un file individuato da fd   *
 ***************************************************/

int c_close(int fd) { 
  
  if (!tabella) {
    set_errno(EINVAL, "Non è inizializzato nessun file system"); 
    return ERROR; 
  }
  
  reset_errno(); 
  if ( remove_fcb(fd) < 0 ) {
      set_errno(EINVAL, "Parametro Errato");
      return ERROR; 
  } else { 
      reset_errno(); 
      return 0; 
  }
  
}

/**********************************************************
 * Cerca di leggere count byte dal file fd al buffer buf. *
 * La funzione ritorna il numero di byte letti in caso di *
 * successo e -1 in caso di errore, nel qual caso errno   *
 * assumerà uno dei valori:                               *
 **********************************************************/ 

int  c_read(int fd, void *buf, size_t count){
  
  FCB * file=NULL; 
//  dword size=sizeof(FCB); 
  int r=0,n_read=0; 
  
  if (!tabella) {
    set_errno(EINVAL, "Non è inizializzato nessun file system"); 
    return ERROR; 
  }
  
  if (  fd<0 || fd>=MAX_FILE  || buf==NULL || count <0) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL;
  }

  if(!(file=mem_alloc(SIZE_FCB,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return -ENOMEM; 
  } 
 
  
  memset(file,0,SIZE_FCB); 
  
  if ((r=io_space_read(ADDR_FCB+(fd*SIZE_FCB),(natb*)file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    mem_free(file); 
    return ERROR; 
  }

   if((n_read=read_fcb(file, buf, count)) < 0 ) { 
     perror("Read");  
     mem_free(file); 
     return ERROR; 
   }

   if ((r=io_space_write(ADDR_FCB+(fd*SIZE_FCB),(const natb*)file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    mem_free(file); 
    return ERROR; 
  }


   mem_free(file); 
   reset_errno(); 
   return n_read; 
} 
  
/*************************************************
 * Scrive count byte dal buffer buf sul file fd. *
 * La funzione ritorna il numero di byte scritti *
 * in caso di successo e -1 in caso di errore, 	 *
 * nel qual caso errno assumerà uno dei valori:  *
 *************************************************/


int  c_write(int fd, const void *buf, size_t count){
  
  FCB * file=NULL; 
//  dword size=sizeof(FCB); 
  int r=0,n_write=0; 
  
  if (!tabella) {
    set_errno(EINVAL, "Non è inizializzato nessun file system"); 
    return ERROR; 
  }
  
  if (  fd<0 || fd>=MAX_FILE  || buf==NULL || count <0) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL;
  }

  if(!(file=mem_alloc(SIZE_FCB,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return -ENOMEM; 
  } 
 
  
  memset(file,0,SIZE_FCB); 

  if ((r=io_space_read(ADDR_FCB+(fd*SIZE_FCB),(natb *)file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    mem_free(file); 
    return ERROR;  
  }

   if((n_write=write_fcb(file, buf, count)) < 0 ) { 
     perror("write"); 
     mem_free(file); 
     return ERROR; 
   }

   if ((r=io_space_write(ADDR_FCB+(fd*SIZE_FCB),(const natb *)file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    mem_free(file); 
    return ERROR; 
  }


   mem_free(file); 
   reset_errno(); 
   return n_write; 
} 


/*********************************************************************
 * Funzione che setta la nuova posizione sul file individuato da fd. *
 * La nuova posizione è impostata usando il valore specificato da    *
 * offset, sommato al riferimento dato da whence; quest'ultimo può   *
 * assumere i seguenti valori:					     *
 * SEEK_SET :si fa riferimento all'inizio del file:		     *
 *	     il valore (sempre positivo) di offset		     *
 *	     indica direttamente la nuova posizione corrente.        *
 * SEEK_CUR :si fa riferimento alla posizione corrente del file:     *
 *	     ad essa viene sommato offset (che può essere negativo   *
 *	     e positivo) per ottenere la nuova posizione corrente.   *
 * SEEK_END  si fa riferimento alla fine del file: alle dimensioni   *
 *	     del file viene sommato offset (che può essere negativo  *
 *	     e positivo) per ottenere la nuova posizione corrente.   *
 * Dato che la funzione ritorna la nuova posizione, usando il valore *
 * zero per offset si può riottenere la posizione corrente nel file  *
 * chiamando la funzione con lseek(fd, 0, SEEK_CUR).                 *
 *********************************************************************/

int c_lseek(int fd, int new_offset, int whence){
  
  FCB * file=NULL;  
  int new_off=0;  
  int r=0; 
  
  flog(LOG_WARN,"LSEEK"); 
  
  if (!tabella) {
    set_errno(EINVAL, "Non e' presente nessun file system"); 
    return ERROR; 
  }
  
  if (  fd<0 || fd>=MAX_FILE ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL;
  }
  
  
  if(!(file=mem_alloc(SIZE_FCB,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return -ENOMEM; 
  } 
 
  
  memset(file,0,SIZE_FCB); 
  //leggo il file control block 
  if ((r=io_space_read(ADDR_FCB+(fd*SIZE_FCB),(natb *)file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    mem_free(file); 
    return -r; 
  }
  flog(LOG_WARN,"LSEEK %d", new_off); 
  
 
   if((new_off=lseek_fcb(file, new_offset, whence)) < 0 ) { 
     perror("LSEEK");  
     mem_free(file); 
     return new_off; 
   }
  flog(LOG_WARN,"LSEEK %d", new_off); 
  

   if ((r=io_space_write(ADDR_FCB+(fd*SIZE_FCB), (const natb *)file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    mem_free(file); 
    return -r; 
   }

  flog(LOG_WARN,"LSEEK %d", new_off); 
    mem_free(file); 
    reset_errno(); 
    return new_off; 
    
}
   

/***REMOVE***************************************************************
 * Cancella un nome dal filesystem.					*
 * Se qualche processo usa il file , questo viene eliminato lo stesso  	*
 * Sta al programmatore evitare che questa situazione di verifichi. 	*
 * La funzione restituisce zero in caso di successo e -1 per un errore,	*
 * nel qual caso il file non viene toccato.				*
 ***********************************************************************/

int c_remove ( const char * path) { 
 
//  dword size=sizeof(FCB); 
  FCB file; 
  int fd=0, r=0; 
  
  if (!tabella) {
    set_errno(EINVAL, "Non e' presente nessun file system"); 
    return ERROR; 
  }
    
  if (!path) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return ERROR;
  }
   
  // uso la funziona di apertura , verifica anche che non sia una directory
  if((fd=c_open(path,O_RDWR)) < 0 ) 
     return ERROR; 
  
  // se tutto è andato bene ho carico il fcb corretto in memria sistema 
  //leggo il file control block 
  if ((r=io_space_read(ADDR_FCB+(fd*SIZE_FCB), (natb *)&file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    return ERROR; 
  }
  //elimino il file 
  if (!delete_entry(&file)) { 
      perror("Delete Entry "); 
      return ERROR; 
  }
   
  if((r=c_close(fd)) < 0 ) 
     return r; 
  
  reset_errno(); 
  return 0; 
  
}

/*RENAME **********************************************************
 * Rinomina un file. I file devono essere nello stesso volume	  *
 * La funzione restituisce zero in caso di successo e -1 per un   *
 * errore, nel qual caso il file non viene toccato. La variabile  *
 * errno viene impostata secondo il giusto codici di errore:	  *
 ******************************************************************/

int c_rename (char * path, char * path2 ) {
  
    FCB *cwd=NULL; 
    FCB *temp=NULL; 
    FCB *source=NULL, *dest=NULL; 
    char new_name[MAX_NAME]; 
    char *chr=NULL; 
    char my_path[MAX_PATH]; 
    int r =0; 
    
    if ( !path || !path2 ) {
      set_errno(EINVAL, "Parametro non corretto (%s-line%d)", __FILE__, __LINE__);
      return ERROR; 
    }
    
    if (!tabella) {
      set_errno(EINVAL, "Non e' presente nessun file system"); 
      return ERROR; 
    }
      
    
    if (!(dest=mem_alloc(sizeof(FCB), 1))) { 
	 set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	 return ERROR; 
    }
    
    if (!(cwd=mem_alloc(sizeof(FCB),1))) {
	set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	mem_free(dest); 
	return ERROR; 
    }
    
    if ( !(source=mem_alloc(sizeof(FCB),1))) { 
	set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	mem_free(cwd); 
	mem_free(dest); 
	return ERROR; 
    }
    
    memset(dest, 0, sizeof(FCB)); 
    memset(cwd,0,sizeof(FCB)); 
    memset(source, 0, sizeof(FCB)); 
    memset(new_name, 0, sizeof(FCB)); 
    memset(my_path, 0, MAX_PATH); 
    memset(new_name,0, MAX_NAME); 
    
    if ((r=io_space_read(ADDR_CWD,(natb*)cwd,sizeof(FCB)))) {
       set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
       goto error_rename; 
    }
        
    if (!open_path(path, cwd, source))
	  goto error_rename; 
    
     
    chr=strrchr(path2, '/'); 
    
    if ( chr == NULL ) { 
	strncpy(new_name, path2, strlen(path2)); 
	memcpy(dest, cwd, sizeof(FCB)); 
    }else if ( chr == path2) { 
	chr++; 
	memcpy(dest, cwd->volume->fcb_root, sizeof(FCB));
	strncpy(new_name,chr, strlen(chr)); 
    }
    else 
    { 
	chr++;
	strncpy(my_path, path2, chr-path2); 
	strncpy(new_name,chr, strlen(chr)); 
 
	if (!open_path(my_path, cwd, dest))
	  goto error_rename; 
    }


    if ( dest->volume != source->volume) {
	set_errno(EINVAL, "I file risiedono in due volumi diversi"); 
	goto error_rename; 
    }

    flog(LOG_WARN, "dest %x source %x nome %s", dest, source, new_name); 

    if (!rename_entry(new_name, dest, source))
	goto error_rename; 

    
    mem_free(source); 
    mem_free(cwd); 
    mem_free(temp); 
    return 0; 
  
error_rename: 
    mem_free(source); 
    mem_free(cwd); 
    mem_free(temp); 
    return ERROR; 
}


/*Funzione che crea una directory*/ 
int c_mkdir (const char * path) {
 
    char  name_file[MAX_NAME];
    char  my_path[MAX_PATH]; 
    char * chr=NULL; 
    FCB  * temp=NULL; 
    FCB  * cwd=NULL, *new=NULL; 
    int r=0; 
    
    
    if (!tabella) {
      set_errno(EINVAL, "Non e' presente nessun file system"); 
      return ERROR; 
    }
    
    if ( !path ) {
      set_errno(EINVAL, "Parametro non corretto (%s-line%d)", __FILE__, __LINE__);
      return ERROR; 
    }
    
    if (!(temp=mem_alloc(sizeof(FCB), 1))) { 
	 set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	 return ERROR; 
    }
    
    if (!(cwd=mem_alloc(sizeof(FCB), 1)) ||!(new=mem_alloc(sizeof(FCB), 1))) { 
	 set_errno(ENOMEM, "Memoria insufficiente(%s-line%d)", __FILE__, __LINE__);
	 return ERROR; 
    }
    
      memset(cwd,0,sizeof(FCB)); 
	
    if ((r=io_space_read(ADDR_CWD,(natb*)cwd,sizeof(FCB)))) {
       set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
       goto error_mkdir; 
    }
    
    
    memset(name_file,0, MAX_NAME); 
    memset(my_path, 0, MAX_PATH); 
    memset(temp, 0, sizeof(FCB));
  
    
    
    
    chr=strrchr(path, '/'); 
    
    if ( chr == NULL ) { 
	strncpy(name_file, path, strlen(path)); 
	memcpy(temp, cwd, sizeof(FCB)); 
    }else if ( chr == path) { 
	chr++; 
	memcpy(temp, cwd->volume->fcb_root, sizeof(FCB));
	strncpy(name_file,chr, strlen(chr)); 
    }
    else 
    { 
	chr++;
	strncpy(my_path, path, chr-path); 
	strncpy(name_file,chr, strlen(chr)); 
 
	if (!open_path(my_path, cwd, temp))
	  goto error_mkdir; 
    }

  flog(LOG_DEBUG, "Creo cartella  %s", name_file); 
  flog(LOG_DEBUG, "PATH %s", my_path); 


  if (!create_directory(name_file,temp, new)) 
     goto error_mkdir; 
  

    mem_free(new); 
    mem_free(cwd); 
    mem_free(temp); 
    return 0; 

error_mkdir:
    mem_free(new); 
    mem_free(cwd); 
    mem_free(temp); 
    return ERROR; 
}



int c_rmdir (const char *path) {

//  dword size=sizeof(FCB); 
  FCB file; 
  int fd=0, r=0; 
  
  if (!tabella) {
    set_errno(EINVAL, "Non e' presente nessun file system"); 
    return ERROR; 
  }
    
  if (!path) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return ERROR;
  }
   
  // uso la funziona di apertura , verifica anche che non sia una directory
  if((fd=c_open(path,O_DIRECTORY)) < 0 ) 
     return ERROR; 
  
  // se tutto è andato bene ho carico il fcb corretto in memria sistema 
  //leggo il file control block 
  if ((r=io_space_read(ADDR_FCB+(fd*SIZE_FCB),(natb*)&file, SIZE_FCB))) { 
    set_errno(r, "Errore io_space_write (%s-line%d)", __FILE__, __LINE__); 
    return ERROR; 
  }
  //elimino il file 
  
  file.mode|=O_WRONLY; 
  
  if (!delete_directory(&file)) { 
      perror("Delete Entry "); 
      return ERROR; 
  }
   
  if((r=c_close(fd)) < 0 ) 
     return r; 
  
  reset_errno(); 
  return 0; 
  
  
  return 0; 
}





/************************************************************************
 *	    int chdir(const char *pathname)				*
 * Cambia la directory di lavoro in pathname.				*
 * La funzione restituisce 0 in caso di successo e -1 per un errore,	* 
 * nel qual caso errno assumera' i valori:				*
 * 	ENOTDIR EINVAL EIO ENOMEM ENOENT				*
 ************************************************************************/

int c_chdir (const char * path) {
  
   FCB * cwd=NULL, *new=NULL;	//FCB della direcotry corrente 
   char volume=0; 	//volume corrente 
   char *pwd=NULL, path_assoluto[MAX_PATH];     // path corrente
   int r=-1;  
   dword size_path=0; 
   
   if (!tabella) {
     set_errno(EINVAL, "Non e' presente nessun file system"); 
     return ERROR; 
   }
   
  if ( !path ) {
    set_errno(EINVAL,"Path errato", __FILE__, __LINE__); 
    return -EINVAL;
  }
  
  size_path=strlen(path); 
  
  if (size_path < 1 || size_path > MAX_PATH) {
    set_errno(EINVAL,"Path errato."); 
    return -EINVAL;
  }
      
   if(!(cwd=mem_alloc(sizeof(FCB), 1))) {
	  set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
	  return -ENOMEM;
   }
   
   if(!(new=mem_alloc(sizeof(FCB), 1))) {
	  set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
	  mem_free(cwd);
	  return -ENOMEM;
   }
   
   if(!(pwd=mem_alloc(MAX_PATH, 1))) {
	set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
	mem_free(cwd); 
	mem_free(new);
	return -ENOMEM;
   }
   
 
  memset(pwd, 0, MAX_PATH);
  memset(path_assoluto, 0, MAX_PATH); 
  memset(cwd, 0, sizeof(FCB));
  
  //PRELEVO LE INFORMAZIONI PRIVATE DEL PROCESSO  
//    // VOLUME 
   if ((r=io_space_read(ADDR_VOL,(byte*)&volume,1))){
      set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
       goto error_chdir; 
  }
//     // PATH
     if ((r=io_space_read(ADDR_PATH, (natb*)pwd,MAX_PATH))){
       set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
       goto error_chdir; 
     }
     //CWD
     if ((r=io_space_read(ADDR_CWD,(natb*)cwd,sizeof(FCB)))) {
       set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
       goto error_chdir; 
    }

   
   if (!open_path(path, cwd, new) ) {
     perror("Errore apertura");
     set_errno(ENOENT, "file %s non esiste", path);
     r=-ENOENT; 
     goto error_chdir; 
   }
   
   
   if (new->type !=ATTR_DIRECTORY ) {
     set_errno(ENOTDIR, "Il nome specificato non e' una directory");
     r=-ENOTDIR; 
     goto error_chdir; 
   }

  
  memset(path_assoluto, 0, MAX_PATH);
  memset(cwd, 0, sizeof(FCB));  
  memcpy(cwd, new, sizeof(FCB));
  
  volume=new->volume->label; 
  create_pathassoluto(path, pwd, path_assoluto, MAX_PATH); 

  

#ifdef DEBUG_FS
   flog(LOG_DEBUG, "CWD :", path ); 
   print_fcb(cwd); 
#endif
     
  
  
  //inserisco il nuovo path in memoria 
  if ((r=io_space_write(ADDR_VOL,(byte*)&volume,1))){
       set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
       goto error_chdir; 
     }
  // PATH
  if ((r=io_space_write(ADDR_PATH, (natb*)path_assoluto,MAX_PATH))){
       set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
       goto error_chdir; 
  }
   //CWD
  if ((r=io_space_write(ADDR_CWD,(natb*)cwd,sizeof(FCB)))) {
      set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
      goto error_chdir; 
  }

  
    mem_free(cwd); 
    mem_free(pwd);
    mem_free(new); 
    reset_errno();
    return 0; 
      
error_chdir: 
    mem_free(new); 
    mem_free(cwd); 
    mem_free(pwd);
    return r; 
} 


/*********************************************************
 *      char *getcwd(char *buffer, size_t size)		 *
 * Legge il pathname della directory di lavoro corrente. *
 * La funzione restituisce il puntatore buffer se riesce,*
 * NULL se fallisce, in quest'ultimo caso viene settata  *
 * la variabile errno.					 *
 *********************************************************/

char *c_getcwd(char *buf, size_t size) {

 char volume[SIZE_VOLUME]; 
 char path[MAX_PATH]; 
 int r=0; 
  
 if (!tabella) {
   set_errno(EINVAL, "Non e' presente nessun file system"); 
   return NULL; 
 }
 
  if ( buf==NULL || size <0) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return NULL;
  }
  
  memset(path,0,MAX_PATH); 
  memset(volume,0,SIZE_VOLUME); 
  
  // prelevo informazioni dall'area privata del processo 
  
  // volume
   if ((r=io_space_read(0,(byte*)volume, SIZE_VOLUME))){
      set_errno(r, "Errore io_space_read  io_space_read (%s-line%d)", __FILE__, __LINE__);
      return NULL; 
   }
   // directory corrente 
   if ((r=io_space_read(ADDR_PATH, (natb*)path, MAX_PATH))){
      set_errno(r, "Errore io_space_read (%s-line%d)", __FILE__, __LINE__);
      return NULL; 
   }
   
   if ( size < strlen(buf)) { 
       set_errno(ERANGE, "Buffer troppo piccolo", path);
      return NULL; 	
    }
   
  memset(buf,0,size); 
  strncat(buf, volume, size); 
  strncat(buf, ":", size-strlen(buf));
  strncat(buf, path, size-strlen(buf));
  
  return buf; 
}
  












