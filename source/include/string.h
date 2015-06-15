#ifndef _STRING_H
#define _STRING_H

#include "type.h"
#include "stdarg.h" 

int isalpha(int c);
int UPPER(int c);
/*funzioni di conversione*/ 
void htostr(char *buf, natl l, int cifre);
void itostr(char *buf, unsigned int len, long l);
void htostr_upper(char *buf, natl l, int cifre);
/*gestione memoria*/  
void *memmove(void* dest, const void *src, size_t n);
void* memset(void* s, int c, size_t n);
int   memcmp(const void* s1, const void* s2, size_t n);
void* memcpy(void*   dest, const void*   src, size_t n);
void* memchr(const void*s, int c, size_t n); 

/*gestione stringhe*/
char *strcpy (char* dest, const char *);
char *strncpy (char *dest, const char *src, size_t );
int   strcmp(const char *s1, const char *s2);
int   strncmp(const char* s1, const char* s2, size_t n);
char *strcat(char*   dest, const char*   src) ;
char *strncat(char*   dest, const char*   src, size_t n);
size_t strlen(const char *s); 
size_t strnlen(const char *s,size_t maxlen);

char *strstr(const char *haystack, const char *needle) ;
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c); 

size_t strspn(const char *s, const char *_accept); 
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *_accept);

char *strsep(char **   stringp, const char *   delim);
char *strtok(char *   s, const char *   delim);
char *strtok_r(char *   s, const char *   delim, char **   ptrptr);

int strnicmp(const char *s1, const char *s2, size_t n);
int sprintf(char *dest,const char *format,...);
#endif
