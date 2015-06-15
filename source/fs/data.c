#include "data.h"


/*******************************************************************************
 *IMPLEMENTAZIONE DELL'INTERFACCIA PER LA GESTIONE DELLA ZONA DATI DEL DISCO   *
 *******************************************************************************/ 

/* FUNZIONE WRITE_DATA **********************************************************
 * La funzione write_data scrive il buffer nella catena specificata da          *
 * chain astraendo il concetto di cluster. La posizione nella catena è data 	*
 * da offset.Se il buffer da scrivere è maggiore dello spazio libero presente	*
 * su chain la funzione aggiunge un ulteriore cluster.  			*
 * 										*
 * LABEL  : volume sul quale scrivere						*
 * CHAIN  : catena di cluster coinvolta nella scrittura				*
 * OFFSET : posizione su cui scrivere						*
 * BUF    : buffer di input							* 
 * SIZE   : grandezza del buffer da scrivere					*
 * 										*
 * Se tutto la scrittura ha avuto successo ritorna il numero di byte scritti, 	*
 * altrimenti ritorna -1.							*
 * N.B se l'offset interessa una parte della catena non presente questa fun-	*
 * zione deve estendere la catena ( specifica UNIX )				*
 ********************************************************************************/


int write_data( VOL_PTR volume, dword chain, size_t offset, const byte * buf, size_t size){ 


  dword size_cluster_byte=0; 
  dword  offset_cluster_first=0,offset_cluster_last=0, n_cluster=0, n_load_cluster=0; 
  dword i=0; 
  dword temp_chain=chain, settore=0, n_settori=0,resto_cluster=0;
  byte *cluster=NULL; 
  fat_ptr fat=NULL; 
  lword n_write=0; 
  
   // non è un errore
  // se in questo caso viene settato un offset , questo è ignorato 
  if ( size == 0) { 
    reset_errno(); 
    return 0; 
  }
   
  //verifico i parametri
  if ( chain <2  || !buf || !volume || size <0) {
      set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
      return -EINVAL;
  }
  
 

  //verifico la catena 
  if (!get_next_fat(volume->fat, chain)) {
      set_errno(EINVAL,"Chain errato (%s-line%d)", __FILE__, __LINE__); 
      return -EINVAL;
  }
 
  // grandezza cluster ( varia tra disco e disco) 
  size_cluster_byte=volume->fat_info.sectors_for_cluster*volume->fat_info.byts_for_sector; 
  // prelevo puntatore alla fat 
  fat=volume->fat; 
  
  // alloco grandezza cluster 
  if(!(cluster=mem_alloc(size_cluster_byte,1))){
      set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
      return -ENOMEM; 
  }
  
  memset(cluster,0, size_cluster_byte);
  
  // calcoliamo quanti cluster dobbiamo scorrere prima di trovare quello che serve a noi 
  n_cluster= offset/size_cluster_byte;
  // calcolo l'offset che mi interessa all'interno del primo cluster 
  offset_cluster_first= offset%size_cluster_byte; 
  //calcolo l'offset che mi interessa sull'ultimo cluster 
  offset_cluster_last=(size%size_cluster_byte+offset_cluster_first)%size_cluster_byte; 
  // calcolo quanto spazio rimane nel primo cluster a partire da offset 
  resto_cluster= size_cluster_byte - (offset_cluster_first);
  //calcoliamo quanti cluster caricare in memoria 
   n_load_cluster=((offset_cluster_first+size)/size_cluster_byte) + (((offset_cluster_first+size)%size_cluster_byte)?1:0);  
  // salvo il numero di settori di cui è composto un cluster 
  n_settori=volume->fat_info.sectors_for_cluster;
  
  //Scorre finché non trovo il primo cluster coinvolto nella scrittura 
  for ( i=0; i< n_cluster; i++) {
      if(!(temp_chain=get_next_fat(fat,temp_chain))){  
	set_errno(EINVAL,"Offset errato o chain corrotta (%s-line%d)", __FILE__, __LINE__); 
	mem_free(cluster);
	return -EINVAL; 
      }
      
       if (isLast_fat(temp_chain) && offset) { 
	if(!append_fat(volume, chain, &temp_chain))  {
	  mem_free(cluster); 
	  return -1; 
	}
   }
  }
  
  // Nel caso si faccia una scrittura con un offset pari alla grandezza di un cluster,  
  // e la catena è formata da un solo , la funzione deve aggiungere il cluster 
  // e poi eseguire la scrittura su questo nuovo cluster. 
  // questo caso lo trattiamo facendo l'aggiunta del cluster in anticipo. 
  
  // ES offset=4096 size =1 
  
  //se offset è diverso da zero si ricade nelle scritture a cavallo tra più cluster 
//    if (temp_chain == EOC_32 && !offset_cluster_first && offset) { 
//  	if(!append_fat(volume,chain, &temp_chain))  {
//  	  mem_free(cluster); 
//  	  return -1; 
//  	}
//     }
  

#ifdef DEBUG_FS
  flog(LOG_DEBUG,"########## WRITE  #########################"); 
  flog(LOG_DEBUG, "Cluster        : %x", chain);
  flog(LOG_DEBUG, "Offset         : %x", offset);
  flog(LOG_DEBUG, "Size Cluster   : %x", size_cluster_byte); 
  flog(LOG_DEBUG, "Primo cluster  : %x", temp_chain);
  flog(LOG_DEBUG, "n_load_cluster : %d", n_load_cluster);    
  flog(LOG_DEBUG, "Offset first   : %d", offset_cluster_first); 
  flog(LOG_DEBUG, "Offset last    : %d", offset_cluster_last); 
  flog(LOG_DEBUG, "Resto          : %d", resto_cluster); 
  flog(LOG_DEBUG,"###########################################");
#endif
     
      
  //Su temp_chain ho il primo cluster coinvolto nella scrittura 
	
  for (i=0; i< n_load_cluster; i++)  {
    
        // calcolo il settore poiché il driver lavora sui settori 
        // Verifica la consistenza della catena o dell'offset simile al controllo alla linea 94 
      if ( temp_chain !=0 && !isLast_fat(temp_chain)) 
	settore=FirstDataSectorOfCluster(temp_chain, volume->fat_info.sectors_for_cluster, volume->fat_info.first_set_data); 
      else {
	    set_errno(EINVAL,"Offset errato o chain corrotta (%s-line%d)", __FILE__, __LINE__);  
	    mem_free(cluster);
	    return -1;
      }
 
      //Gestione buffer di scrittura 
      
      // il primo è l'ultimo cluster vanno trattati in modo diverso 
 
      // GESTIONE PRIMO CLUSTER 
      if (i==0) { 
	  
	  read_part_n(volume->ata, volume->disco, volume->indice_partizione, settore, n_settori, (void *) cluster); 

	  if ( size > resto_cluster)  {  
	    memcpy((cluster+offset_cluster_first), buf,  resto_cluster); 
	    n_write+=resto_cluster;
	    buf+=resto_cluster;
	  }else  {
	    memcpy((cluster+offset_cluster_first), buf, size);
	    n_write+=size;
	    buf+=size; 
	  }
        // GESTIONE ULTIMO CLUSTER 
      } else if ( i==(n_load_cluster -1)) {  
	    read_part_n(volume->ata, volume->disco, volume->indice_partizione, settore, n_settori, (void *) cluster); 
      	    memcpy(cluster, buf, offset_cluster_last);
	    buf+=offset_cluster_last; 
	    n_write+=offset_cluster_last; 
      }
      //GESTIONE RESTANTI CLUSTER 
	else { 
	  memcpy(cluster,buf, size_cluster_byte);
	  buf+=size_cluster_byte; 
	  n_write+=size_cluster_byte; 
	}
	
    
    write_part_n(volume->ata, volume->disco, volume->indice_partizione, settore, n_settori, (void *) cluster);
    
    // prelevo il successivo devo crearlo se non presente  
    if (!(temp_chain= get_next_fat(fat, temp_chain)))  {
	mem_free(cluster);
	return -1; 
    }
    
    // significa che sono all'ultimo , ma non ho finito le scritture 
    // quindi l'ho aggiungo, a differenza del caso di sopra arrivo 
    // a questa condizione dipende dalla grandezza del buffer 
    if (isLast_fat(temp_chain) && i < (n_load_cluster -1)) {
	if(!append_fat(volume,chain, &temp_chain)) {
	  mem_free(cluster);
	  return -1; 
	}
      }
  
  } //for 
  
  
  return n_write; 
  
}




/* FUNZIONE READ_DATA ***********************************************************
 * La funzione read_data scrive il buffer nella catena specificata da           *
 * chain astraendo il concetto di cluster. La posizione nella catena è data 	*
 * da offset. Se la funzione ha un offset maggiore dei cluster allocati questa  *
 * riporta il valore zero							*
 * 										*
 * LABEL  : volume sul quale scrivere						*
 * CHAIN  : catena di cluster coinvolta nella lettura				*
 * OFFSET : posizione su cui scrivere						*
 * BUF    : buffer di output							* 
 * SIZE   : grandezza del buffer da leggere					*
 * 										*
 * Se tutto la scrittura ha avuto successo ritorna il numero di byte scritti, 	*
 * altrimenti ritorna -1.							*
 ********************************************************************************/


int read_data(VOL_PTR volume , dword chain, lword offset,  byte * buf, dword size) { 
  
  dword size_cluster_byte=0; 
  dword  offset_cluster_first=0, offset_cluster_last=0, n_cluster=0, n_load_cluster=0; 
  dword i=0; 
  dword temp_chain=chain, settore=0, n_settori=0,resto_cluster=0;
  byte *cluster=NULL; 
  fat_ptr fat=NULL; 
  lword n_read=0; 
  
  
  
  //verifico i parametri
  if ( chain <2  || !buf || !volume || size < 0 ) {
      set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
      flog(LOG_WARN, "chain %d buf %x volume %x size %d", chain, buf, volume, size); 
      return -EINVAL;
  }
  
  // non è un errore
  if ( size == 0) { 
    reset_errno(); 
    return 0; 
  }
  

  
  //verifico la catena 
  if (!get_next_fat(volume->fat, chain)) {
      set_errno(EINVAL,"Chain errato %d %d (%s-line%d)", chain, (volume->fat+chain),  __FILE__, __LINE__); 
      return -EINVAL;
  }
  
  // grandezza cluster ( varia tra disco e disco) 
  size_cluster_byte=volume->fat_info.sectors_for_cluster*volume->fat_info.byts_for_sector; 
  fat=volume->fat; 
  
  // alloco grandezza cluster 
  if(!(cluster=mem_alloc(size_cluster_byte,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return -ENOMEM; 
  }
  
  memset(cluster,0, size_cluster_byte);
  
  // calcoliamo quanti cluster dobbiamo scorrere prima di trovare quello che serve a noi 
  n_cluster= offset/size_cluster_byte;
  // calcolo l'offset che mi interessa all'interno di un cluster 
  offset_cluster_first= offset%size_cluster_byte; 
    //calcolo l'offset che mi interessa sull'ultimo cluster 
   offset_cluster_last=(size%size_cluster_byte+offset_cluster_first)%size_cluster_byte;
  // calcolo quanto spazio rimane nel primo cluster a partire da offset 
  resto_cluster= size_cluster_byte - (offset_cluster_first);
  //calcoliamo quanti cluster caricare in memoria 
  n_load_cluster=((offset_cluster_first+size)/size_cluster_byte) + (((offset_cluster_first+size)%size_cluster_byte)?1:0); 
  // salvo il numero di settori di cui è composto un cluster 
  n_settori=volume->fat_info.sectors_for_cluster;


  
 //Scorre finché non trovo il primo cluster coinvolto nella lettura
  for ( i=0; i< n_cluster; i++) {
      if(!(temp_chain=get_next_fat(fat,temp_chain))){  
	set_errno(EINVAL,"Offset errato o chain corrotta (%s-line%d)", __FILE__, __LINE__); 
	mem_free(cluster);
	return -EINVAL; 
      }
   //     flog(LOG_INFO, "chain %d", temp_chain); 
  }
 
  
  if ( isLast_fat(temp_chain))  {  	
	mem_free(cluster);
	return 0;
  }
  
  
#ifdef DEBUG_FS
  flog(LOG_DEBUG,"########## READ   #########################"); 
  flog(LOG_DEBUG, "Cluster        : %x", chain);
  flog(LOG_DEBUG, "Offset         : %x", offset);
  flog(LOG_DEBUG, "Size Cluster   : %x", size_cluster_byte); 
  flog(LOG_DEBUG, "Primo cluster  : %x", temp_chain);
  flog(LOG_DEBUG, "n_load_cluster : %d", n_load_cluster);    
  flog(LOG_DEBUG, "Offset first   : %d", offset_cluster_first); 
  flog(LOG_DEBUG, "Offset last    : %d", offset_cluster_last); 
  flog(LOG_DEBUG, "Resto          : %d", resto_cluster); 
  flog(LOG_DEBUG,"###########################################");
#endif
  
  
  
  
  //Su temp_chain ho il primo cluster coinvolto nella lettura    
  for (  i=0; i< n_load_cluster; i++)  {
    
        // calcolo il settore poiché il driver lavora sui settori 
      if ( temp_chain !=0 && !isLast_fat(temp_chain)) 
	settore=FirstDataSectorOfCluster(temp_chain, volume->fat_info.sectors_for_cluster, volume->fat_info.first_set_data); 
      else {
	    mem_free(cluster);
	    return n_read;
      }
      
      // chiamo il driver
      read_part_n(volume->ata, volume->disco, volume->indice_partizione, settore, n_settori, (void *) cluster); 
	 
      // il primo è l'ultimo vanno trattati in modo diverso 

      //PRIMO
      if (i==0) { // primo 

	  if ( size > resto_cluster) {
	    // carico parte del buffer richiesto  la seconda parte verrà caricata da un altra interazione 
	    memcpy(buf, (cluster+offset_cluster_first), resto_cluster); 
	    n_read+=resto_cluster;
	    buf+=resto_cluster;
	  }else  {
	    // carico tutto il buffer richiesto
	    memcpy(buf, (cluster+offset_cluster_first), size);
	    n_read+=size;
	    buf+=size; 
	  }
	    //ULTIMO
      } else if ( i==(n_load_cluster -1)) {  
	    // copio la parte rimanente 
	    memcpy(buf, cluster, offset_cluster_last);
	    n_read+=offset_cluster_last;
	    buf+=offset_cluster_last; 
      }
	else { //restanti 
	  memcpy(buf, cluster, size_cluster_byte);
	  buf+=size_cluster_byte;
	  n_read+=size_cluster_byte;
	}
      
    // prelevo il successivo 
    if (!(temp_chain= get_next_fat(fat, temp_chain)))  {
	mem_free(cluster);
	return -1; 
    }
    
  }//for

  mem_free(cluster);
  return n_read; 
  
}


