
#include "direntry.h"
#include "volumi.h"
#include "fat.h"

// oggetto globale 

extern void print_fcb(const FCB*); 
// Funzioni Private 
BOOL aggiungi_partizione ( TABELLA_VOLUMI ** tabella, natl ata, natl disco, natl indice) ;
void stampa_entry_tabella_volumi(const TABELLA_VOLUMI *); 
void format_root_fcb( FCB * fcb, const VOL_PTR volume); 


/*************************************************************************
 *Funzione che calcola il tipo di file system in base ai cluster di cui  *
 *e' composto.								 *
 *************************************************************************/

byte getFileSystemType ( dword DataSizeCluster ) {

    if ( DataSizeCluster < 4085 )
        return FAT12;
    else if ( DataSizeCluster < 65525 )
        return FAT16;
    else
        return FAT32;
}


//Funzione, testa se è presente il Magic Number 0x55 0xAA nel Settore di boot 
BOOL check_boot_sector ( BOOT_SECTOR * mbr ) {

    if ( mbr->magic_number[0] == 0x55 && mbr->magic_number[1]==0xAA )
        return TRUE;
    else
        return FALSE;
}







TABELLA_VOLUMI * crea_tabella_volumi() { 

  // per prima cosa devo prelevare la lista delle partizioni 
  // L'insieme  delle partizioni e' inserito nei descittori di interfaccia dei vari device 
  // Questo kernel al massimo puo gestire 4 hardisk ( 2 per ogni canale ATA ) 
  // per fare questo uso una routine di LIV_KERNEL che rende una partizione una per volta 

  TABELLA_VOLUMI *tabella=NULL; 
  int indice_partizione=0; 
  int i,j,indice=0; 

   // dischi li classifichiamo com hda hdb hdc hdd 
   // l'indirizzo zero nelle catene identifica il disco, quindi le varie partzioni 
   // partono da 1 e cosi via

   
   for ( i=0; i<N_ATA; i++ )   { 
      for ( j=0; j<N_DISK; j++)  {
	
	  //FAT32
	    for (  indice=0; ; indice++) 
	        if ((indice_partizione=get_partizione(i,j, FAT32, indice)) == -1) 
		    break; 
		else { 
		      flog(LOG_DEBUG, "Individuata Partizione FAT32 %d", indice_partizione); 
		      indice=indice_partizione;
		      if (!aggiungi_partizione(&tabella, i, j, indice_partizione)) {
			  perror("Errore aggiungi_partizione");  
			  //return FALSE; 
			}
		      }
	 
	 
	  // FAT16
	    for (  indice=0; ; indice++) 
	        if ((indice_partizione=get_partizione(i,j, 0X0C, indice)) == -1) 
	           break; 
		else { 
		      flog(LOG_WARN, "Partizione FAT32 %d", indice_partizione); 
		      indice=indice_partizione;
		      if (!aggiungi_partizione(&tabella, i, j, indice_partizione)) {
			  perror("Errore aggiungi_partizione");  
			  //return FALSE; 
			}
		      }
// 	 // FAT12
// 	    for ( indice=0; ; indice++) 
// 	        if ((indice_partizione=get_partizione(i,j, FAT12, indice)) == -1) 
// 	           break; 
// 		else {
// 		   indice=indice_partizione;
// 		    aggiungi_partizione(&tabella,i,j, indice_partizione); 
// 		   flog(LOG_WARN, "Partizione FAT12 %d", indice_partizione); 
// 	    
// 		   }
// 	*/ 
	  }	//j   
      }		// i 

  if (!tabella) 
      set_errno(ENXIO, "Nessun volume FAT32"); 
  else 
      reset_errno(); 
  
  return tabella; 
        
}

/*FUNZIONE**********************************************
 * Funzione che alloca nella memoria dinamica un campo *
 * per la tabella dei volumi. 			       *
 * Se la funzione ha successo ritorna TRUE altrimenti  *
 * ritorna FALSE e setta opportunamente l'errore.      *
 *******************************************************/


BOOL  aggiungi_partizione ( TABELLA_VOLUMI ** tab, natl ata, natl disco, natl indice) {

  BOOT_SECTOR *  bpb=NULL;  // informazioni volume fat
  TABELLA_VOLUMI *new_tabella=NULL; 
  static byte label='C'; 
   /*NB la grandezza */ 
  byte  set_for_cluster=0;
  word  root_dir_sector=0; 
  word  byts_for_sect=0;
  word  n_sect_reserved=0; 
  word  n_ent_root=0;
  byte  n_FAT=0;
  dword data_size; 
  dword FAT_size=0;  
  dword tot_settori=0;  
  dword data_size_cluster=0; 
  dword last_set_data=0; 
  dword first_set_data=0; 
  dword first_set_FAT=0; 
  byte type_fs=0; 
  
  if(tab==NULL) {
     set_errno(EINVAL,"Tabella invalida (%s-line%d)", __FILE__, __LINE__); 
     return FALSE;
  } 
  
  
  if(!(bpb=(BOOT_SECTOR*)mem_alloc(BS_SIZE, 1))) { 
    set_errno(ENOMEM, "Errore mem_alloc (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  if(!(new_tabella=(TABELLA_VOLUMI* )mem_alloc(sizeof(TABELLA_VOLUMI),1))) { 
    set_errno(ENOMEM, "Errore mem_alloc (%s-line%d)", __FILE__, __LINE__); 
    mem_free(bpb); 
    return FALSE; 
  }
    
  memset(bpb, 0, BS_SIZE);
  memset(new_tabella,0, sizeof(TABELLA_VOLUMI));

    
   new_tabella->next=*tab; 
   *tab=new_tabella; 
   
   
  // Leggo il BPB  // 
    if(read_part_n(ata, disco, indice , 0, 1, (void*)bpb) < 0 ) { 
     if (!(*tab)->next)
	*tab=NULL;
     else 
       *tab=(*tab)->next; 
      label++; 
      mem_free(new_tabella); 
      mem_free(bpb); 
      return FALSE; 
    }
    
 // VERIFICO SE E' IL SETTORE DI BOOT 
    if( check_boot_sector((BOOT_SECTOR *)bpb) != TRUE) {
      set_errno(EINVAL,"BPB errato %c", label); 
      if (!(*tab)->next)
	*tab=NULL;
     else 
       *tab=(*tab)->next; 
      label++; 
      mem_free(new_tabella); 
      mem_free(bpb); 
      return FALSE; 
    }
  
    // inserisco i dati record dalla tabella dei volumi; 
    

    
    /* Gestione settori e Cluste */
    set_for_cluster=bpb->BPB_SecPerClus;
    byts_for_sect=bpb->BPB_BytsPerSec; 
    n_FAT=bpb->BPB_NumFATs; 
    FAT_size=FATsize(bpb->BPB_FATSz16,bpb->bpb_32.BPB_FATsz32);
    n_sect_reserved=bpb->BPB_RsvdSecCnt;
    n_ent_root=bpb->BPB_RootEntCnt; 
    first_set_FAT=FirstSectFAT(n_sect_reserved);
    tot_settori=TotSector(bpb->BPB_TotSec16, bpb->BPB_TotSec32);

    /* Gestione regione dati */
    data_size=DataSize( tot_settori, n_sect_reserved, bpb->BPB_NumFATs, FAT_size, root_dir_sector); 
    data_size_cluster=DataSizeCluster( data_size, set_for_cluster); 
    first_set_data= FirstDataSector( n_sect_reserved,n_FAT, FAT_size,root_dir_sector); 
    last_set_data = first_set_data + data_size;     

    
    
    if ( (type_fs=getFileSystemType (data_size_cluster)) != FAT32 ) {
      set_errno(EINVAL,"FS Errato %c TYPE %X DATA SIZE CLUSTER %d", label, type_fs, data_size_cluster); 
      if (!(*tab)->next)
	*tab=NULL;
     else 
       *tab=(*tab)->next; 
      label++; 
      mem_free(new_tabella); 
      mem_free(bpb); 
      return FALSE; 
    }
  
    
    /*riempio i vari campi*/ 
    new_tabella->type_fs=type_fs;
    new_tabella->ata=ata; 
    new_tabella->disco=disco; 
    new_tabella->indice_partizione=indice; 
    new_tabella->label=label;
    label++; // incremento l'eticheta; 
    
    new_tabella->fat_info.sectors_for_cluster=set_for_cluster; 
    new_tabella->fat_info.byts_for_sector=bpb->BPB_BytsPerSec;
    new_tabella->fat_info.first_set_data=first_set_data; 
    new_tabella->fat_info.size_data=data_size; 
    tot_settori=TotSector(bpb->BPB_TotSec16, bpb->BPB_TotSec32);
    new_tabella->fat_info.first_set_fat=first_set_FAT; 
    new_tabella->fat_info.size_fat=FAT_size; 
    new_tabella->fat_info.first_cluster_directory= (bpb->bpb_32.BPB_RootClus); 
    new_tabella->size_fat=FAT_size*byts_for_sect; 
    new_tabella->sem_fat=sem_ini(MUTEX); // mutua esclusione 
    new_tabella->sem_data=sem_ini(MUTEX); 
    
    if(!((new_tabella->fcb_root)=mem_alloc(sizeof(FCB),1))){
      set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
       if (!(*tab)->next)
	*tab=NULL;
     else 
       *tab=(*tab)->next; 
      mem_free(new_tabella); 
      mem_free(bpb); 
      return FALSE; 
    }
    
    
    memset(new_tabella->fcb_root,0, sizeof(FCB)); 


    mem_free(bpb); 
    return TRUE; 
}



extern TABELLA_VOLUMI *tabella; 

 void stampa_tabella_volumi() { 
  
  TABELLA_VOLUMI * next=NULL;  
  
  if ( tabella == NULL) { 
    flog(LOG_WARN, "Non e' presente nessun tabella dei volumi "); 
    return; 
 }
  next=tabella; 
  
flog(LOG_DEBUG,"********************* VOLUMI ***************************"); 
   
  while ( next ) { 
    stampa_entry_tabella_volumi(next);  
    next=next->next; 
  }
  
flog(LOG_DEBUG,"*********************************************************");  
  
} 

/*FUNZIONE*****************************
 * Elimina tutta la tabella dei volumi*
 **************************************/

void delete_tabella() { 
  
  TABELLA_VOLUMI * next=NULL, *temp=NULL;  
  
  if ( tabella == NULL) {  
    return; 
 }
 
  next=tabella; 
  temp=tabella; 
  
  while ( next ) { 
    temp=next->next;
    mem_free(next);
    next=temp; 
  }
  
} 


 void stampa_entry_tabella_volumi(const TABELLA_VOLUMI * l) {
  

  flog(LOG_DEBUG, " Label               : %c", l->label); 
  flog(LOG_DEBUG, " ATA                 : %x", l->ata); 
  flog(LOG_DEBUG, " DISCO               : %x", l->disco); 
  flog(LOG_DEBUG, " Partizione          : %x", l->indice_partizione); 
  flog(LOG_DEBUG, " TYPE                : %x", l->type_fs);
  flog(LOG_DEBUG, " Byte for sector     : %x", l->fat_info.byts_for_sector);
  flog(LOG_DEBUG, " Sector for cluster  : %x", l->fat_info.sectors_for_cluster);
  flog(LOG_DEBUG, " First FAT sector    : %x", l->fat_info.first_set_fat); 
  flog(LOG_DEBUG, " FAT size            : %x", l->fat_info.size_fat); 
  flog(LOG_DEBUG, " First Data sector   : %x", l->fat_info.first_set_data);
  flog(LOG_DEBUG, " DATA size           : %x", l->fat_info.size_data); 
  flog(LOG_DEBUG, " Cluster             : %x", l->fat_info.first_cluster_directory); 
  flog(LOG_DEBUG, " Semaphore data      : %d", l->sem_data);
  flog(LOG_DEBUG, " Semaphore fat       : %d", l->sem_fat);

}

/*FUNZIONE ************************************
 * Funzione che rende il puntatore all'entry  *
 * della tabella cercata se questa è presente.*
 * Altrimenti ritorna un punatore NULL.       *
 **********************************************/

TABELLA_VOLUMI * get_volume ( byte label) { 
  
  TABELLA_VOLUMI * tmp=tabella; 

  if ( label  < 'C' ) 
    return NULL; 

  while ( tmp )
    if ( tmp->label == label)  
      return tmp; 
    else  
      tmp=tmp->next; 


  return NULL; 

} 




