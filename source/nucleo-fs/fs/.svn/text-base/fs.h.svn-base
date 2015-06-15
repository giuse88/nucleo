#ifndef FS_H 
#define FS_H 


/*struttre che mi permettono l'astrazzione del file system*/ 


typedef struct _FCB_ FCB;

struct _FCB_{ 
  
  char name[260]; 
  
  //posizione data 
  byte volume; 
  
  
  dword cluster; 	//primo cluster data 
  
  byte type;		// flag 
  
  dword pos_corr;   // posizione corrente   

  dword semaphore; 

  dword cluster_father; 
  lword offset_father;          // offset in byte nel padre 
  word  n_entry; 		// n_entry di cui è composto 
  
  
  // info fileSystem
  
  word CrtTime ;  // ora di creazione 
  word CrtDate; // data di creazione 
  word LstAccDate; // Ultimo accesso

  word FstClusHI; // parte alta indirizzo clusterFAT32 

  word WrtTime ;  // ora di scrittura 
  word WrtDate; // data discrittura

  dword size; 
  
  
  // posizione entry 
  FCB * father;    // cluster della direcotory padre
  
}; 


typedef struct _DIRECTORY_ DIRECTORY; 

struct _DIRECTORY_ { 

  char path[255]; 
  char name [255]; 

  byte dispositivo; // servono per essere generale  
  byte fileSystem;  // idem 

  byte loock; // 
  byte mode;  // inutili in questo caso 

  void * physic_des; // (puo puntare ad una cluster fat o un indoe ext ) 
  

}; 




#endif 

