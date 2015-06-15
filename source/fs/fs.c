#include "type.h"
#include "sistema.h" 
#include "volumi.h"
#include "system_call.h" 
#include "data.h"
#include "direntry.h" 
#include "fs.h" 
#include "errno.h" 
#include "fat.h"


/*STRUTTURE DATI GLOBALI */
TABELLA_VOLUMI * tabella=NULL; 

// funzione che inizializza la struttura globale presente nel modulo sistema
// inizializza inserendo il fcb della root direcotry 

BOOL init_fs_io_state() { 
    
  
    byte *buf=NULL; 
    dword size = SIZE_VOLUME + MAX_PATH +sizeof(FCB)+(sizeof(FCB) *MAX_FILE); // MAX_FILE + CWD  
    VOL_PTR vol=NULL; 
    dword r=0; 
    
    if (!tabella) { 
	set_errno(EINVAL, "Non presente tabella volumi (%s-line%d)", __FILE__, __LINE__);; 
	return FALSE; 
    }
    
    if (!(vol=get_volume('C'))) { 
        set_errno(EIO, "Non presente volume C (%s-line%d)", __FILE__, __LINE__); 
	return FALSE; 
    }
    
    if(!(buf=mem_alloc(size,1))){
      set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
      return FALSE; 
    }
    
    memset(buf, 0 , size); 
    
    // inserisco la label del volume 
    strncpy( (char*)(buf+ADDR_VOL), (const char*)(&vol->label), 1);
    //inserisco il path
    strncpy( (char*)(buf+ADDR_PATH), "/", 1); 
    // INSERISCO LA CURENT DIRECOTRY 
    memcpy((buf+ADDR_CWD), (vol->fcb_root), sizeof(FCB));  
    
    //primitiva di sistema 
    if((r=io_space_init(buf,size))) {
      set_errno(r, "Io space init errore : %d", r); 
      return FALSE; 
    }
    
    return TRUE; 
    
}

/*FUNZIONE********************************************
 * Funzione che formata il FCB per che rappresenta la * 
 * directory di root				     *
 *****************************************************/ 

void format_root_fcb( FCB * fcb, VOL_PTR volume) { 
     
    dword temp=0; 
    dword size_cluster=0; 
    int i=1; 
    
    if ( !fcb || !volume )
	return;  
    
    memset((void*)fcb, 0, SIZE_FCB); 
    
    fcb->cluster=ROOT_CLUSTER; 
    fcb->volume=volume; 
    fcb->type=ATTR_DIRECTORY; 	// directory 
    fcb->mode=0x10000; 		// directory
    fcb->cluster_father=ROOT_CLUSTER; 
    fcb->offset_father=0; 
    fcb->n_entry=0; 
    fcb->pos_corr=0;   		// posizione corrente   
    
    temp=get_next_fat(volume->fat, ROOT_CLUSTER); 
    size_cluster=volume->fat_info.byts_for_sector*volume->fat_info.sectors_for_cluster;
    
 
    //calcola grandezza tabella 
    while ( temp < BAD_CLUSTER_32) {
	 i++; 
	 temp=get_next_fat(volume->fat, temp);
	 if(!temp)
	      break; 
    }

    fcb->size=i*size_cluster; 
        // grandezza file
}

/***FUNZIONE***************************************************************************
 * Funzione che inizializza i fcb che rappresentano la root directory dei vari volumi.* 
 * L'inizializzazione deve avvenire dopo l'inizializzazione della tabella FAT, 	      *
 * perchè si poterebbe avere la necessità di scorrerla per determinare la grandezza   *
 *  della cartella.								      *
 **************************************************************************************/


BOOL init_root_fcb() {
  
  TABELLA_VOLUMI * temp=NULL; 
  
  if ( !tabella ) { 
      set_errno(EINVAL, "Non presente tabella dei volumi (%s-line%d)", __FILE__, __LINE__);
      return FALSE; 
  }
  
  temp=tabella; 
      
   while (temp) { 
      format_root_fcb(temp->fcb_root, temp); 
#ifdef DEBUG_FS
      flog(LOG_DEBUG, " FCB root volume    : %c ", temp->label);
      print_fcb((temp->fcb_root));
#endif   
      temp=temp->next;
   } 
   
   return TRUE; 
}

/* Pubblica FUNZIONE ***********************************************************
 * Questa funzione ha lo scopo di inizializzare il FS mediante delle chiamate  *
 * ai vari moduli di cui e' composto il fs. Se attiva la macro DEBUG_FS esegue *
 * invoca delle funzioni di debug.					       *	
 * Puo' fallire quando fallisce la chiamata di un sotto modulo. 	       *	
 *******************************************************************************/

BOOL  fs_init(void){ 
  
  BOOL r=TRUE; 
  
 flog(LOG_INFO, "Inizialiazzazione FILE SYSTEM "); 

/*********** VOLUMI  *********************************/
 
  // chiamata per la creazione della tabella dei volumi 
  if(!(tabella=crea_tabella_volumi())) {
      perror("Creazione tabella volumi fallita"); 
      r=FALSE; 
      delete_tabella(); 	// elimino la tabella 
      goto end_fs; 
  }

#ifdef DEBUG_FS
  stampa_tabella_volumi(); 
#else
  flog(LOG_INFO, "Tabella Volumi creata correttamente"); 
#endif

/*************** FAT **********************************/
 
  if(!(init_fat())){
      perror("Caricamento FAT fallito"); 
      r=FALSE; 
      delete_memory_fat(); 
      delete_tabella(); 	// elimino la tabella 
      goto end_fs; 
  }else {
      flog(LOG_INFO, "Fat caricata correttamente"); 
  }

#ifdef DEBUG_FS 
 info_fat(); 
#endif

/******************** ROOT FCB******************************/ 
  
  if (!init_root_fcb()) { 
      perror("init_root_fcb");
      delete_memory_fat(); 
      delete_tabella();
      goto end_fs; 
  } else {
        flog(LOG_INFO, "Root Fcb Inizializzata correttamente"); 
  }
 
/******************IOSPACE ************************/
if(init_fs_io_state()) 
     flog(LOG_INFO,"Inizializzata area IOSPACE");
else 
    perror("Inizializzazione IOSPACE fallita "); 
  

end_fs :
  flog(LOG_INFO,"Fine inizializzazione FILE SYSTEM"); 
  return r; 
}






