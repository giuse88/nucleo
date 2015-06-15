
#include "data.h"
#include "fat.h" 
#include "volumi.h" 
#include "sistema.h"
#include "type.h"
#include "errno.h" 

/*Interfaccia per la gestione della zona dati del disco */ 

// funzione che scrive buf sul volume nella catena chain 
// la funzione write controll ache l'offeset appartenga alla catena di cluster 
BOOL write_data(byte label , dword chain, lword offset, const byte * buf, dword size){ 
//TESTARE OFFSET   

  TABELLA_VOLUMI * part=NULL; 
  dword size_cluster_byte=0; 
  dword  offset_cluster_first=0,offset_cluster_last=0, n_cluster=0, n_load_cluster=0; 
  dword i=0; 
  dword temp_chain=chain, settore=0, n_settori=0,resto_cluster=0;
  byte *cluster=NULL; 
  fat_ptr fat=NULL; 
  lword n_write=0; 
  
  if(chain <2 ) { 
      set_errno(1,"Bad Chain"); 
      return FALSE; 
  } 
  
  if ( buf == NULL) {
    set_errno(1,"Bad buffer"); 
    return FALSE; 
  }
  
  if (size <0 ) { 
      set_errno(1,"Bad Size"); 
      return FALSE; 
  } 
  
  if (!(part=get_volume(label)))
      return FALSE; 
  
  // grandezza cluster ( varia tra disco e disco) 
  size_cluster_byte=part->fat_info.sectors_for_cluster*part->fat_info.byts_for_sector; 
  fat=part->fat; 
  
  if(!(cluster=mem_alloc(size_cluster_byte,1))){
      set_errno(1,"Errore memoria"); 
	return FALSE; 
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
   n_load_cluster=((offset_cluster_first+size)/size_cluster_byte) + ((size%size_cluster_byte)?1:0);  
  // salvo il numero di settori di cui è composto un cluster 
  n_settori=part->fat_info.sectors_for_cluster;
  
  // se qui mancano cluster questa li deve aggiungere 
  
  for ( i=0; i< n_cluster; i++) {
      if(!(temp_chain=get_next_fat(fat,temp_chain)))
	return FALSE; 
  }
  
  // nel caso si faccesse una scittura con offset = ala grandezza della catena 
  // la scrittura deve avvenire quindi aggiungo il cluster 
  // se sonno arrivato a alla fine significa che qui ho EOC 
  
  flog(LOG_WARN, "ULTIMO %x", temp_chain); 
 
  // condizione che mi rapresenta la scrittura nel primo byte 
  // di un nuovo cluster 
  // offset evita che si possano creare le catene con questa chiamata 
  if (temp_chain == EOC_32 && !offset_cluster_first && offset) { 
    // in questo caso aggiungo un cluster 
	if(!append_fat(label, chain, &temp_chain)) 
	  return FALSE; 
	
	 flog(LOG_WARN, "NEW %x", temp_chain); 
  }
  
#ifdef DEBUG_FS
  flog(LOG_DEBUG, "Primo cluster : %d", temp_chain);
  flog(LOG_DEBUG, "Size Cluster : %x", size_cluster_byte); 
  flog(LOG_DEBUG, "n_load_cluster : %d", n_load_cluster);    
  flog(LOG_DEBUG, "Offset first : %d", offset_cluster_first); 
  flog(LOG_DEBUG, "Resto : %d", resto_cluster); 
  flog(LOG_DEBUG, "Offset last : %d", offset_cluster_last); 
#endif    
      
  //Su temp_chain ho il primo cluster coinvolto nella scrittura 
	
  for (i=0; i< n_load_cluster; i++)  {
    
        // calcolo il settore poiché il driver lavora sui settori 
        // se l'offeset first è zero allora posso aggiungere un cluster anche se dovuto all'offset 
        // serve per gestire il caso in cui si fa una scrittura della grandezza del cluster  
      if ( temp_chain !=0 && temp_chain != EOC_32) 
	settore=FirstDataSectorOfCluster(temp_chain, part->fat_info.sectors_for_cluster, part->fat_info.first_set_data); 
      else {
	    set_errno (1,"Si e' provato a leggere un cluster non buono"); 
	    mem_free(cluster);
	    return FALSE;
      }
    	 
      // il primo è l'ultimo vanno trattati in modo diverso 

  
      // GESTIONE PRIMO CLUSTER 
      if (i==0) { 
	  
	  read_part_n(part->ata, part->disco, part->indice_partizione, settore, n_settori, (void *) cluster); 

	  if ( size > resto_cluster)  {  
	    memcpy((cluster+offset_cluster_first), buf,  resto_cluster); 
	    n_write+=resto_cluster;
	  //   flog(LOG_INFO, "Ho scritto  %d", n_write);
	    buf+=resto_cluster;
	  }else  {
	    memcpy((cluster+offset_cluster_first), buf, size);
	    n_write+=size;
	   //  flog(LOG_INFO, "Ho scritto  %d", n_write);
	    buf+=size; 
	  }
        // GESTIONE ULTIMO CLUSTER 
      } else if ( i==(n_load_cluster -1)) {  // ultimo 
	    read_part_n(part->ata, part->disco, part->indice_partizione, settore, n_settori, (void *) cluster); 
      	    memcpy(cluster, buf, offset_cluster_last);
	    buf+=offset_cluster_last; 
	    n_write+=offset_cluster_last; 
	    // flog(LOG_INFO, "Ho scritto  %d", n_write);
      }
      //GESTIONE RESTANTI CLUSTER 
	else { 
	  memcpy(cluster,buf, size_cluster_byte);
	  buf+=size_cluster_byte; 
	  n_write+=size_cluster_byte; 
	//   flog(LOG_INFO, "Ho scritto  %d", n_write);
	}
    
    write_part_n(part->ata, part->disco, part->indice_partizione, settore, n_settori, (void *) cluster);
    
    // prelevo il successivo devo crearlo se non presente  
    if (!(temp_chain= get_next_fat(fat, temp_chain)))  {
	mem_free(cluster);
	return FALSE; 
    }
    
    // significa che sono all'ultimo 
    if (temp_chain == EOC_32 && i < (n_load_cluster -1)) {
      if(!append_fat(label,chain, &temp_chain)) 
	return FALSE; 
    //  flog(LOG_INFO, "HO creato il cluste %d", temp_chain); 
    }
  
    
  }
  
  
  //flog(LOG_DEBUG, "Ho scritto  %d", n_write);
  
  
   if(n_write!=size) { 
    set_errno(1,"Errore quantità dati letti "); 
    mem_free(cluster);
    return FALSE; 
  }
  
  
  return TRUE; 
  
}



// funzione che legge buf dal volume label sulla catena chain 
/* label partizione 
 * chain catena che rapresenta un file 
 * offset posizioni all'interno della catena 
 * buf posizione nel quale inserire il buff 
 * grandezza del buffer 
 * la funzione deve finire quando sono stati caricati tutti i cluster nel buffer 
 * se cosi non fosse significa che è andato male qualcosa 
 * la funzione puo leggere meno byte di quelli passati 
 * se questo accade riporta il numero di byte letti e
 * se è zero suiamo alla fine 
*/ 

dword read_data(byte label , dword chain, lword offset,  byte * buf, dword size) { 
  
  TABELLA_VOLUMI * part=NULL; 
  dword size_cluster_byte=0; 
  dword  offset_cluster_first=0, offset_cluster_last=0, n_cluster=0, n_load_cluster=0; 
  dword i=0; 
  dword temp_chain=chain, settore=0, n_settori=0,resto_cluster=0;
  byte *cluster=NULL; 
  fat_ptr fat=NULL; 
  lword n_read=0; 
  
  if(chain <2 ) { 
      set_errno(1,"Bad Chain"); 
      return FALSE; 
  } 
  
  if ( buf == NULL) {
    set_errno(1,"Bad buffer"); 
    return FALSE; 
  }
  
  // non devo settare nessun errare 
  if (size <0 ) { 
     // set_errno("Bad Size"); 
      return FALSE; 
  } 
  
  if (!(part=get_volume(label)))
      return FALSE; 
  
  // grandezza cluster ( varia tra disco e disco) 
  size_cluster_byte=part->fat_info.sectors_for_cluster*part->fat_info.byts_for_sector; 
  fat=part->fat; 
  
  if(!(cluster=mem_alloc(size_cluster_byte,1))){
      set_errno(1,"Errore memoria"); 
	return FALSE; 
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
  n_load_cluster=((offset_cluster_first+size)/size_cluster_byte) + ((size%size_cluster_byte)?1:0); 
  // salvo il numero di settori di cui è composto un cluster 
  n_settori=part->fat_info.sectors_for_cluster;
 
  // devo verificare l'errore 
  // quando torna questo il file è terminato 
  for ( i=0; i< n_cluster; i++) { 
      if(!(temp_chain=get_next_fat(fat,temp_chain))){
	perror("lettura"); 
	return 0; 
      }
  }
 
#ifdef DEBUG_FS
  flog(LOG_DEBUG, "Primo cluster : %d", temp_chain);
  flog(LOG_DEBUG, "Size Cluster : %x", size_cluster_byte); 
  flog(LOG_DEBUG, "n_load_cluster : %d", n_load_cluster);    
  flog(LOG_DEBUG, "Offset first : %d", offset_cluster_first); 
  flog(LOG_DEBUG, "Resto : %d", resto_cluster); 
  flog(LOG_DEBUG, "Offset last : %d", offset_cluster_last); 
#endif
  
  //Su temp_chain ho il primo cluster coinvolto nella lettura    
  for (i=0; i< n_load_cluster; i++)  {
    
        // calcolo il settore poiché il driver lavora sui settori 
      if ( temp_chain !=0 && temp_chain != EOC_32) 
	settore=FirstDataSectorOfCluster(temp_chain, part->fat_info.sectors_for_cluster, part->fat_info.first_set_data); 
      else {
	    set_errno (1,"Si e provato a leggere un cluster non buono"); 
	    flog(LOG_WARN, "Read %d", n_read); 
	    mem_free(cluster);
	    return n_read;
      }
      
      // chiamo il driver
      read_part_n(part->ata, part->disco, part->indice_partizione, settore, n_settori, (void *) cluster); 
	 
      // il primo è l'ultimo vanno trattati in modo diverso 

      if (i==0) { // primo 

	  if ( size > resto_cluster) {
	    // carico parte del buffer richiesto  la seconda parte verrà caricata da un altra interazione 
	    memcpy(buf, (cluster+offset_cluster_first), resto_cluster); 
	    n_read+=resto_cluster;
	//    flog(LOG_INFO, "Ho letto :%d", n_read); 
	    buf+=resto_cluster;
	  }else  {
	    // carico tutto il buffer richiesto
	    memcpy(buf, (cluster+offset_cluster_first), size);
	    n_read+=size;
	//    flog(LOG_INFO, "Ho letto :%d", n_read); 
	    buf+=size; 
	  }
	    
      } else if ( i==(n_load_cluster -1)) {  // ultimo 
	    // coppio la parte rimanente 
	    memcpy(buf, cluster, offset_cluster_last);
	    n_read+=offset_cluster_last;
	//    flog(LOG_INFO, "Ho letto :%d", n_read); 
	    buf+=offset_cluster_last; 
      }
	else { //restanti 
	  memcpy(buf, cluster, size_cluster_byte);
	  buf+=size_cluster_byte;
	  n_read+=size_cluster_byte;
	//  flog(LOG_INFO, "Ho letto :%d", n_read); 
	}
      
    // prelevo il successivo 
    if (!(temp_chain= get_next_fat(fat, temp_chain)))  {
	mem_free(cluster);
	return FALSE; 
    }
    
  }
  
  //  flog(LOG_DEBUG, "HO letto %d", n_read); 
  
  if(n_read!=size) { 
    set_errno(1,"Errore quantità dati letti "); 
    mem_free(cluster);
    return FALSE; 
  }
  

  
  return n_read; 
  
}



/* funzione di test */ 

#define S 10000

BOOL test_read ( byte label) {
  
  dword chain = 0, i=0,t=0, j=0,n=0; 
  byte* buf=mem_alloc(S, 1); 
  byte* b= mem_alloc(S,1);
  byte nome[100]; 
  TABELLA_VOLUMI * tab=get_volume('C'); 
  lword off=0; 
  memset(buf, 1, S); 
  memset(b, 0, S); 
  dword temp=0; 
  
  flog(LOG_WARN, "Test DATA OBJECT "); 
 
  // creiamo un catena ( elementi 1 ) 
  if(!create_fat('C',&chain)) {
    perror("Creazione chain fallita"); 
    return FALSE; 
  } 
  
 // scrivo 10000 byte con offset 3000  usa 4 cluster 
 // quindi ne crea 3 
 // 1096 4096 4096 712
  if(!(write_data('C', chain, 3000, buf,S))) { 
    perror("Errore scrittura"); 
    delete_all_fat('C', chain);
    return FALSE; 
  }
  
  t=chain; 
    do {
	flog(LOG_DEBUG, "Elemento : %d", t);
	if(!(t=get_next_fat(tab->fat, t))) {
	  perror("Errore lettura ");
	  break; 
	}
    } while( !isLast_fat(t)); 
 
  
  
  if(!(read_data('C', chain,3000, b, S))) {
    perror("Errore lettura"); 
    delete_all_fat('C', chain);
    return FALSE;
  }
 
  for ( i=0; i< S; i++) 
      if ( buf[i]!=b[i]) {
	   flog(LOG_WARN, "Buffer diversi %d %x != %x" ,i,  buf[i], b[i]);  
	  break;
      }
      
  flog(LOG_WARN, "Buffer uguali"); 
  
 
  if(!(delete_all_fat('C',chain))) {
      perror("Eliminzazione chain fattila"); 
      return FALSE; 
  }
  
  
  
  
  if(!create_fat('C',&chain)) {
    perror("Creazione chain fallita"); 
    return FALSE; 
  } 
  

 
 
 // scrivo 10000 byte con offset 3000  usa 4 cluster 
 // 1096 4096 4096 712
  if(!(write_data('C', chain, 3000, buf,S))) { 
    perror("Errore scrittura"); 
    delete_all_fat('C', chain);
    return FALSE; 
  }
 
 
  if(!(read_data('C', chain,3000, b, S))) {
    perror("Errore lettura"); 
    delete_all_fat('C', chain);
    return FALSE;
  }
 
  for ( i=0; i< S; i++) 
      if ( buf[i]!=b[i]) {
	   flog(LOG_WARN, "Buffer diversi %d %x != %x" ,i,  buf[i], b[i]);  
	  break;
      }
      
  flog(LOG_WARN, "Buffer uguali"); 

  memset(buf, 1, S); 
  memset(b, 0, S); 
   
 // scrivo 10000 byte con offset 5000  usa 3 cluster 
 // non crea ulteriori cluster 
 // 3192 4096 2712
  if(!(write_data('C', chain, 5000, buf,S))) { 
    perror("Errore scrittura"); 
    delete_all_fat('C', chain);
    return FALSE; 
  }
 

  if(!(read_data('C', chain,5000, b, S))) {
    perror("Errore lettura"); 
    delete_all_fat('C', chain);
    return FALSE;
  }
 
  for ( i=0; i< S; i++) 
      if ( buf[i]!=b[i]) {
	   flog(LOG_WARN, "Buffer diversi %d %x != %x" ,i,  buf[i], b[i]);  
	  break;
      }
      
  flog(LOG_WARN, "Buffer uguali"); 
  
  // ingrandisco i buffer ma la catena rimane la stessa 
  mem_free(buf);
  mem_free(b);
  buf=mem_alloc(S+5000, 1); 
  b=mem_alloc(S+5000,1);
  memset(buf, 1, S+5000); 
  memset(b, 0, S+5000); 
  
   if(!(write_data('C', chain, 5000, buf,S+5000))) { 
    perror("Errore scrittura"); 
    delete_all_fat('C', chain);
    return FALSE; 
  }
 

  if(!(read_data('C', chain,5000, b, S+5000))) {
    perror("Errore lettura"); 
    delete_all_fat('C', chain);
    return FALSE;
  }
 
  for ( i=0; i< S+5000; i++) 
      if ( buf[i]!=b[i]) {
	   flog(LOG_WARN, "Buffer diversi %d %x != %x" ,i,  buf[i], b[i]);  
	  break;
      }
      
  flog(LOG_WARN, "Buffer uguali"); 
  
  
  // STAMPO LA CATENA 
  t=chain; 
    do {
	flog(LOG_DEBUG, "Elemento : %d", t);
	if(!(t=get_next_fat(tab->fat, t))) {
	  perror("Errore lettura ");
	  break; 
	}
    } while( !isLast_fat(t)); 
 
  
  
   if(!delete_all_fat('C', chain)) { 
      perror("Errore delete"); 
      return FALSE;
  }
  
  mem_free(buf);
  mem_free(b);
  
  /* FINE TEST GROSSI BUFFER */
  
  /* Test piccoli buffer subdoli */
   
  if(!create_fat('C',&chain)) {
    perror("Creazione chain fallita"); 
    return FALSE; 
  } 
  
  /*semplice test */
  
  memset(nome,'a', 100); 
  strncpy((char*)nome, "giuseppe",8);
      
   if(!(write_data('C', chain,0, nome,8))) { 
    perror("Errore scrittura"); 
    delete_all_fat('C', chain);
    return FALSE; 
  }

  memset(nome,0, 100); 
  if(!(read_data('C', chain,0, nome, 8))) {
    perror("Errore lettura"); 
    delete_all_fat('C', chain);
    return FALSE;
  }
   
  flog(LOG_DEBUG, "Nome : %s", nome); 
  
  /* test a cavallo di due cluster
   * uno deve essere creato  */
  
  memset(nome,'a', 100); 
  strncpy((char*)nome, "giuseppe",8);
      
   if(!(write_data('C', chain, 4094, nome,8))) { 
    perror("Errore scrittura"); 
    delete_all_fat('C', chain);
    return FALSE; 
  }
 
   memset(nome,0, 100); 
  if(!(read_data('C', chain,4094, nome, 8))) {
    perror("Errore lettura"); 
    delete_all_fat('C', chain);
    return FALSE;
  }
  
  flog(LOG_DEBUG, "Nome : %s", nome);  
  
  // altro test a cavallo di due cluster ma senza creazione di nuovi cluster 
   
  memset(nome,'a', 100); 
  strncpy((char*)nome, "giuseppe",8);
      
   if(!(write_data('C', chain, 4089, nome,8))) { 
    perror("Errore scrittura"); 
    delete_all_fat('C', chain);
    return FALSE; 
  }
   memset(nome,0, 100); 
  if(!(read_data('C', chain,4089, nome, 8))) {
    perror("Errore lettura"); 
    delete_all_fat('C', chain);
    return FALSE;
  }
  flog(LOG_DEBUG, "Nome : %s", nome);  
  
   
 if(!delete_all_fat('C', chain)) { 
      perror("Errore delete"); 
      return FALSE;
  }
 
  /*fine Test piccoli buffer */ 
  
  
  /***************************** test di scrittura e lettura  di 20MB *********
  buf=mem_alloc(S,1); 
  memset(buf, 'G', S);
  off=0;
 
  if(!create_fat('C',&chain)) {
    perror("Creazione chain fallita"); 
    return FALSE; 
  } 
  temp=chain; 
  
  flog(LOG_WARN, "Inizio scrittura 20 MB" ); 
  
  for ( i=0; i < 200 ; i++) {
	if(!write_data('C', chain, off,buf, S)) {
	  perror("Scrittura");
	  break;
	}
	off+=S; 
  }

  flog(LOG_DEBUG, "Ho scritto %d ", off); 
  flog(LOG_WARN, "Fine scrittura 20 MB "  ); 
  
  flog(LOG_WARN, "Inizio FAT 20 MB" ); 
  
  t=chain; j=0;
    do {
//	flog(LOG_DEBUG, "Elemento : %d", t);
	j++;
	if(!(t=get_next_fat(tab->fat, t))) {
	  perror("Errore lettura ");
	  break; 
	}
    } while( !isLast_fat(t)); 
   flog(LOG_DEBUG, "La FAT ha %d ", j); 
  flog(LOG_WARN, "Fine FAT 20 MB" ); 
  
  
  flog(LOG_WARN, "Inizio lettura 20 MB" ); 
 // flog(LOG_WARN, "Inizio lettura %d %d", chain, temp ); 
  off=0; 
  for ( i=0; i < 200 ; i++) {
	 memset(buf, 0, S);
	//flog(LOG_WARN, "%d", i); 
	 if(!read_data('C', chain, off,buf, S)) {
	  perror("Lettura");
	  break;
	}
	off+=S; 
	
      for( j=0; j < S ; j++) {
	if(buf[j]!='G') {
	    flog(LOG_WARN,"ERRORE LETTERA NON COINCIDENTE %d ",j); 
	    break;
	}
      }
    
}
  flog(LOG_DEBUG, "Ho letto %d ", off); 
  flog(LOG_WARN, "Fine lettura 20 MB" ); 
  
  if(!delete_all_fat('C', chain)) { 
      perror("Errore delete"); 
      return FALSE;
  }
  mem_free(buf); 
  / ***************************************fine test di scrittura di 20MB 
  
  // facciamo un po di test sui parametri 
  
   if(!create_fat('C',&chain)) {
    perror("Creazione chain fallita"); 
    return FALSE; 
  } 
  buf=mem_alloc(100,1);
  // la catena è formata da 1 cluster ma l'offset punta al secondo
  // questa funzione deve fallire
  
  // questa deve fallire 
   if(!read_data(label, chain,4096,buf,10)) 
     perror("READ"); 
  
   buf[0]='1'; 
   buf[1]='o'; 
   
     if(!write_data(label, chain, 4094 ,buf,2))
     perror("WRITE"); 
   
   
   // questa deve fallire 
   if(!read_data(label, chain,4094,buf,10)) 
     perror("READ"); 
   
   
  flog(LOG_WARN, "%s", buf); 
  
  memset(buf,0,100);
  strncpy((void*)buf,"giuseppe", 8);
   // questa deve avere successo 
  if(!write_data(label, chain, 4096 ,buf,10))
     perror("WRITE"); 
  // successo 
  if(!read_data(label, chain,4096,buf,10)) 
     perror("READ"); 
  
  flog(LOG_WARN, "%s", buf); 
  
  // queste devono fallire
  
  if(!write_data(label, chain, 15000,buf,10)) 
     perror("WRITE"); 
  
  if(!read_data(label, chain, 15000,buf,10)) 
     perror("READ"); 
  
  // le funzioni vengono bloccate dalla get_next che verifica
  // la consistenza della catena 
  
   // devo fare i test con i ciclci  
   mem_free(buf);
   buf=mem_alloc(10000,1);
   memset(buf,1,10000); 
   
   if(!write_data(label, chain, 0,buf,10000))
     perror("WRITE"); 
   
    memset(buf,0,10000); 
   j=0; 
   n=0; 
   
  
   while((n=read_data(label,chain,j,buf,1000))) {
    j+=n;
      flog(LOG_DEBUG, "Ho letto %d", n); 
      for(i=0;i<n;i++)
	if(buf[i]!=1) 
	  flog(LOG_WARN, "ERRORE"); 
   }
   
    
    if(!delete_all_fat('C', chain)) { 
      perror("Errore delete"); 
      return FALSE;
  }
  */
  
  
   while((n=read_data(label,chain,j,buf,1000))) {
    j+=n;
      flog(LOG_DEBUG, "Ho letto %d", n); 
      for(i=0;i<n;i++)
	if(buf[i]!=1) 
	  flog(LOG_WARN, "ERRORE"); 
   }
  
  
  
  return TRUE; 
  
}
