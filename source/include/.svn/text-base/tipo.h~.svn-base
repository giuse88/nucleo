#ifndef   TIPO_H
#define TIPO_H

typedef void* addr; // indirizzo virtuale e indirizzo fisico nello spazio di memoria
typedef unsigned short ioaddr; // indirizzo (fisico) nello spazio di I/O
typedef unsigned char natb;
typedef unsigned short natw;
typedef unsigned int  natl;
typedef void* str;
typedef const void* cstr;

// (* log di sistema
#define  LOG_MSG_SIZE   80
enum _log_sev_ { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERR };

typedef enum _log_sev_ log_sev; 


struct log_msg {
	 log_sev sev;
	natw identifier;
       char msg [LOG_MSG_SIZE + 1];
};
// *)

typedef enum ETH_ERR {
	RX_OK = 0,
	TX_OK = 0,
	RX_ERROR = 1,
	TX_ERROR = 1,
	RX_STATUS_ERROR_OR_WRONG_LEN = 2,
	RX_NOT_ENOUGH_SPACE = 3,
	SYSTEM_ERROR = 10
} ETH_ERR;

#ifdef WIN
	typedef long unsigned int size_t;
#else
	typedef unsigned int size_t;
#endif

#endif 
