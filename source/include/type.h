
#ifndef TYPE_MY
#define TYPE_MY
/*DEFINIZIONE DATI */ 
 
typedef unsigned char     byte; 
typedef unsigned short	  word; 
typedef unsigned int   	  dword;
typedef unsigned long     lword; 
typedef unsigned int  	   BOOL;  

typedef dword * dword_ptr;
typedef dword * fat_ptr;

#ifndef _WCHAR_
#define _WCHAR_
	typedef unsigned short wchar;
#endif

#define NULL 0 
#define TRUE  1 
#define FALSE 0

#include "tipo.h"

	
#endif 

