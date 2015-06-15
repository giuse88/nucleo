
#include <volumi.h>
#include <type.h> 
#include <sistema.h>


BOOL  aggiungi_partizione ( TABELLA_VOLUMI ** tabella, natl ata, natl disco, natl indice) ;
void stampa_entry_tabella_volumi(const TABELLA_VOLUMI *); 

extern TABELLA_VOLUMI * tabella; 

TABELLA_VOLUMI * crea_tabella_volumi() { 

  // per prima cosa devo prelevare la lista delle partizioni 
  // L'insieme  delle partizioni è inserito nei descittori di interfaccia dei vari device 
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
	flog(LOG_WARN, "DISCO : %d %d", i,j);
	  
	    for (  indice=0; ; indice++) 
	        if ((indice_partizione=get_partizione(i,j, FAT32, indice)) == -1) 
	           break; 
		else { 
		  indice=indice_partizione;
		     flog(LOG_WARN, "Partizione FAT32 %d", indice_partizione); 
		      aggiungi_partizione(&tabella, i, j, indice_partizione); 	
		   }

	    for (  indice=0; ; indice++) 
	        if ((indice_partizione=get_partizione(i,j, FAT16, indice)) == -1) 
	           break; 
		else {
		    indice=indice_partizione;
		     aggiungi_partizione(&tabella,i,j,indice_partizione); 
		    flog(LOG_WARN, "Partizione FAT16 %d", indice_partizione); 
		  }

	    for ( indice=0; ; indice++) 
	        if ((indice_partizione=get_partizione(i,j, FAT12, indice)) == -1) 
	           break; 
		else {
		   indice=indice_partizione;
		    aggiungi_partizione(&tabella,i,j, indice_partizione); 
		   flog(LOG_WARN, "Partizione FAT12 %d", indice_partizione); 
	    
		   }
      }  
      }

   
  return tabella; 
        
}


BOOL  aggiungi_partizione ( TABELLA_VOLUMI ** tabella, natl ata, natl disco, natl indice) {

  BPB *  bpb=(BPB*)mem_alloc(BPB_SIZE, 1);  // informazioni volume fat
  TABELLA_VOLUMI *new_tabella=(TABELLA_VOLUMI* )mem_alloc(sizeof(TABELLA_VOLUMI),1); 
  static byte label='C'; 

  memset(bpb, 0, BPB_SIZE);
  memset(new_tabella,0, sizeof(TABELLA_VOLUMI));

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

    if(tabella==NULL) {
      mem_free(new_tabella); 
      mem_free(bpb); 
      return FALSE;
    } 

      new_tabella->next=*tabella; 
    *tabella=new_tabella; 
   
   
  // Leggo il BPB  // 
    if(read_part_n(ata, disco, indice , 0, 1, (void*)bpb) < 0 ) 
      flog(LOG_WARN , "ERRORE READ "); 
  
 // VERIFICO SE É IL SETTORE DI BOOT 
    if( check_boot_sector((MBR *)bpb) == TRUE)
         flog(LOG_WARN, "BPB correct\n"); 
    else {
         flog(LOG_WARN, "BPB incorrent\n"); 
    }
  
    // inserisco i dati record dalla tabella dei volumi; 
    
    /* Gestione settori e Cluste */
    set_for_cluster=bpb->BPB_SecPerClus;
    byts_for_sect=bpb->BPB_BytsPerSec; 
    n_FAT=bpb->BPB_NumFATs; 
     FAT_size=FATsize(bpb->BPB_FATSz16,bpb->bpb_32.BPB_FATsz32);
    n_sect_reserved=bpb->BPB_RsvdSecCnt;
    n_ent_root=bpb->BPB_RootEntCnt; 
    first_set_FAT=FirstSectFAT( n_sect_reserved);
    tot_settori=TotSector(bpb->BPB_TotSec16, bpb->BPB_TotSec32);

    /* Gestione regione dati */
    flog(LOG_WARN, " Settori Riservati %d  n_fat  %d, Fat Size %d",  n_sect_reserved, bpb->BPB_NumFATs, FAT_size);

     data_size=DataSize( tot_settori, n_sect_reserved, bpb->BPB_NumFATs, FAT_size, root_dir_sector); 
    data_size_cluster=DataSizeCluster( data_size, set_for_cluster); 
    first_set_data= FirstDataSector( n_sect_reserved,n_FAT, FAT_size,root_dir_sector); 
    last_set_data = first_set_data + data_size;     

    new_tabella->type_fs=getFileSystemType (data_size_cluster);
    new_tabella->ata=ata; 
    new_tabella->disco=disco; 
    new_tabella->indice_partizione=indice; 
    new_tabella->label=label;

    label++; // incremento l'eticheta; 

    new_tabella->fat_info.sectors_for_cluster=set_for_cluster; 
    new_tabella->fat_info.byts_for_sector=bpb->BPB_BytsPerSec;
  
    new_tabella->fat_info.first_set_data=first_set_data; 
    new_tabella->fat_info.size_data=data_size; 
    flog(LOG_DEBUG, "Settore data %d", first_set_data); 
    tot_settori=TotSector(bpb->BPB_TotSec16, bpb->BPB_TotSec32);
 
    
    new_tabella->fat_info.first_set_fat=first_set_FAT; 
    new_tabella->fat_info.size_fat=FAT_size; 

    new_tabella->fat_info.first_cluster_directory= (bpb->bpb_32.BPB_RootClus); 

    new_tabella->semaphore=sem_ini(1); // mutua esclusione 
   

    return FALSE; 
}

 
 void stampa_tabella_volumi( const TABELLA_VOLUMI * list) { 
  if ( list == NULL) 
    return; 
  
  stampa_entry_tabella_volumi(list); 
  stampa_tabella_volumi(list->next); 
} 

 void stampa_entry_tabella_volumi(const TABELLA_VOLUMI * l) {
  

  flog(LOG_DEBUG, " \nLabel :              %c", l->label); 
  flog(LOG_DEBUG, " ATA        :             %x", l->ata); 
  flog(LOG_DEBUG, " DISCO  :             %x", l->disco); 
  flog(LOG_DEBUG, " TYPE     :             %x", l->type_fs);
  flog(LOG_DEBUG, " Byte for sector       : %x", l->fat_info.byts_for_sector);
  flog(LOG_DEBUG, " Sector for cluster  : %x", l->fat_info.sectors_for_cluster);
  flog(LOG_DEBUG, " Partizione               : %x", l->indice_partizione); 
  flog(LOG_DEBUG, " First FAT sector     : %x",l->fat_info.first_set_fat); 
  flog(LOG_DEBUG, " FAT size                  : %x", l->fat_info.size_fat); 
  flog(LOG_DEBUG, " First Data sector   : %x", l->fat_info.first_set_data);
  flog(LOG_DEBUG," CLuster                  : %x ",  l->fat_info.first_cluster_directory); 
  flog(LOG_DEBUG, " DATA size               : %x\n", l->fat_info.size_data); 

}

TABELLA_VOLUMI * get_volume ( byte label) { 
  
  TABELLA_VOLUMI * tmp=tabella; 

  if ( label  < 'C' ) 
    return NULL; 

  while ( tmp )
    if ( tmp->label == label)  
      return tmp; 
    else  
      tmp=tmp->next; 

  set_errno("Volume non trovato");  
  return NULL; 

} 

