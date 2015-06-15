#ifndef FS_H 
#define FS_H 


/*struttre che mi permettono l'astrazzione del file system*/ 


typedef struct _FCB_ FCB;

struct _FCB_{ 
  
  char name[260]; 
  
  //posizione data 
  byte volume; 
  dword cluster; 
  
  byte type;
  
  dword pos_corr; // posizione corrente   
  byte loock; // 
  byte mode;  // inutili in questo caso 

  dword semaphore; 

  // posizione entry 
  FCB * father;    // cluster della direcotory padre
  lword offset_father;          // offset in byte 
  
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

