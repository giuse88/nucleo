#include "fat.h"
#include "type.h"
#include "volumi.h"
#include "sistema.h"
#include "direntry.h" 



extern TABELLA_VOLUMI * tabella;



/*FUNZIONI PRIVATE*******************************************************************
 * Funzioni private che mi servono per gestire la tabella FAT, usate dalle funzioni *
 * di interfaccia								    *
 ************************************************************************************/

// funzione che scrive 'value' all'indirizzo addr sulla fat 
BOOL scrivi_fat  (const VOL_PTR v , dword addr, dword value);
// funzione che scrive un valore nella fat in memoria 
BOOL scrivi_mem_fat (const fat_ptr fat, const dword addr, const dword value);  
// funzione che scrive un valore nella fat presente sul disco 
BOOL scrivi_disco_fat (const TABELLA_VOLUMI * tab, dword addr , dword value);
// funzione che legge un entrata della tabella è inserisce il valore su value 
BOOL leggi_fat  (dword* value, dword addr, fat_ptr fat);
// funzione che riporta il numero di un cluster libero
BOOL getFree_fat  (const fat_ptr fat, dword fat_size, dword* addr);
// funzione che individua l'ultimo elemento di una catena 
BOOL get_ultimo_fat (const fat_ptr fat, const dword chain, dword* ultimo); 
// funzione che individua il penultimo elemento di una catena
BOOL get_penUltimo_fat (const fat_ptr fat, const dword chain, dword* ultimo); 
// funzione che carica una tabella fat in memoria 
dword_ptr  load_fat_memory ( TABELLA_VOLUMI * entry );



/**************************************************************************
 * Funzione, richiamata in fase di inizializzazione che Carica in memoria *
 * tutte le fat dei vari volumi.					  *
 **************************************************************************/

BOOL init_fat() {

    TABELLA_VOLUMI * temp=tabella;
    dword_ptr  fat_tmp=NULL;

   while ( temp ) {

        fat_tmp= ( dword_ptr ) load_fat_memory ( temp );

        if ( fat_tmp==NULL ) { 
	    perror("Errore creazione FAT"); 
            return FALSE;  // verifica se devo rimuover qualcosa
	}

        // inizializzo la directory di root facendola puntare al cluster in memoria
        temp->root= ( dword_ptr ) fat_tmp + ( temp->fat_info.first_cluster_directory );
        temp->fat=fat_tmp;
        temp=temp->next;

     }

    return TRUE;

}



/******************************************************************************
 *Carica in memoria la tabella FAT associata al volume individuato da entry   *
 ******************************************************************************/
dword_ptr  load_fat_memory ( TABELLA_VOLUMI * entry ) {

    dword size_fat_byte=0;
    dword size_fat_set=0;
    dword sector_size=0;
    dword sector=0; 
    int cicli=0, r=0;
    byte *fat_tmp=NULL;
    fat_ptr fat=NULL; 
    int i=0;
    	int res=0; 
    
    if ( !entry ) {
      set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
      return FALSE;
    }
    
    sector_size=entry->fat_info.byts_for_sector;
    size_fat_byte=entry->fat_info.size_fat*sector_size;
    size_fat_set=entry->fat_info.size_fat; 
    sector= entry->fat_info.first_set_fat; 
 
    if(!(fat=mem_alloc(size_fat_byte,4))){
      set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
      return FALSE; 
    }
    
    memset(fat, 0, size_fat_byte); 

   

    
    fat_tmp=(byte*)fat;
    cicli=size_fat_set /SECTOR_GROUP;
    r=size_fat_set%SECTOR_GROUP; 
    
#ifdef DEBUG_FS
    flog ( LOG_DEBUG, "Inizio caricamento FAT.... " );
    flog(LOG_DEBUG, "Label            : %c",entry->label);
    flog(LOG_DEBUG, "Partizione       : %d",entry->indice_partizione); 
    flog(LOG_DEBUG, "Devo caricare    : %d byte %d settori", size_fat_byte, size_fat_set );
    flog(LOG_DEBUG, "Fat              : %x", fat); 
    flog(LOG_DEBUG, "Inizio lettura a : %d",  entry->fat_info.first_set_fat); 
    flog(LOG_DEBUG, "Ultimo settore   : %d", entry->fat_info.first_set_fat +  entry->fat_info.size_fat); 
    flog(LOG_DEBUG, "Cicli %d Rimanenza %d", cicli, r);
#endif

    for ( i =0; i< cicli; i++ ) {
	read_part_n ( entry->ata, entry->disco, entry->indice_partizione ,sector, SECTOR_GROUP, ( void * ) fat_tmp );    
 	if ( res < 0 ) { 
	    perror("read_part_n");
	    set_errno(EIO, "Errore read_part_n"); 
	    mem_free(fat); 
	    return NULL; 
	}
	fat_tmp=fat_tmp+sector_size*SECTOR_GROUP; // aggiorno il puntatore 
 	sector+=SECTOR_GROUP;
      }
     

    res=read_part_n ( entry->ata, entry->disco, entry->indice_partizione ,sector, r, ( void * ) fat_tmp );    
    
    if ( res < 0 ) { 
	    perror("read_part_n");
	    set_errno(EIO, "Errore read_part_n"); 
	    mem_free(fat); 
	    return NULL; 
      
    }
    
    fat_tmp=fat_tmp+r*sector_size; // aggiorno il puntatore 
    sector+=r;
    
#ifdef DEBUG_FS
    flog(LOG_DEBUG, "Memoria Occupata : %d", (void*)fat_tmp-(void*)fat);
    flog(LOG_DEBUG, "Caricati settori : %d", sector-entry->fat_info.first_set_fat); 
    flog(LOG_DEBUG, "Fine caricamento  FAT."); 
#endif

       return fat; 
  
}




/****************************************************************************************
 * SCRIVI_FAT :  									*
 * Funzione che scrive un elemento nella tabella FAT. Questa funzione si occupa di	*
 * modificare il valore della fat in memoria mediante la funzione scrivi_mem_fat, modi- *
 * ficare il valore della fat sul disco, assicurandoci la consistenza dei due valori.   *
 * Senza la gestione di questa funzione in un segmento di codice protetto dal semaforo  *
 * della fat si potrebbero verificare delle perdite di dati.  				*
 * 											*
 * VOLUME  : volume sul quale lavorare							* 
 * ADDR    : indirizzo della fat nel quale inserire il valore 				*
 * VALUE   : valore da inserire 							*
 * 											*
 * Return, riporta TRUE se tutto è avvenuto con successo, altrimenti rende FALSE	*
  e setta la variabile errno opportunamente.						*
 ****************************************************************************************/

BOOL scrivi_fat  (VOL_PTR part , dword addr, dword value) {
  

    dword backup=0, read=0; 
    
    
    if ( !part ) {
      set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
      return FALSE;
    }
    //verifico se stiamo tentando una scrittura su un cluster errato 
    
    if(!leggi_fat(part->fat,addr,&read)) {
	 set_errno(EIO,"Lettura Fallita");
	 return FALSE; 
    }
    
    if(read == BAD_CLUSTER_32) { 
	set_errno(EIO,"Si sta provando a scrivere su un cluster corrotto"); 
	return FALSE; 
    }
   
   // le letture sono fatte in memoria
    
    
      //leggo valore di backup per assicurare la consistenza dei dati tra disco è memoria
      if(!leggi_fat(part->fat, addr, &backup)) {
	 set_errno(1,"Lettura Fallita");
	 return FALSE; 
      }
      //scrivo in memoria 
      if(!scrivi_mem_fat(part->fat, addr, value))
	  return FALSE; 
	  
      // provo a scrivere sul disco se non ci riesco provo a ripristinare il vecchio stato
      if(!scrivi_disco_fat(part, addr, value)) {
	  if(!scrivi_mem_fat(part->fat, addr, backup))
	    set_errno(EIO,"Impossibile ripristinare il corretto valore, possibili malfunzionamenti");
	  return FALSE; 
      } 
    
    return TRUE; 
} 

/****************************************************************************************
 * SCRIVI_MEM_FAT : 									*
 * Funzione che scrive un valore nella fat in memoria, facendo le opportune conversioni * 
 * perché le entrate sono a 28 bit.  							*
 ****************************************************************************************/

inline BOOL scrivi_mem_fat (const fat_ptr fat, const dword addr , const dword value) {
  
   *(fat+addr)= (value & MASK_32);
  
  return TRUE; 
}

/************************************************************************
 * SCRIVI_DISCO_FAT :							*
 * Funzione che scrive un valore nella fat presente sul disco, 		*
 * da notare che questa funzione deve essere eseguita in mutua 		*
 * esclusione.								*
 * 									*
 * TAB   : puntatore ad un volume indispensabile per le info sulla FAT 	*
 * ADDR  : indirizzo cluster da scrivere nella FAT			*
 * VALUE : valore da inserire nella cluster				*
 * 									*
 * Ritorna TRUE se ha avuto successo altrimenti FALSE			*
 ************************************************************************/

BOOL scrivi_disco_fat (const TABELLA_VOLUMI * tab, dword addr , dword value) {
  
  dword settore[SECT_SIZE/SIZE_ENTRY_FAT_32]; 
  dword n_set=FATSectorNum_32 (addr, tab->fat_info.first_set_fat);
  dword offset=FATOffeset_32 (addr)/SIZE_ENTRY_FAT_32;

  if(!(tab)) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  //leggo il settore  
  read_part_n (tab->ata, tab->disco, tab->indice_partizione, n_set, 1, ( void * ) settore );
  //modifico il settore 
  settore[offset]=value & MASK_32;
  //scrivo il settore modificato 
  write_part_n ( tab->ata, tab->disco, tab->indice_partizione, n_set, 1, ( void * ) settore );
  
  return TRUE; 
  
}

/***************************************************************************************
 * LEGGI_FAT : 									       *
 * Funzione che legge un valore dalla fat in memoria, facendo le opportune conversioni *
 ***************************************************************************************/

BOOL leggi_fat  (fat_ptr fat , dword addr, dword * value) { 
  
  if(!(fat)) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
   
  *value=*(fat+ (addr & MASK_32)) & MASK_32;
 
  return TRUE; 
  
}



/****************************************************************************************
 *GETFREE_FAT :										*
 * Funzione che ricerca un cluster vuoto, questa  funzione deve essere gestita in mutua *	
 * esclusione, questo le viene garantito dalla funzione append e create, le uniche 	*
 * funzioni che hanno la possibilità di invocarla .					*
 * 											*
 * FAT : puntatore alla fat in memoria 							*
 * SIZE : grandezza fat byte								*
 * ADDR : numero del cluster vuoto							*
 *											*
 * Ritorna Falso se non è stato possibile trovare un cluster libero e addr non ha senso.*
 * OTTIMIZZAZIONE : si potrebbe tenere un puntatore all'ultimo cluster trovato libero 	*
 ****************************************************************************************/  


BOOL  getFree_fat  (const fat_ptr fat , dword fat_size, dword * addr) {
  
    int i=0; 
    fat_ptr tmp_fat=fat; 
    
    
    if ( !fat ||  !fat_size || !addr  ) {
      set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
      return FALSE;
    }
    
    for ( i=0 ; i< fat_size; i++, tmp_fat++) { 
      
	   if(isFree_fat(*tmp_fat)) {
	      *addr=i; 
	      return TRUE; 
	   } 
    }
    
    *addr=NULL; 
    set_errno(1,"Non ci sono entrate libere nella fat"); 
    return FALSE; 
}

/********************************************************************************
 * APPEND_FAT :									*
 * Funzione che aggiunge un cluster in ultima posizione alla catena di cluster	*
 * passata come argomento. Questa funzione ci garantisce che le operazioni di	*
 * scrittura avvengano in mutua esclusione mediante 				*
 * l'utilizzo di semafori.							*
 * 										*
 * VOLUME : identifica il volume nel quale stiamo lavorando 			*
 * CHAIN  : identifica la catena sulla quale dobbiamo fare l'inserimento 	*
 *	    se zero la crea  							*
 * ADDR   : indirizzo nel quale viene inserita la posizione relativa alla fat 	*
 *	    a senso solo se è la funzione a successo.				*	
 *										*
 * Ritorna TRUE se l'inserimento ha avuto successo altrimenti falso e setta il	*
 * valore della variabile errno opportunamente.					*
 ********************************************************************************/ 

BOOL append_fat (VOL_PTR tab , const dword chain, dword * addr) {
  
  fat_ptr fat=NULL; 
  dword cluster_ultimo=0, cluster_new=0;
  
  if(!(tab)) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  if(chain <2 ) { 
     set_errno(EINVAL,"Cluster riservato");
    return FALSE; 
  }
  
  fat=tab->fat; 
  
  sem_wait(tab->sem_fat); 
  
    if(!get_ultimo_fat(fat, chain, &cluster_ultimo)) {
      sem_signal(tab->sem_fat); 
      return FALSE; 
    }
    
  if(!getFree_fat(fat, tab->size_fat,&cluster_new)) {
     sem_signal(tab->sem_fat);
    return FALSE; 
  }
  
  //devo scrivere nell'ultimo l'indirizzo del nuovo cluster
    
    if(!scrivi_fat(tab, cluster_ultimo, cluster_new))  {
      sem_signal(tab->sem_fat);
      return FALSE; 
    }
    
  if(!scrivi_fat(tab, cluster_new, EOC_32)) {
    // se fallisce provo a ripristinare la situazione precedente
    if(!scrivi_fat(tab, cluster_ultimo,  EOC_32)) 
      set_errno(EIO,"Errore irrecuperabile, possibile perdita di dati");    
      sem_signal(tab->sem_fat); 
     return FALSE; 
    }
  
  // ... ----->Ultimo---->new(EOC); 
  // finito inserimento 

  sem_signal(tab->sem_fat); 
  
  if(addr)
    *addr=cluster_new; 
  
  return TRUE; 
  
}

/********************************************************************************
 *GET_ULTIMO_FAT 								*
 * Funzione che analizza una catena di cluster e riporta  il numero dell'ultimo *
 * cluster presente, facendo le opportune verifiche sulla consistenza stessa 	*
 * della catena.								*
 * FAT : Puntatore alla fat in memoria 						*
 * CHAIN : numero di entrata del primo elemento di una catena 			*
 * ULTIMO : indirizzo nel quale verrà memorizzato il numero dell'ultimo cluster *
 ********************************************************************************/

BOOL get_ultimo_fat (const fat_ptr fat, const dword chain,  dword * ultimo){
  

 fat_ptr chain_tmp=fat+chain; //punto al primo elemento in memoria
 dword count=chain;
 
  if(chain < 2) {
    set_errno(1,"Cluster riservati");
    return FALSE; 
  } 
 
 if(!(fat)) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
 
 while (TRUE) { 
   
   if(*chain_tmp==FREE_32) { 
    set_errno(1,"Presente cluster libero nella lista"); 
    *ultimo=0;
    return FALSE; 
   }
  
  if(*chain_tmp==BAD_CLUSTER_32) { 
    set_errno(1,"Presente cluster corrotto nella lista"); 
    *ultimo=0; 
    return FALSE; 
  }
   
  if(*chain_tmp>=EOC_32) {
    *ultimo=count;
    return TRUE; 
  }
  
  //aggiorno i puntatori 
  count=*chain_tmp; // aggiorno il contenuto 
  chain_tmp=(fat+*chain_tmp); // punto alla casella successiva
   
 }
 
 return FALSE;
}

/********************************************************************************
 *GET_PENULTIMO_FAT 								*
 * Funzione che analizza una catena di cluster e riporta  il numero del penulti-*
 * mo cluster presente, facendo le opportune verifiche sulla consistenza stessa *
 * della catena.								*
 * FAT    : Puntatore alla fat in memoria 					*
 * CHAIN  : numero di entrata del primo elemento di una catena 			*
 * ULTIMO : indirizzo nel quale verrà memorizzato il numero dell'ultimo cluster *
 ********************************************************************************/

BOOL get_penUltimo_fat (const fat_ptr fat, const dword chain,  dword * pen_ultimo){
  
 fat_ptr chain_tmp=fat+chain; //punto al primo elemento in memoria
 dword count=chain;
 dword ultimo=chain; 
 *pen_ultimo=0; 
 
  if(chain < 2) {
    set_errno(1,"Cluster riservati");
    return FALSE; 
  }  
 
 if(!(fat)) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
 
 
 while (TRUE) { 
   
   if(*chain_tmp==FREE_32) { 
    set_errno(EINVAL,"Presente cluster libero nella lista"); 
    *pen_ultimo=0;
    return FALSE; 
   }
  
  if(*chain_tmp==BAD_CLUSTER_32) { 
    set_errno(EINVAL,"Presente cluster corrotto nella lista"); 
    *pen_ultimo=0; 
    return FALSE; 
  }
   
   
  if(*chain_tmp>=EOC_32) {
    *pen_ultimo=count;
    return TRUE; 
  }
  
  //aggiorno i puntatori 
  count=ultimo;
  ultimo=*chain_tmp; 
  chain_tmp=fat+*chain_tmp;
   
 }
 
 return FALSE;
}




/********************************************************
 *  DELETE_FAT 						*
 * Funzione che elimina l'ultimo cluster della catena. 	*
 * La gestione della fat avviene in mutua esclusione. 	*
 * 							*
 *  TAB   : puntatore al volume 			*
 *  CHAIN : cluster che individua la catena		*
 *							*
 * Ritorna TRUE se a avvuto successo, altrimenti riporta*
 * False e setta la variabile errno in modo opportuno.  *
 ********************************************************/ 

BOOL delete_fat (VOL_PTR tab, const dword chain) { 
  

  fat_ptr fat=NULL; 
  dword cluster_ultimo=0,cluster_penultimo=0; 
  dword backup=0; 
  
  if(!(tab)) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  if(chain < 2) {
    set_errno(1,"Cluster riservati");
    return FALSE; 
  }
    
    
  fat=tab->fat; 
  leggi_fat(fat,cluster_penultimo,&backup);
  
  
  sem_wait(tab->sem_fat); 
  
  if(!get_ultimo_fat(fat, chain, &cluster_ultimo)) {
    sem_signal(tab->sem_fat);
    return FALSE; 
  }
  
  if(!get_penUltimo_fat(fat, chain, &cluster_penultimo)) {
    sem_signal(tab->sem_fat);
    return FALSE;   
  }
  
  if(!scrivi_fat(tab, cluster_penultimo, EOC_32)) 
    return FALSE; 

  
  if(!scrivi_fat(tab, cluster_ultimo, FREE_32)) {
    if(!scrivi_fat(tab, cluster_penultimo,  backup)) 
      set_errno(1,"Errore irrecuperabile, possibile perdita di dati");    
    sem_signal(tab->sem_fat);
    return FALSE; 
  }
  
  sem_signal(tab->sem_fat); 
  
  return TRUE;
 
}

/********************************************************
 *  DELETE_FAT 						*
 * Funzione che elimina l'ultimo cluster della catena. 	*
 * La gestione della fat avviene in mutua esclusione. 	*
 * 							*
 *  TAB   : puntatore al volume 			*
 *  CHAIN : cluster che individua la catena		*
 *							*
 * Ritorna TRUE se a avvuto successo, altrimenti riporta*
 * False e setta la variabile errno in modo opportuno.  *
 ********************************************************/ 

BOOL delete_all_fat (const VOL_PTR volume, const dword chain) { 
  
  dword t=0, prec=0; 
  
  
  if(!(volume) || chain <= 2 ) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  sem_wait(volume->sem_fat); 
  
  t=chain; 
  prec=chain; 
  
    do {
	prec=t; 
	if(!(t=get_next_fat(volume->fat, t))) {
	  perror("Errore lettura ");
	  sem_signal(volume->sem_fat); 
	  return FALSE; 
	}
	
	if (!scrivi_fat(volume,prec, FREE_32)) {
	  perror("Errore scrittura");
	  sem_signal(volume->sem_fat); 
	  return FALSE;  
	}
    } while(!isLast_fat(t)); 
    
   sem_signal(volume->sem_fat); 
  
    
  reset_errno(); 
  return TRUE; 
  
}

/*************************************************************
 * Funzione che elimina una catena riscrivendo tutta la fat  *
 * Non viene utilizzata. 				     *
 *************************************************************/

BOOL delete_all_rw_fat (const VOL_PTR volume, const dword chain) { 
  
  dword t=0, prec=0; 
  
  
  if(!(volume) || chain <= 2 ) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  sem_wait(volume->sem_fat); 
  
  t=chain; 
  prec=chain; 
  
    do {
	prec=t; 
	if(!(t=get_next_fat(volume->fat, t))) {
	  perror("Errore lettura ");
	  sem_signal(volume->sem_fat); 
	  return FALSE; 
	}
	
	if (!scrivi_fat(volume,prec, FREE_32)) {
	  perror("Errore scrittura");
	  sem_signal(volume->sem_fat); 
	  return FALSE;  
	}
    } while(!isLast_fat(t)); 
    
   sem_signal(volume->sem_fat); 
  
    
  reset_errno(); 
  return TRUE; 
  
}



/*******************************************************************
 * Funzione che server per scorrere una catena fat. 		   *
 * Ritorna il valore del cluster successivo, facendo gli opportuni *
 * controlli. 							   *
 * Se ritorna zero si è verificato un errore.			   *
 *******************************************************************/

dword get_next_fat(const fat_ptr fat, const dword cluster) {
  
  dword value=*(fat+cluster)&MASK_32; 
 
  
  if ( fat == NULL || cluster < 2 ) { 
     set_errno(EINVAL,"Parametri errati (%s-line%d)", __FILE__, __LINE__); 
     return FALSE;  
  }
  
  if ( isLast_fat(cluster)) {
      set_errno(EIO,"Catena terminata"); 
      return FALSE; 
  }
 
  if ( isBad_fat(cluster)) {
     set_errno(EIO,"Elemento corrotto");
     return FALSE; 
  }
  
  if ( isFree_fat(cluster)) {
     set_errno(EIO,"Elemento vuoto");
     return FALSE; 
  }
  
  
  if (isFree_fat(value)) {  
      set_errno(1,"Elemento Vuoto");
      return FALSE; 
  } 
   
  
  return *(fat+cluster)&MASK_32;
  
}


/********************************************************************************
 * CREATE_FAT :									*
 * Funzione che crea una nuova catena nella tabella FAT.			*
 * Questa funzione ci garantisce che le operazioni di scrittura avvengano in    *
 * mutua esclusione mediante  l'utilizzo di semafori.				*
 * 										*
 * VOLUME : identifica il volume nel quale stiamo lavorando 			*
 * CHAIN  : identifica la catena sulla quale dobbiamo fare l'inserimento 	*
 *	    se zero la crea  							*
 * ADDR   : indirizzo nel quale viene inserita la posizione relativa alla fat 	*
 *	    a senso solo se è la funzione a successo.				*	
 *										*
 * Ritorna TRUE se l'inserimento ha avuto successo altrimenti falso e setta il	*
 * valore della variabile errno opportunamente.					*
 ********************************************************************************/ 

BOOL create_fat (const VOL_PTR tab, dword* chain) { 
  
  

  fat_ptr fat=NULL; 
  dword cluster_new=0;
  dword size_fat=0; 
 
  if(!(tab)) {
    set_errno(EINVAL,"Volume errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  
  fat=tab->fat; // puntatore alla fat in memoria
  size_fat=tab->size_fat;	
  *chain=0; 
  
  sem_wait(tab->sem_fat); 
  
  if(!getFree_fat(fat,size_fat ,&cluster_new)) {
     sem_signal(tab->sem_fat); 
    return FALSE; 
  } 
  
  if(!scrivi_fat(tab, cluster_new, EOC_32)) {
    sem_signal(tab->sem_fat); 
    return FALSE; 
  }
  
  sem_signal(tab->sem_fat); 
  
  *chain=cluster_new; 
  
  return TRUE; 
  
}



/***********************************************************
 * Funzione di debug, 					   *	
 * Stampa la catena CHAIN				   *
 ***********************************************************/

void print_chain( const VOL_PTR tab, dword chain ) {

  
  if ( !tab ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return;
  }
  
    dword t=chain; 
    do {
	flog(LOG_DEBUG, "Elemento : %d", t);
	if(!(t=get_next_fat(tab->fat, t))) {
	  perror("Errore lettura ");
	  break; 
	}
    } while(!isLast_fat(t)); 
    
}


/****************************************************************************
 * Funzione di debug, Stampa tutte le informazioni sulla FAT dei vari volumi*
 ****************************************************************************/

void info_fat() { 
   
    VOL_PTR tab=tabella; 
  
   flog(LOG_DEBUG,"********************* INFO FAT ***************************"); 
   
   while (tab) {
	  flog(LOG_DEBUG,"\tVOLUME : %c", tab->label); 
	  flog(LOG_DEBUG,"\tADDR   : %x", tab->fat);
	  flog(LOG_DEBUG,"\tSIZE   : %d", tab->size_fat);
	  tab=tab->next; 
   }
   
   flog(LOG_DEBUG,"**********************************************************"); 
}

/*****************************************************************************
 * Funzione che elimina tutte le tabelle dei vari volumi presenti in memoria.* 
 * Tipicamente usata in caso di errore					     *
 *****************************************************************************/ 

void delete_memory_fat() {
  
  VOL_PTR temp =tabella; 
  
  while ( temp) { 
      if (temp->fat) 
	mem_free(temp->fat); 
      temp=temp->next; 
  }
  
}


