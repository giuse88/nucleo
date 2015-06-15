

#include "fat.h"
#include "type.h"
#include "volumi.h"
#include "sistema.h"
#include "direntry.h" 


extern TABELLA_VOLUMI * tabella;
dword_ptr load_fat_memmory ( TABELLA_VOLUMI* );
/*void insert_part_name ( const word * name, char * buf, int );
//FCB *  add_short_dir ( SHORT_ENTRY_FAT  ** dir );
//FCB *  add_long_dir ( LONG_ENTRY_FAT  ** dir );
void print_fcb ( const FCB* );
dword free_dir_entry ( dword_ptr );
dword get_free_entry ( dword  n, dword_ptr cluster, dword size );
dword get_free_cluster ( byte label, dword ultimo );
//FCB * entry_on_cluster ( dword indice_free, dword_ptr cluster, const SHORT_ENTRY_FAT * short_entry );
byte CHkSum ( char * name );
void print_cluster ( dword_ptr cluster , int n );
//BOOL create_short_entry ( SHORT_ENTRY_FAT * short_entry, const char *name );
inline unsigned char toUpperChar ( unsigned char c );
//BOOL createShortName ( const char * long_name,  char *short_name );
/* Funzione che riporta il tipo di file system
   Seguendo le specifiche MICROSOFT
 */

byte getFileSystemType ( dword DataSizeCluster ) {

    if ( DataSizeCluster < 4085 )
        return FAT12;
    else if ( DataSizeCluster < 65525 )
        return FAT16;
    else
        return FAT32;
}


/* Verifichiamo se è presente il MAGIC Number nel Settore */
BOOL check_boot_sector ( MBR * mbr ) {

    if ( mbr->magic_number[0] == 0x55 && mbr->magic_number[1]==0xAA )
        return TRUE;
    else
        return FALSE;
}


/*
 * FUNzione che Carica in memoria tutte le fat dei vari volumi
 */

BOOL init_fat() {

    TABELLA_VOLUMI * temp=tabella;
    dword_ptr  fat_tmp=NULL;

    while ( temp ) {

        fat_tmp= ( dword_ptr ) load_fat_memmory ( temp );

        if ( fat_tmp==NULL )
            return FALSE;  // verifica se devo deallocare qualcosa

        // inizializzo la directory di root facendola puntare al cluster in memoria
        temp->root= ( int * ) fat_tmp + ( temp->fat_info.first_cluster_directory );
        temp->fat=fat_tmp;
        temp=temp->next;

    }

    return TRUE;

}



// carica la fat in memoria

dword_ptr  load_fat_memmory ( TABELLA_VOLUMI * entry ) {

    lword size_fat_byte=entry->fat_info.size_fat*SECT_SIZE;
    dword size_fat_set=entry->fat_info.size_fat;
    int cicli=0, r=0;
    char *  fat =NULL, *fat_tmp=NULL;
    int i=0;

    flog ( LOG_INFO, "Inizio caricamento FAT " );
    flog ( LOG_INFO, "SIZE %d", size_fat_byte );

    fat= ( char * ) mem_alloc ( size_fat_byte, 2 );

    if ( fat == NULL )
        flog ( LOG_WARN, "Errore Allocazione FAT" );

    flog ( LOG_WARN, "Devo caricare %d", entry->fat_info.size_fat );
    flog ( LOG_WARN, "cicli %d",  cicli=size_fat_set /32 );
    flog ( LOG_WARN, "Rimanenza %d", size_fat_set%32 );
    fat_tmp=fat;

    for ( i=0; i <cicli; i++, ( ( char * ) fat_tmp +32*512 ) )
        read_part_n ( entry-> ata, entry-> disco, entry->indice_partizione, entry->fat_info.first_set_fat, 32, ( void * ) fat_tmp );

    if ( r != 0 )
        read_part_n ( entry-> ata, entry-> disco, entry->indice_partizione, entry->fat_info.first_set_fat, r, ( void * ) fat_tmp );

    flog ( LOG_INFO, "Fine caricamento FAT " );

    return ( dword_ptr ) fat;

}


// funzione che mi rende il prossimo puntatore faccendo gli oportuni controlli

// funzione che serve per scorrere la lista dei chain

dword getNext ( dword_ptr fat , dword cluster ) {

    dword_ptr ptr = fat + cluster ;
    dword entry;
    FCB  *  fcb=NULL;

    if ( fat == NULL || cluster < 2 )
        return NULL;

    entry= * ( ( dword_ptr ) ( ptr ) ) & MASK_32 ;

    if ( entry != EOC_32 )
        return entry;
    else
        return 0;

}






/*GESTIONE FAT §§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§*/

 /* INTERFACCIA  che mi permette di gestire la tabella FAT			   *
 * Funzioni elementari che lavorano sulla tabella FAT				
 * per convenzione tutte le funzioni che riguardano a fat terminano con fat*
 ***********************************************************************************/

// aggiunge un cluster alla lista chain
// se chain è zero la crea 
BOOL append_fat (const byte volume , const dword chain, dword *addr); 
// elimina l'ultimo cluster dalla lista chain 
BOOL delete_fat (const byte volume, const dword chain); 
// elimina tutta una lista chain 
BOOL delete_all_fat (const byte volume, const dword chain);
void test_fat (byte label);
/*FUNZIONI PRIVATE*******************************************************************
 * Funzioni private che mi servono per gestire la tabella FAT, usate dalle funzioni *
 * di interfaccia								     *
/************************************************************************************/

// funzione che scrive 'value' all'indirizzo addr sulla fat 
BOOL scrivi_fat  (byte volume , dword addr, dword value);
// funzione che scrive un valore nella fat in memoria 
BOOL scrivi_mem_fat (const fat_ptr fat, const dword addr, const dword value);  
// funzione che scrive un valore nella fat presente sul disco 
BOOL scrivi_disco_fat (const TABELLA_VOLUMI * tab, dword addr , dword value);
// funzione che legge un entrata della tabella è inserisce il valore su value 
BOOL leggi_fat  (dword* value, dword addr, fat_ptr fat);
// funzione che dato un cluster riporta il successivo della catena (si potrebbe fare come macro)
//dword  getNext_fat (fat_ptr chain);
// funzione che riporta il numero di un cluster libero
BOOL getFree_fat  (const fat_ptr fat, dword fat_size, dword* addr);

BOOL get_ultimo_fat (const fat_ptr fat, const dword chain, dword* ultimo); 
BOOL get_penUltimo_fat (const fat_ptr fat, const dword chain, dword* ultimo); 


dword get_next_fat(fat_ptr fat, dword cluster); 

BOOL create_fat (const byte volume, dword* chain); 

#define isFree_fat(N) ((N&MASK_32)==FREE_32)
#define isLast_fat(N) ((N&MASK_32)>=EOC_32)
/*§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§*/



/* **********************************************************************************************
 * SCRIVI_FAT :  
 * Funzione che scrive un elemento nella tabella FAT. Questa funzione si occupa di modificare *
 * il valore della fat in memoria mediante la funzione scrivi_mem_fat , modificare il valore *
 * della fat sul disco, assicurandoci la consistenza dei due valori e la mutua esclusione *
 * con altre chiamate alla scrivi o chiamate alla getFree mediante un semaforo di mutua  *
 * esclusione. Senza questi accorgimenti si potrebbero verificare delle perdite di dati  *
 * 											*
 * VOLUME : volume sul quale lavorare							* 
 * ADDR   : indirizzo della fat nel quale inserire il valore 				*
 * VALUE : valore da inserire 								*
 * 											*
 * Return, riporta true se tutto è avvenuto con successo , altrimenti rende false e setta* 
 * la variabile errno opportunamente.							*
 ************************************************************************************************/

BOOL scrivi_fat  (byte volume , dword addr, dword value) {
  
    TABELLA_VOLUMI * part=NULL; 
    dword backup=0, read=0; 
    
    if(!(part=get_volume(volume))){ 
	set_errno(1,"Volume errrato"); 
	return FALSE;
    } 
    
    //verifico se stiamo tentando una scrittura su un cluster errato 
    
    if(!leggi_fat(part->fat,addr,&read)) {
	 set_errno(1,"Lettura Fallita");
	 return FALSE; 
    }
    
    if(read == BAD_CLUSTER_32) { 
	set_errno(1,"Si sta provando a scrivere su un cluster corrotto"); 
	return FALSE; 
    }
    
    
  //  sem_wait(part->semaphore);
    
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
	    set_errno(1,"Impossibile ripristinare il corretto valore, possibili malfunzionamenti");
	  return FALSE; 
      } 
  
  //  sem_signal(part->semaphore); 
  
    return TRUE; 
} 

/*SCRIVI_MEM_FAT : 
 * funzione che scrive un valore nella fat in memoria,  facendo le opportune conversioni, 
 * perché le entrate sono a 28 bit.  
 */

inline BOOL scrivi_mem_fat (const fat_ptr fat, const dword addr , const dword value) {
  
   *(fat+addr)= (value & MASK_32);
  
  return TRUE; 
}

/* SCRIVI_DISCO_FAT :
 * funzione che scrive un valore nella fat presente sul disco, 
 * da notare che questa funzione deve essere eseguita in mutua 
 * esclusione, questo mi viene garantito dalla funzione 
 * scrivi che è l'unica che invoca questa funzione 
 * 
 * TAB   : puntatore ad un volume indispensabile per le info sulla FAT 
 * ADDR  : indirizzo cluster da scrivere nella FAT
 * VALUE : valore da inserire nella cluster
 * 
*/

BOOL scrivi_disco_fat (const TABELLA_VOLUMI * tab, dword addr , dword value) {
  
  dword settore[SECT_SIZE/SIZE_ENTRY_FAT_32]; 
  
  
  dword n_set=FATSectorNum_32 (addr, tab->fat_info.first_set_fat);
  dword offset=FATOffeset_32 (addr)/SIZE_ENTRY_FAT_32;

  //leggo il settore  
  read_part_n (tab->ata, tab->disco, tab->indice_partizione, n_set, 1, ( void * ) settore );
  //modifico il settore 
  settore[offset]=value & MASK_32;
  //scrivo il settore modificato 
  write_part_n ( tab->ata, tab->disco, tab->indice_partizione, n_set, 1, ( void * ) settore );
  
  return TRUE; 
  
}

/* LEGGI_FAT : 
 * Funzione che legge un valore dalla fat in memoria, facendo le opportune conversioni 
 */

inline BOOL leggi_fat  (fat_ptr fat , dword addr, dword * value) { 
    //devo verificare che fat sia un buon cluster 
  *value=*(fat+ (addr & MASK_32)) & MASK_32;
 
  return TRUE; 
  
}



/*GETFREE_FAT :
 * funzione che ricerca un cluster vuoto, questa  funzione deve essere gestita in mutua esclusione, 
 * questo le viene garantito dalla funzione append, l'unica funzione che ha la possibilità di 
 * invocarla .
 * 
 * FAT : puntatore alla fat in memoria 
 * SIZE : grandezza fat 
 * ADDR : numero del cluster vuoto
 *
 * Ritorna Falso se non è stato possibile trovare un cluster libero e addr non ha senso.
 * OTTIMIZZAZIONE : si potrebbe tenere un puntatore all'ultimo cluster trovato libero 
 */  


BOOL  getFree_fat  (const fat_ptr fat , dword fat_size, dword * addr) {
  
    int i=0; 
    fat_ptr tmp_fat=fat; 
    
    for ( i< 2 ; i< fat_size; i++, tmp_fat++) 
	   if(isFree_fat(*tmp_fat)) {
	      *addr=i; 
	      return TRUE; 
	   } 
	   
    *addr=NULL; 
    set_errno(1,"Non ci sono entrate libere nella fat"); 
    return FALSE; 
}

/* +PUBBLICA APPEND_FAT :
 * Funzione che aggiunge un cluster in ultima posizione alla catena di cluster passata come argomento. 
 * Questa funzione ci garantisce che le operazioni di scrittura avvengano in mutua esclusione mediante 
 * l'utilizzo di semafori. Se la catena non esiste la funzione la crea, per passare una catena vuota 
 * si può passare il valore zero, in quanto questa è una posizione riservata all'interno della fat. 
 * 
 * VOLUME : identifica il volume nel quale stiamo lavorando 
 * CHAIN  : identifica la catena sulla quale dobbiamo fare l'inserimento 
 *	    se zero la crea  
 * ADDR   : indirizzo nel quale viene inserita la posizione relativa alla fat 
 *	    a senso solo se è la funzione a successo.
 *
 *Return Un valore e booleano se true l'inserimento ha avuto successo.
 */ 

BOOL append_fat (const byte volume , const dword chain, dword * addr) {
  
  TABELLA_VOLUMI * tab=NULL; 
  fat_ptr fat=NULL; 
  dword cluster_ultimo=0, cluster_new=0;
  
  if(!(tab=get_volume(volume))) {
    set_errno(1,"Volume errato");
    return FALSE; 
  }
  
  
  if(chain <2 ) { 
     set_errno(1,"Cluster riservato");
    return FALSE; 
  }
  
  fat=tab->fat; 
  
  sem_wait(tab->semaphore); 
  
    if(!get_ultimo_fat(fat, chain, &cluster_ultimo)) {
      sem_signal(tab->semaphore); 
      return FALSE; 
    }
    
 //   flog(LOG_WARN, "Ultimo %d", cluster_ultimo); 
    
  if(!getFree_fat(fat, tab->fat_info.size_fat,&cluster_new)) {
     sem_signal(tab->semaphore);
    return FALSE; 
  }
  
  //devo scrivere nell'ultimo l'indirizzo del nuovo cluster
    
    if(!scrivi_fat(volume, cluster_ultimo, cluster_new))  {
      sem_signal(tab->semaphore);
      return FALSE; 
    }
    
  if(!scrivi_fat(volume, cluster_new, EOC_32)) {
    // se fallisce provo a ripristinare la situazione precedente
    if(!scrivi_fat(volume, cluster_ultimo,  EOC_32)) 
      set_errno(1,"Errore irrecuperabile, possibile perdita di dati");    
    sem_signal(tab->semaphore); 
    return FALSE; 
  }
  
  // ... ----->Ultimo---->new(EOC); 
  // finito inserimento 

  sem_signal(tab->semaphore); 
  
  if(addr)
    *addr=cluster_new; 
  
  return TRUE; 
  
}
/*GET_ULTIMO_FAT 
 * Funzione che analizza una catena di cluster e riporta  il numero dell'ultimo 
 * cluster presente, facendo le opportune verifiche sulla consistenza stessa 
 * della catena.
 * FAT : Puntatore alla fat in memoria 
 * CHAIN : numero di entrata del primo elemento di una catena 
 * ULTIMO : indirizzo nel quale verrà memorizzato il numero dell'ultimo cluster
 */

BOOL get_ultimo_fat (const fat_ptr fat, const dword chain,  dword * ultimo){
  
 dword value=0; 
 fat_ptr chain_tmp=fat+chain; //punto al primo elemento in memoria
 dword count=chain;
 
  if(chain < 2) {
    set_errno(1,"Cluster riservati");
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

/*funzione uguale alla get ultim, con lunica differenza che ripora il pen ultimo indirizzo*/ 

BOOL get_penUltimo_fat (const fat_ptr fat, const dword chain,  dword * pen_ultimo){
  
 dword value=0; 
 fat_ptr chain_tmp=fat+chain; //punto al primo elemento in memoria
 dword count=chain;
 dword ultimo=chain; 
 *pen_ultimo=0; 
 
  if(chain < 2) {
    set_errno(1,"Cluster riservati");
    return FALSE; 
  }  
 
 while (TRUE) { 
   
   if(*chain_tmp==FREE_32) { 
    set_errno(1,"Presente cluster libero nella lista"); 
    *pen_ultimo=0;
    return FALSE; 
   }
  
  if(*chain_tmp==BAD_CLUSTER_32) { 
    set_errno(1,"Presente cluster corrotto nella lista"); 
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




/*PUBLICA : DELETE_FAT 
 * Funzione che elimina l'ultimo cluster della catena. 
 */ 

BOOL delete_fat (const byte volume, const dword chain) { 
  
  TABELLA_VOLUMI * tab=NULL; 
  fat_ptr fat=NULL; 
  dword cluster_ultimo=0,cluster_penultimo=0; 
  dword backup=0; 
  
  if(!(tab=get_volume(volume))) {
    set_errno(1,"Volume errato");
    return FALSE; 
  }
  
  if(chain < 2) {
    set_errno(1,"Cluster riservati");
    return FALSE; 
  }
    
    
  fat=tab->fat; 
  leggi_fat(fat,cluster_penultimo,&backup);
  
  
  sem_wait(tab->semaphore); 
  
  if(!get_ultimo_fat(fat, chain, &cluster_ultimo)) {
    sem_signal(tab->semaphore);
    return FALSE; 
  }
  
  if(!get_penUltimo_fat(fat, chain, &cluster_penultimo)) {
    sem_signal(tab->semaphore);
    return FALSE;   
  }
  //flog(LOG_WARN, "penultimo : %d ultimo %d", cluster_penultimo, cluster_ultimo);
  
  if(!scrivi_fat(volume, cluster_penultimo, EOC_32)) 
    return FALSE; 

  
  if(!scrivi_fat(volume, cluster_ultimo, FREE_32)) {
    if(!scrivi_fat(volume, cluster_penultimo,  backup)) 
      set_errno(1,"Errore irrecuperabile, possibile perdita di dati");    
    sem_signal(tab->semaphore);
    return FALSE; 
  }
  
  sem_signal(tab->semaphore); 
  
  return TRUE;
 
}


inline BOOL delete_all_fat (const byte volume, const dword chain) { 
  
  while (delete_fat(volume,chain)); 
  reset_errno(); 
  return TRUE; 
  
}

/*funzione che mi riporta il valore del cluster successivo*/
/* uso l'int perchè se c'è un errore riporta meno uno  
  lo zero è una posizione riservata della fat 
  
 */ 

dword get_next_fat(const fat_ptr fat, const dword cluster) {
  
  dword value=*(fat+cluster)&MASK_32; 
  
  
  if ( fat == NULL || cluster < 2 ) { 
     set_errno(1,"Parametri errati"); 
     return FALSE;  
  }
  
  if ( cluster >= EOC_32) {
      set_errno(1,"Catena terminata"); 
      return FALSE; 
  }
  
  if ( value == BAD_CLUSTER_32) {
     set_errno(1,"Elemento corrotto");
     return FALSE; 
  }
  
  if (value == FREE_32) { 
      set_errno(1,"Elemento Vuoto");
      return FALSE; 
  } 
   
  
  return *(fat+cluster)&MASK_32;
  
}


/*Funzione che  crea una catena all'interno della da fat*/

BOOL create_fat (const byte volume, dword* chain) { 
  
  
  TABELLA_VOLUMI * tab=NULL; 
  fat_ptr fat=NULL; 
  dword cluster_new=0;
  
  if(!(tab=get_volume(volume))) {
    set_errno(1,"Volume errato");
    return FALSE; 
  }
  
  
  fat=tab->fat; 
  *chain=NULL; 
  
  sem_wait(tab->semaphore); 
  
  if(!getFree_fat(fat, tab->fat_info.size_fat,&cluster_new)) {
     sem_signal(tab->semaphore); 
    return FALSE; 
  }
  
  if(!scrivi_fat(volume, cluster_new, EOC_32)) {
    sem_signal(tab->semaphore); 
    return FALSE; 
  }
  
  sem_signal(tab->semaphore); 
  
  *chain=cluster_new; 
  
  return TRUE; 
  
}

/*funzione di test sul fat, usa tutte le funzioni di interfaccia disponibili 
 * per evitare malfunzionamenti del disco l'insieme di queste operazioni deve essere un ioperazione neutra 
 * in modo tale che non lasci dei risultati permaneti sul disco :) 
 */

void test_fat (byte label) { 
  
  dword cluster_1=0,cluster_2=0,t,i; 
  // per prima cosa creiamo un entry , se tutto avviene secondo le impostazioni 
  // deve essere crerata una catena e reso il numero del primo cluster 
  
  TABELLA_VOLUMI * tab=get_volume(label);
  
  
  // stampo le prime 20 entrate della fat in modo grezzo 
  
  flog(LOG_DEBUG,"FAT :"); 
  
  for ( i=0; i< 20; i++) 
    flog(LOG_DEBUG,"%x", *(tab->fat+i)); 
  
   flog(LOG_DEBUG, "\n"); 
  
  // creo la catena 1
  if(!create_fat(label, &cluster_1))
      perror("Creazione prima catena"); 
  else 
      flog(LOG_DEBUG, "Prima catena creata %d", cluster_1); 
  
  
  // creo la catena 2
  if(!create_fat(label, &cluster_2))
      perror("Creazione seconda catena"); 
  else 
      flog(LOG_DEBUG, "Seconda catena creata %d", cluster_2); 
  
  
  // inserisco nelle  catene intrecciandole  
  for (i=0; i< 5; i++) {
 
    if(!append_fat(label,cluster_1,&t)) 
      perror("APPEND 1"); 
    else 
      flog(LOG_DEBUG, "Inserito : %d", t); 
    
     if(!append_fat(label,cluster_2,&t))
      perror("APPEND 1"); 
    else 
      flog(LOG_DEBUG, "Inserito : %d", t); 
  
  }
  
  
  //stampo la catena 
   t=cluster_1; 
    do {
	flog(LOG_DEBUG, "Elemento : %d", t);
	if(!(t=get_next_fat(tab->fat, t))) {
	  perror("Errore lettura ");
	  break; 
	}
    } while( !isLast_fat(t)); 
    
    
  //stampo la catena 
   t=cluster_2; 
    do {
	flog(LOG_DEBUG, "Elemento : %d", t);
	if(!(t=get_next_fat(tab->fat, t))) {
	  perror("Errore lettura ");
	  break; 
	}
    } while( !isLast_fat(t)); 
    
    
  // elimino la catena   1   
 for (i=0; i< 5; i++) {
    if(!delete_fat(label,cluster_1)) 
      perror("Delete"); 
    else 
      flog(LOG_DEBUG, "Cancellato  : %d", i); 
  }
  
  if(!delete_fat(label,cluster_1)) 
          perror("Delete"); 

   t=cluster_1; 
   if(!(t=get_next_fat(tabella->fat, t))) 
     perror("Errore lettura");
   else 
     flog(LOG_DEBUG, "Elemento : %d", t);

  // elimino la catena due 
  delete_all_fat(label, cluster_2);
  t=cluster_2; 
   if(!(t=get_next_fat(tabella->fat, t))) 
     perror("Errore lettura");
   else 
     flog(LOG_DEBUG, "Elemento : %d", t);

  
  flog(LOG_DEBUG,"FAT :"); 
  
  for ( i=0; i< 20; i++) 
    flog(LOG_DEBUG,"%x", *(tab->fat+i)); 
  
  flog(LOG_DEBUG, "\n"); 
 
  
  // creo la catena 2
  if(!create_fat(label, &cluster_2))
      perror("Creazione seconda catena"); 
  else 
      flog(LOG_DEBUG, "Seconda catena creata %d", cluster_2); 
  
 delete_all_fat(label, cluster_2);


   // creo la catena 2
  if(!create_fat(label, &cluster_2))
      perror("Creazione seconda catena"); 
  else 
      flog(LOG_DEBUG, "Seconda catena creata %d", cluster_2); 
  
 delete_all_fat(label, cluster_2);
  
  return ; 
  
  
  
}



/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

