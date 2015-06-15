#ifndef _STDARG_H_
#define _STDARG_H_

#include "type.h" 


#ifndef _VA_LIST_
#define _VA_LIST_ 
typedef char *va_list;
#endif 
// Versione semplificata delle macro per manipolare le liste di parametri
//  di lunghezza variabile; funziona solo se gli argomenti sono di
//  dimensione multipla di 4, ma e' sufficiente per le esigenze di printk.
//
#define va_start(ap, last_req) (ap = (char *)&(last_req) + sizeof(last_req))
#define va_arg(ap, type) ((ap) += sizeof(type), *(type *)((ap) - sizeof(type)))
#define va_end(ap)

int vsnprintf(char *str, natl size, const char *fmt, va_list ap);
int snprintf(char *buf, unsigned long n, const char *fmt, ...);

#endif
