// ( costanti usate in sistema.cpp e sistema.S
//   SEL = selettori (segmentazione con modello flat)
//   LIV = livelli di privilegio
#define SEL_CODICE_SISTEMA	8
#define SEL_DATI_SISTEMA 	16
#define SEL_CODICE_UTENTE	27
#define SEL_DATI_UTENTE 	35
#define LIV_UTENTE		3
#define LIV_SISTEMA		0
// )

// ( varie dimensioni
#define MAX_SEM			4096	// num. seafori per ogni livello
#define DIM_PAGINA		4096U
#define DIM_MACROPAGINA		(DIM_PAGINA * 1024U)
#define DIM_DESP		216 	// descrittore di processo
#define DIM_DESS		8	// descrittore di semaforo
#define BYTE_SEM		(DIM_DESS * MAX_SEM)
#define MIN_ID			5
#define MAX_ID			8191
#define MAX_PRD			16
// )

// ( risoluzione video
#define VID_W			1280
#define VID_H			1024
#define VID_B			8
// )

// ( tipi interruzioni esterne
#define VETT_0			0xF0
#define VETT_1                  0xD0
#define VETT_2                  0xC0
#define VETT_3                  0xB0
#define VETT_4                  0xA0
#define VETT_5                  0x90
#define VETT_6                  0x80
#define VETT_7                  0x70
#define VETT_8                  0x60
#define VETT_9                  0x50
#define VETT_10                 0x40
#define VETT_11                 0x30
#define VETT_12                 0x20
#define VETT_13                 0xD1
#define VETT_14                 0xE0
#define VETT_15                 0xE1
#define VETT_16                 0xC1
#define VETT_17                 0xB1
#define VETT_18                 0xA1
#define VETT_19                 0x91
#define VETT_20                 0x81
#define VETT_21                 0x71
#define VETT_22                 0x61
#define VETT_23                 0x51
#define VETT_S			0x4F
// )

// ( tipi delle primitive
#define TIPO_A			0x42	// activate_p
#define TIPO_T			0x43	// terminate_p
#define TIPO_SI			0x44	// sem_ini
#define TIPO_W			0x45	// sem_wait
#define TIPO_S			0x46	// sem_signal
#define TIPO_D			0x49	// delay
#define TIPO_RL			0x4a	// *read_log
#define TIPO_RE			0x4b	// resident
#define TIPO_EP			0x4c	// end_program
#define TIPO_APE		0x52	// activate_pe
#define TIPO_WFI		0x53	// wfi
#define TIPO_FG			0x54	// *fill_gate
#define TIPO_P			0x55	// *panic
#define TIPO_AB			0x56	// *abort_p
#define TIPO_L			0x57	// *log
#define TIPO_TRA		0x58	// trasforma
#define TIPO_UD			0x59	// *udelay
#define TIPO_DM			0x5a	// *dump

#define TIPO_PALLOC		0x5b	// *phys_alloc
#define TIPO_PFREE		0x5c	// *phys_free

#define TIPO_GETID		0x5d	// *get_id
#define TIPO_GETIDP		0x5e	// *get_id_p

#define IO_TIPO_HDR		0x62	// readhd_n
#define IO_TIPO_HDW		0x63	// writehd_n


#define IO_TIPO_RSEN		0x72	// readse_n
#define IO_TIPO_RSELN		0x73	// readse_ln
#define IO_TIPO_WSEN		0x74	// writese_n
#define IO_TIPO_WSE0		0x75	// writese_0
#define IO_TIPO_RCON		0x76	// readconsole
#define IO_TIPO_WCON		0x77	// writeconsole
#define IO_TIPO_INIC		0x78	// iniconsole
// * in piu' rispetto al libro
// )

// (* estensioni rispetto al libro
#define IO_TIPO_PCIF		0x64	// pci_find
#define IO_TIPO_PCIR		0x65	// pci_read
#define IO_TIPO_PCIW		0x66	// pci_write
#define IO_TIPO_DMAHDR		0x67	// dmareadhd_n
#define IO_TIPO_DMAHDW		0x68	// dmawritehd_n

#define IO_TIPO_RKBD		0x82	// vkbd_read
#define IO_TIPO_IKBD		0x83	// vkbd_intr_enable
#define IO_TIPO_WFIKBD		0x84	// vkbd_wfi
#define IO_TIPO_SKBD		0x85	// vkbd_switch
#define IO_TIPO_SMON		0x86	// vmon_switch
#define IO_TIPO_WMON		0x87	// vmon_write_n
#define IO_TIPO_CMON		0x88	// vmon_setcursor
#define IO_TIPO_GMON		0x89	// vmon_getsize
#define IO_TIPO_LKBD		0x8a	// vkbd_leds
#define IO_TIPO_KMON		0x8b	// vmon_cursor_shape
#define IO_TIPO_PKBD		0x8c	// vkbd_send

//tipo per le interfacce di rete
#define	IO_TIPO_ETHT		0x8d	// eth_transmit
#define	IO_TIPO_ETHR		0x8e	// eth_receive
// tipi per es1370
#define IO_TIPO_AK_SET		0xE5	// setta porta nel mixer
#define IO_TIPO_AK_GET		0xE6	// legge porta del mixer

#define IO_TIPO_SNDCOMMAND	0xE7	// inizia l'esecuzione di un brano
#define IO_TIPO_PLAY		0xE8	// ferma temporaneamente l'esecuzione di un brano

#define IO_TIPO_GM		0x90	// graphic_mode
// *)

#define TIPO_GET_PART           0x95	// get_part
#define TIPO_WRITE_PART_N    	0x96	// write_part_n
#define TIPO_READ_PART_N        0x97 	// read_part_n
#define IO_TIPO_DATE            0x98	// get_date
#define IO_TIPO_TIME            0x99	// get_time
#define TIPO_IOSPACEREAD	0x9A	// *io_read_space
#define TIPO_IOSPACEWRITE	0x9B 	// *io_write_space
#define TIPO_IOSPACEINIT	0x9C   	// * init_space

// FS 

#define TIPO_OPEN 		0xA5 	//open
#define TIPO_CLOSE 		0xA6 	//close
#define TIPO_READ 		0xA7	//read
#define TIPO_WRITE              0xA8 	//write
#define TIPO_LSEEK		0xA9 	//lseek 
#define TIPO_REMOVE		0xAA	//unlink
#define TIPO_RMDIR 		0xAB	//rmdir
#define TIPO_CHDIR		0xAC	//chdir
#define TIPO_MKDIR		0xAD 	//mkdir
#define TIPO_GETCWD		0xAE	//get_cwd
#define TIPO_ERR		0xAF	// err 
#define TIPO_RENAME		0xB0 	// rename 
// ( suddivisione della memoria virtuale
//   NTAB = Numero di Tabelle delle pagine
//   SIS  = SIStema
//   MIO  = Modulo IO
//   VID  = memoria VIDeo
//   USR  = utente (USeR)
//   PCI  = dispositivi mappati in memoria
//   C    = condiviso
//   P    = privato
#define NTAB_SIS_C		256	// 1GiB
#define NTAB_SIS_P		1	// 4MiB
#define NTAB_MIO_C		248	// 1GiB - 4MiB - 8MiB - 20MiB
#define NTAB_VID_C		2	// 8MiB
#define NTAB_PCI_C		5	// 20MiB
#define NTAB_USR_C		256	// 1GiB
#define NTAB_USR_P		256	// 1GiB
// )

// ( tipi di dump
#define DUMP_IOAPIC		1
#define DUMP_PROC		2
#define DUMP_SEM		3
// )


// costanti per la scheda audio Ensoniq AudioPci 1370
#define BUFFERSIZE (64*1024)				// dimensione del buffer per l'esecuzione
									// corrisponde alla dimensione della
									// pagina fisica
#define HALFBUFFERSIZE (BUFFERSIZE>>1)
#define BUFFERALIGN 4				// allineamento del buffer obbligatorio


