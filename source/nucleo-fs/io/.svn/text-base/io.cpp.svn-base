//io.cpp
//
#include "costanti.h"
#include "tipo.h"
#include "es1370.h"
extern "C" {
#include "string.h" 
#include "stdarg.h" 
}
	// spiegazioni delle varie costanti/direttive al processore
	// e link alle specifiche della scheda audio/ mixer
	// sono contenute nel file


//#define BOCHS
////////////////////////////////////////////////////////////////////////////////
//    COSTANTI                                                                //
////////////////////////////////////////////////////////////////////////////////


const natl PRIO = 1000;
const natl LIV = LIV_SISTEMA;


////////////////////////////////////////////////////////////////////////////////
//                        CHIAMATE DI SISTEMA USATE                           //
////////////////////////////////////////////////////////////////////////////////

extern "C" natl activate_pe(void f(int), int a, natl prio, natl liv, natb type);
extern "C" void terminate_p();
extern "C" void sem_wait(natl sem);
extern "C" void sem_signal(natl sem);
extern "C" natl sem_ini(int val);
extern "C" natl activate_p(void f(int), int a, natl prio, natl liv);
extern "C" void wfi();	// [9.1]
extern "C" void abort_p();
extern "C" void writevid_n(int off, const unsigned char* vett, int quanti);
extern "C" void attrvid_n(int off, int quanti, unsigned char bg, unsigned char fg, bool blink);
extern "C" void log(log_sev sev, const char* buf, int quanti);
extern "C" addr graphic_mode();
extern "C" void* phys_alloc(natl dim, natl balign = 8);
extern "C" void phys_free(void* addr);
extern "C" addr trasforma(addr ind_virt);
extern "C" natl get_id();
extern "C" natl get_id_p();
extern "C" void* get_iostate(natl id);
extern "C" void set_iostate(natl id, void* state);

extern "C" natl pci_find(natl code, natw i);
extern "C" natl pci_read(natw l, natw regn, natl size);
extern "C" void pci_write(natw l, natw regn, natl res, natl size);

natw pci_loc(natb bus, natb dev, natb fun);
natb pci_get_bus(natw l);
natb pci_get_dev(natw l);
natb pci_get_fun(natw l);

//extern "C" natl le32toh(natl le);


// aggiunte rispetto a io.cpp standard:

extern "C" void udelay(natl usec);
	// il CODEC viene modificato scrivendo su un apposito registro;
	// i comandi verranno poi trasmessi serialmente al CODEC
	// in caso di CODEC occupato, è opportuno attendere un tempo di
	// 100 microsecondi, molto minore ai 50ms della semplice delay
	// preferiamo, quindi, l'attesa attiva

extern "C" bool resident(addr ind_virt, natl quanti, bool residente);
extern "C" void delay(natl n);


#define NULL 0

////////////////////////////////////////////////////////////////////////////////
//                      FUNZIONI GENERICHE DI SUPPORTO                        //
////////////////////////////////////////////////////////////////////////////////


// ingresso di un byte da una porta di IO
extern "C" void inputb(ioaddr reg, natb &a);

// uscita di un byte su una porta di IO
extern "C" void outputb(natb a, ioaddr reg);

// ingresso di una word da una porta di IO
extern "C" void inputw(ioaddr reg, natw &a);

// uscita di una word su una porta di IO
extern "C" void outputw(natw a, ioaddr reg);

// ingresso di una long da una porta di IO
extern "C" void inputl(ioaddr reg, natl &a);

// uscita di una long su una porta di IO
extern "C" void outputl(natl a, ioaddr reg);


extern "C" void flog(log_sev sev, const char* fmt, ...);
/*extern "C" void *memset(void *dest, int c, unsigned int n);
extern "C" void *memcpy(void *dest, const void *src, unsigned int n);
extern "C" natl strlen(const char *s); 
extern "C" char *strncpy(char *dest, const char *src, unsigned long l);
*/
extern "C" void* mem_alloc(natl dim, natl balign = 2);
extern "C" void mem_free(void *p);

////////////////////////////////////////////////////////////////////////////////
//                    GESTIONE DELLE INTERFACCE DI RETE                       //
////////////////////////////////////////////////////////////////////////////////

// Massimo numero di schede di rete
#define N_ETH 2

/*enum ETH_ERR {
	RX_OK = 0,
	TX_OK = 0,
	RX_ERROR = 1,
	TX_ERROR = 1,
	RX_STATUS_ERROR_OR_WRONG_LEN = 2,
	RX_NOT_ENOUGH_SPACE = 3,
	SYSTEM_ERROR = 10
};*/

//Struttura del descrittore d'interfaccia di rete
struct des_eth {
	bool presente;

	//Segistri della scheda di rete
	ioaddr CMD;		//8
	ioaddr MPC;		//32
	ioaddr IMR;		//16
	ioaddr ISR;		//16
	
	ioaddr RBSTART;	//32
	ioaddr CAPR;	//16
	ioaddr RCR;		//32
	
	ioaddr TSD0;	//32
	ioaddr TSAD0;	//32
	ioaddr TCR;		//32
	
	//Semafori di sincronizzazione e di mutua esclusione
	natl sincr;
	natl mutex;
	
	ETH_ERR tx_err; //Variabile per la propagazione dell'errore in trasmissione
	ETH_ERR rx_err; //Variabile per la propagazione dell'errore in ricezione
	
	natb* u_rx_buf; //Buffer (livello utente) ricezione
	natl u_rx_quanti;  //quanti bytes devo ricevere
	
	natb* u_tx_buf; //Buffer (livello utente) invio
	natl u_tx_quanti; //quanti bytes devo trasmettere
	
	//Piedino del controllore su cui arrivano le interruzioni, questo valore
	//verrà aggiornato in fase di inizializzazione della scheda leggendolo dallo
	//spazio di configurazione PCI.
	int irq;
};
extern "C" des_eth deseth[N_ETH];
natl eth_interf = 0;

//Funzione per il reset della scheda di rete
bool eth_reset(int interf) {
	des_eth* p = &deseth[interf];
		
	//Invio del comando di reset
	outputb(0x10, p->CMD);
	natb cmd;
	inputb(p->CMD, cmd);
	for (int i=0; i<1000; i++); //attesa rudimentale
	
	if ((cmd & 0x10) == 0x10) //if (RST == 1) => il reset non è ancora stato completato
		return false;

	return true;
}

//Funzione comune sia alla trasmissione che alla ricezione dei pacchetti:
//disabilita sia ricevitore che trasmettitore
//e maschera tutte le richieste di interruzione che la scheda può generare
void eth_halt(int interf) { 
	des_eth* p = &deseth[interf];
	
	//Azzeramento del Command Register
	outputb(0x00, p->CMD);
	
	//Mascheramento di tutte le interruzioni
	outputw(0x0000, p->IMR);
}

//Durante la fase di probing si effettuano interrogazioni nello spazio di
//configurazione PCI per individuare l'indirizzo base di I/O della scheda
//al quale si sommano gli offset dei registri, e su quale piedino del
//controllore di interruzioneessa essa presenta le richieste di interruzione.
void eth_probe(natl interf, natw l) {
	des_eth* p = &deseth[interf];

	//Lettura dell'indirizzo di I/O base (dal datasheet)
	ioaddr base = (ioaddr) pci_read(l, 0x10, 2);
	base &= ~0x1;
	
	//Inizializzazione del descrittore dell'interfaccia di rete.
	//Non importa che vengano sporcati i campi relativi ad esempio ai semafori
	//od altro, perché la loro inizializzazione verrà fatta in un secondo momento.
	ioaddr* x = (ioaddr*) p;
	
	//La dimensione in word del descrittore di interfaccia è data dalla dimensione
	//totale del descrittore in bytes divisa per la dimensione di una word.
	int wsize = sizeof(des_eth)/(sizeof(natw));
	
	for (int i = 0; i < wsize; i++) {
		*x += base;
		x++;
	}

	//Lettura del piedino del PIC su cui sono generate le richieste d'interruzione
	p->irq = (int) pci_read(l, 0x3c, 1);

	flog(LOG_INFO, "eth%d: I/O base = %x, irq=%d", interf, base, p->irq);

}

//Inizializzazione "materiale" della scheda.
void eth_if_init(natl interf) {
	des_eth* p = &deseth[interf];

	eth_halt(interf); //serve a stoppare richieste pendenti precedenti

	eth_reset(interf); //reset dell'interfaccia
	
	//Inizializzazione del registro dell'indirizzo (fisico) di trasferimento.
	//
	//In realtà esistono 4 registri diversi di questo tipo, per semplicità
	//ci riferiremo sempre e solo ad uno di questi (TSAD0). In fase di
	//inizializzazione lo settiamo a 0x00000000, quando vorremo trasmettere
	//ci dovremo preoccupare di settarlo con un opportuno indirizzo (fisico)
	//di un pacchetto.
	outputl(0x00000000, p->TSAD0);
	//inizializzazione (a NULL) del puntatore del buffer in trasmissione
	p->u_tx_buf = (natb*)0xffffffff;
	p->u_tx_quanti = 0;
	
	//Inizializzazione del registro per la ricezione.
	//
	//In fase di inizializzazione lo settiamo a 0x00000000 quando vorremo
	//ricevere ci dovremo preoccupare di settarlo con un opportuno indirizzo
	//(fisico) di un'area di memoria adibita alla ricezione.
	outputl(0x00000000, p->RBSTART);
	//inizializzazione (a NULL) del puntatore del buffer in ricezione
	p->u_rx_buf = (natb*)0xffffffff;
	p->u_rx_quanti = 0;
	
	//La scheda dovrebbe inizializzare anche i registri CBR e CAPR, per sicurezza
	//lo facciamo qui con i valori di default suggeriti dai datasheets.
	//
	//Noi non useremo la predisposizione della scheda ad usare un buffer ring,
	//perciò non ci preoccuperemo di utilizzare il registro CBR che sostanzialmente
	//punta, relativamente all'indirizzo contenuto in RBSTART, all'indirizzo di
	//memoria dal quale in poi la scheda può aggiungere pacchetti ricevuti.
	//
	//CAPR: Current Address of Packet Read, contiente l'indirizzo corrente
	//del pacchetto letto.
	//
	//In realtà dato che sono words, sono impropriamente chiamati indirizzi,
	//sarebbe più opportuno chiamarli offsets a partire dall'inidirizzo
	//contenuto nel registro RBSTART.
	outputw(0xfff0, p->CAPR);
		
	//Inizializzazione del registro TCR (Transmit Configuration Register).
	//I parametri che si vanno ad inizializzare sono 2:
	//
	//TCR[25:24] = IFG (InterFrame Gap) -
	//Any value other than 11 will violate the IEEE 802.3 standard.
	//
	//TCR[10:8] = MXDMA -
	//This field sets the maximum size of transmit DMA.
	//Un paper non ufficiale consiglia di impostare la configurazione
	//110 per questo campo che corrisponde a 1024 bytes per ciclo di burst.
	//
	outputl(0x03000600, p->TCR);
	
	//Inizializzazione del registro RCR (Receive Configuration Register)
	//
	//si inizializzano 3 parametri:
	//
	//RCR[15:13] = RXFTH (Rx Fifo THreshold) -
	//noi lo manterremo sempre settato a 111 che indica alla scheda
	//di trasferire nel buffer in memoria, solo quando un pacchetto completo
	//è entrato nel buffer FIFO
	//
	//RCR[12:11] = RBLEN (Rx Buffer LENgth) -
	//This field indicates the size of the Rx ring buffer.
	//Dato che non sfruttiamo i vantaggi offerti dal buffer circolare, e
	//sapendo che un frame ethernet è grande circa 1600 bytes, lo manterremo
	//a 00 che sta a significare
	//8 kbytes + 16 bytes
	//
	//RCR[10:8] = MXDMA -
	//This field sets the maximum size of receive DMA.
	//Noi lo manterremo settato a 111 che indica "il massimo possibile"
	//
	//Occorre inoltr configurare anche i tipi di pacchetti che si vogliono
	//ricevere. Si fa ciò settando dei determinati bits del primo byte di
	//RCR. In particolare mettendo tale byte a 0x0e si accettano pacchetti
	//corretti aventi indirizzi broadcast, multicast e singlecast
	outputl(0x0000e70e, p->RCR);
	
	//Abilitazione alla generazione di interruzioni da parte dell'interfaccia.
	//TER -> 1: Transmit Error, TOK -> 1: Transmit OK
	//RER -> 1: Receive Error, ROK -> 1: Receive OK
	outputw(0x000f, p->IMR);
	
	//Avvio del processo di trasmissione/ricezione.
	//si ottiene azzerando il registro per il conteggio dei pacchetti
	//persi MPC (Missed Packets Counter)
	outputl(0x00000000, p->MPC);
	
	//Abilitazione ricevitore e trasmettitore
	// TE -> 1, RE -> 1;
	outputb(0x0c, p->CMD);
}

void eth_go(des_eth* p) {
	//Abilitazione alla generazione di interruzioni da parte dell'interfaccia.
	//TER -> 1: Transmit Error, TOK -> 1: Transmit OK
	//RER -> 1: Receive Error, ROK -> 1: Receive OK
	//
	natw imr;
	inputw(p->IMR, imr);
	imr |= 0x000f;
	outputw(imr, p->IMR);
	
	//A questo punto l'interfaccia dovrebbe generare un'interruzione non
	//appena il pacchetto sarà traferito.
}

void eth_start_transmit(des_eth* po, natb vetto[], natl quanti) {
	po->u_tx_buf = vetto;
	po->u_tx_quanti = quanti;
	
	//Calcolo indirizzo fisico
	addr ptx_buf = trasforma(po->u_tx_buf);
	
	//Trasferimento indirizzo fisico del pacchetto in TSAD0
	outputl((natl)ptx_buf, po->TSAD0);
	
	natl tsd0;
	inputl(po->TSD0, tsd0);
	tsd0 &= 0xffffd000; // TSD0[13] = OWN -> 0 e TSD0[12:0] = SIZE -> 0
	//Con questa configurazione di bit si abilita il trasferimento del
	//pacchetto dalla memoria fisica al buffer FIFO della scheda
	//
	//Aggiornamento del campo SIZE
	tsd0 |= po->u_tx_quanti;
	//
	//Aggiornamento di TSD0
	outputl(tsd0, po->TSD0);
	
	//Per precauzione in questa fase abilitiamo anche la trasmissione
	natb cmd;
	inputb(po->CMD, cmd);
	cmd |= 0x04;
	outputb(cmd, po->CMD);

	eth_go(po);
}

extern "C" void c_eth_transmit(natl eth_if, natb uvetto[], natl uquanti, ETH_ERR& tx_err) {
	des_eth* po;

	if (eth_if >= N_ETH) {
		flog(LOG_WARN, "eth_if non valido: %d", eth_if);
		abort_p();
	}

	po = &deseth[eth_if];

	if (!po->presente) {
		flog(LOG_WARN, "eth non presente");
		abort_p();
	}

	sem_wait(po->mutex);
	eth_start_transmit(po, uvetto, uquanti);
	sem_wait(po->sincr);
	//qui sappiamo che il processo esterno ha aggiornato il campo
	//tx_err del descrittore di interfaccia per cui possiamo
	//propagarlo a livello utente.
	tx_err = po->tx_err;
	sem_signal(po->mutex);
}

void eth_start_receive(des_eth* pi, natb vetti[], natl quanti) {
	pi->u_rx_buf = vetti;
	pi->u_rx_quanti = quanti;
	
	//calcolo indirizzo fisico di ricezione
	addr prx_buf = trasforma(pi->u_rx_buf);
	
	//inizializzazione del registro appropriato nella scheda
	outputl((natl)prx_buf, pi->RBSTART);
	outputl(0xfff0, pi->CAPR);
	
	//Per precauzione in questa fase abilitiamo anche la ricezione
	natb cmd;
	inputb(pi->CMD, cmd);
	cmd |= 0x08;
	outputb(cmd, pi->CMD);
	
	eth_go(pi);
}

extern "C" void c_eth_receive(natl eth_if, natb uvetti[], natl uquanti, ETH_ERR& rx_err) {
	des_eth* pi;

	if (eth_if >= N_ETH) {
		flog(LOG_WARN, "eth_if non valido: %d", eth_if);
		abort_p();
	}

	pi = &deseth[eth_if];

	if (!pi->presente) {
		flog(LOG_WARN, "eth non presente");
		abort_p();
	}

	sem_wait(pi->mutex);
	eth_start_receive(pi, uvetti, uquanti);
	sem_wait(pi->sincr);
	//qui sappiamo che il processo esterno ha aggiornato il campo
	//rx_err del descrittore di interfaccia per cui possiamo
	//propagarlo a livello utente.
	rx_err = pi->rx_err;
	sem_signal(pi->mutex);
}

void ext_eth(int h) {
	des_eth* p = &deseth[h];
	natw isr;
	
	for (;;) {
		inputw(p->ISR, isr);

		if (isr == 0)
			goto out;
		
		if (isr == 0xffff)
			goto safe_break;
		
		outputw(isr, p->ISR);
		
		if ((isr & 0xc07f) == 0)
			goto safe_break;
		
		if (isr & 0x0001) {
			//se (ROK == 1) vuol dire che un pacchetto è stato ricevuto correttamente.
			p->rx_err = RX_OK; // 0
			
			//andiamo a leggere il Command Register per capire se il buffer
			//è pieno o vuoto.
			natb cmd;
			inputb(p->CMD, cmd);
			while ((cmd & 0x01) == 0) {
				//mentre (BUFE == 0), cioè mentre il buffer non è vuoto,
				//cerchiamo di prendere i pacchetti.
				
				//Offset di estrazione dalla coda
				natw capr;
				inputw(p->CAPR, capr);
				capr += 16;
				
				//Ogni pacchetto è preceduto da un header composto da una
				//sezione detta packet status e da un'altra detta packet length
				//Lo header è lungo una parola lunga.
				natl rx_header = *(natl*)p->u_rx_buf;
				//flog(LOG_ERR, "capr = %x, rx_header = %x", capr, rx_header);
				
				
				//adesso è possibile leggere la lunghezza del pacchetto
				//shiftando a destra di 16 rx_status
				natw packet_len = rx_header >> 16;
				
				//l'utente ha allocato poco spazio. dev'essere allocato più spazio.
				if (p->u_rx_quanti < (packet_len + 8U)) {
					p->rx_err = RX_NOT_ENOUGH_SPACE; // 3
					goto safe_break;
				}

				if (!(rx_header & 0x00000001) || //errore nella ricezione (ROK == 0)
					packet_len < 64 || //pacchetto minore della lunghezza minimina di un frame ethernet
					packet_len > (1518 + 4) //pacchetto maggiore della lunghezza del frame ethernet + 4 per la vlan encap.
					) { //vuol dire che c'è un errore
					p->rx_err = RX_STATUS_ERROR_OR_WRONG_LEN; // 2
					goto safe_break; //esci dal ciclo con errore
				}
				//non è stato rilevato nessun errore
				//si passa alla ricezione del pacchetto
				
				capr += 4;
				//per finire capr viene allineato alla parola lunga
				capr = (capr + 3) & ~3;
				//e quindi si aggiorna il suo valore anche sulla scheda
				outputw(capr - 16, p->CAPR);
				inputb(p->CMD, cmd);
			}

			//riportiamo a 0 ROK per far capire alla scheda che abbiamo
			isr &= 0xfffe; //finito di gestire l'interruzione
			outputw(isr, p->ISR);
		}
		
		if (isr & 0x0002) { //(RER == 1)
			//se (RER == 1) vuol dire che abbiamo avuto degli errori nella ricezione
			p->rx_err = RX_ERROR; // 1
			
			//riportiamo a 0 RER per far capire alla scheda che abbiamo
			isr &= 0xfffd; //finito di gestire l'interruzione
			outputw(isr, p->ISR);
		}
		
		if (isr & 0x0004) { //(TOK == 1)
			//se (TOK == 1) vuol dire che il trasferimento è stato completato con successo
			//per cui si liberano le strutture per l'immagazzinamento del pacchetto
			p->tx_err = TX_OK; // 0

			//riportiamo a 0 TOK per far capire alla scheda che abbiamo
			isr &= 0xfffc; //finito di gestire l'interruzione
			outputw(isr, p->ISR);
		}
		
		if (isr & 0x0008) { //(TER == 1)	
			//se (TER == 1) vuol dire che abbiamo avuto degli errori nel
			//trasferimento. In maniera semplificata si ipotizza che il motivo
			//del mancato trasferimento, sia dovuto ad un'errata soglia di
			//trasferimento.
			p->tx_err = TX_ERROR; // 1
			
			//preleviamo la vecchia soglia TSD0[21:16] = ERTXTH
			natl tsd0;
			inputl(p->TSD0, tsd0);
			natl ertxth = (tsd0 & 0x003f0000) >> 11; //preleviamo il valore di
			//ertxth shiftando a destra di 16 tsd0 & 0x003f0000, ogni unità di
			//tale campo è considerata moltiplicato per 32 bytes per cui bisognerebbe
			//shiftare a sinistra di 5 per ottenere il corretto valore rappresentato.
			// 16 - 5 = 11.
			//
			//Adesso si può leggere il vecchio valore della soglia in bytes.
			//Tale campo ha un valore massimo raggiungibile che è pari a
			//2048 bytes, perciò nell'andare ad aumentarlo dobbiamo fare attenzione
			//a non eccedere tale "upper-bound".
			if (ertxth < 2016)
				ertxth += 32;
			tsd0 = (tsd0 & 0xffc0ffff) | (ertxth << 11);
			outputl(tsd0, p->TSD0);
				
			//riportiamo a 0 TER per far capire alla scheda che abbiamo
			isr &= 0xfff7; //finito di gestire l'interruzione
			outputw(isr, p->ISR);
		}
		
		if (isr & 0x8000) { //(SERR == 1)
			//(isr == 0x8000) vuol dire che la scheda va resettata
			p->tx_err = SYSTEM_ERROR; // 10
			p->rx_err = SYSTEM_ERROR; // 10
			eth_reset(h);
			eth_if_init(h);
		}

		if (isr == 0)
safe_break:
			//se si esce con il "safe break" si stoppa la scheda.
			eth_halt(h);

		sem_signal(p->sincr);
out:
		wfi();
	}
	
}

void eth_init(natw l) {

	if (eth_interf >= N_ETH) {
		flog(LOG_WARN, "pci %2x.%2x.%2x: troppe schede di rete",
				pci_get_bus(l), pci_get_dev(l), pci_get_fun(l));
		return;
	}
	natl i = eth_interf;

	des_eth* p = &deseth[i];
	
	p->presente = true;

	//Fase Probe.
	//
	//In questa fase principalmente vengono "scoperti":
	//
	//il numero del piedino del controllore di interruzione su cui
	//arrivano le richieste di interruzione da parte dell'interfaccia
	//
	//l'indirizzo base di I/O a partire dal quale iniziano i registri
	//della nostra interfaccia di rete.
	eth_probe(i, l);

	//Adesso inizializziamo i semafori. Lo facciamo dopo la Fase Probe,
	//perché essa va a "sporcare" il descrittore di interfaccia e quindi
	//anche i valori dei nostri semafori.
	
	//Inizializzazione semaforo di mutua esclusione
	if ((p->mutex = sem_ini(1)) == 0) {
		flog(LOG_ERR, "eth%d: impossibile creare mutex", i);
		p->presente = false;
		return;
	}
	
	//Inizializzazione semaforo di sincronizzazione
	if ((p->sincr = sem_ini(0)) == 0) {
		flog(LOG_ERR, "eth%d: impossibile creare sincr", i);
		p->presente = false;
		return;
	}
	
	//Fase Attach.
	//
	//Sostanzialmente consiste nel reset dell'interfaccia.
	bool eth_rst;
	
	//il reset va sempre protetto in mutua esclusione!
	sem_wait(p->mutex);
	eth_rst = eth_reset(i);
	sem_signal(p->mutex);
	
	if (!eth_rst) {
		flog(LOG_ERR, "eth%d: impossibile resettare l'interfaccia di rete!", i);
		p->presente = false;
		return;
	}
	flog(LOG_INFO, "eth%d: reset dell'interfaccia di rete completato.", i);

	//Fase Init
	sem_wait(p->mutex);
	eth_if_init(i);
	sem_signal(p->mutex);
	
	//Inizializziamo i campi di propagazione a livello superiore dell'errore
	p->rx_err = RX_OK; // 0
	p->tx_err = TX_OK; // 0
	
	//Attivazione del processo esterno
	//Ricordiamoci che p->irq è stato inizializzato durante la Fase Probe
	natl id = activate_pe(ext_eth, 0, PRIO, LIV_SISTEMA, p->irq);
	if (id == 0xFFFFFFFF) {
		flog(LOG_ERR, "eth%d: impossibile creare proc. esterno", i);
		p->presente = false;
		return;
	}
	eth_interf++;
}


////////////////////////////////////////////////////////////////////////////////
//                    GESTIONE DELLE INTERFACCE SERIALI [9.2]                 //
////////////////////////////////////////////////////////////////////////////////

enum funz { input_n, input_ln, output_n, output_0 };  // [9.2]

struct interfse_reg {	// [9.2]
	ioaddr iRBR, iTHR, iLSR, iIER, iIIR;
};

struct des_se {		// [9.2]
	interfse_reg indreg;
	natl mutex;
	natl sincr;
	natl cont;
	addr punt;
	funz funzione;
	natb stato;
};

const natl S = 2;
extern "C" des_se com[S];	// [9.2]

void input_com(des_se* p_des);	// [9.2]
void output_com(des_se* p_des);	// [9.2]
void estern_com(int i) // [9.2]
{
	natb r;
	des_se *p_des;
	p_des = &com[i];
	for(;;) {
		inputb(p_des->indreg.iIIR, r);
		if ((r&0x06) == 0x04) 
			input_com(p_des);
		else if ((r&0x06) == 0x02)
			output_com(p_des);
		wfi();
	}
}

void startse_in(des_se *p_des, natb vetti[], natl quanti, funz op); // [9.2.1]
extern "C" void c_readse_n(natl serial, natb vetti[], natl quanti, natb& errore) // [9.2.1]
{
	des_se *p_des;

	// (* le primitive non devono mai fidarsi dei parametri
	if (serial >= S) {
		flog(LOG_WARN, "readse_n con serial=%d", serial);
		abort_p();
	}
	// *)

	p_des = &com[serial];
	sem_wait(p_des->mutex);
	startse_in(p_des, vetti, quanti, input_n);
	sem_wait(p_des->sincr);
	errore = p_des->stato;
	sem_signal(p_des->mutex);
}

extern "C" void c_readse_ln(natl serial, natb vetti[], int& quanti, natb& errore)
{
	des_se *p_des;

	// (* le primitive non devono mai fidarsi dei parametri
	if (serial >= S) {
		flog(LOG_WARN, "readse_ln con serial=%d", serial);
		abort_p();
	}
	// *)

	p_des = &com[serial];
	sem_wait(p_des->mutex);
	startse_in(p_des, vetti, 80, input_ln);
	sem_wait(p_des->sincr);
	quanti = p_des->cont;
	errore = p_des->stato;
	sem_signal(p_des->mutex);
}

extern "C" void go_inputse(ioaddr i_ctr);
void startse_in(des_se *p_des, natb vetti[], natl quanti, funz op) // [9.2.1]
{
	p_des->cont = quanti;
	p_des->punt = vetti;
	p_des->funzione = op;
	go_inputse(p_des->indreg.iIER);
}

extern "C" void halt_inputse(ioaddr i_ctr);
void input_com(des_se *p_des) // [9.2.1]
{
	natb c; bool fine;
	fine = false;

	halt_inputse(p_des->indreg.iIER);

	inputb(p_des->indreg.iLSR, c);

	p_des->stato = c & 0x1e;
	if (p_des->stato != 0)
		fine = true;
	else {
		inputb(p_des->indreg.iRBR, c);
		if (p_des->funzione == input_n) {
			*static_cast<natb*>(p_des->punt) = c; // memorizzazione
			p_des->punt = static_cast<natb*>(p_des->punt) + 1;
			p_des->cont--;
			if(p_des->cont == 0)
				fine = true;
		} else {
			if (p_des->funzione == input_ln) {
				if(c == '\r' || c == '\n') {
					fine = true;
					p_des->cont = 80 - p_des->cont;
				} else {
					*static_cast<natb*>(p_des->punt) = c; // memorizzazione
					p_des->punt = static_cast<natb*>(p_des->punt) + 1;
					p_des->cont--;
					if (p_des->cont == 0) {
						fine = true;
						p_des->cont = 80;
					}
				}
			}
		}
	}

	if(fine == true) {
		*static_cast<natb*>(p_des->punt) = 0;	// carattere nullo
		sem_signal(p_des->sincr);
	} else
		go_inputse(p_des->indreg.iIER);
}

void startse_out(des_se *p_des, natb vetto[], natl quanti, funz op);
extern "C" void c_writese_n(natl serial, natb vetto[], natl quanti)	// [9.2.2]
{
	des_se *p_des;

	// (* le primitive non devono mai fidarsi dei parametri
	if (serial >= S) {
		flog(LOG_WARN, "writese_n con serial=%d", serial);
		abort_p();
	}
	// *)

	p_des = &com[serial];
	sem_wait(p_des->mutex);
	startse_out(p_des, vetto, quanti, output_n);
	sem_wait(p_des->sincr);
	sem_signal(p_des->mutex);
}

extern "C" void c_writese_0(natl serial, natb vetto[], natl &quanti)
{
	des_se *p_des;

	// (* le primitive non devono mai fidarsi dei parametri
	if (serial >= S) {
		flog(LOG_WARN, "writese_0 con serial=%d", serial);
		abort_p();
	}
	// *)

	p_des = &com[serial];
	sem_wait(p_des->mutex);
	startse_out(p_des, vetto, 0, output_0);
	sem_wait(p_des->sincr);
	quanti = p_des->cont;
	sem_signal(p_des->mutex);
}

extern "C" void go_outputse(ioaddr i_ctr);
void startse_out(des_se *p_des, natb vetto[], natl quanti, funz op) // [9.2.2]
{
	p_des->cont = quanti;
	p_des->punt = vetto;
	p_des->funzione = op;
	go_outputse(p_des->indreg.iIER);
	output_com(p_des); 
}

extern "C" void halt_outputse(ioaddr i_ctr);
void output_com(des_se *p_des)	// [9.2.2]
{
	natb c; bool fine;
        fine = false;

	if (p_des->funzione == output_n) {
		p_des->cont--;
		if(p_des->cont == 0) {
			fine = true;
			halt_outputse(p_des->indreg.iIER);
		}
		c = *static_cast<natb*>(p_des->punt); //prelievo
		outputb(c, p_des->indreg.iTHR);
		p_des->punt = static_cast<natb*>(p_des->punt) + 1; 
	} else if (p_des->funzione == output_0) {
		c = *static_cast<natb*>(p_des->punt); //prelievo
		if (c == 0) {
			fine = true;
			halt_outputse(p_des->indreg.iIER);
		} else {
			outputb(c, p_des->indreg.iTHR);
			p_des->cont++;
			p_des->punt = static_cast<natb*>(p_des->punt) + 1; 
		}
	}

	if (fine == true)
		sem_signal(p_des->sincr);

}

// ( inizializzazione delle interfacce seriali
extern "C" void com_setup(void);	// vedi "io.S"
// interruzioni hardware delle interfacce seriali
int com_irq[S] = { 4, 3 };

bool com_init()
{
	des_se *p_des;
	natl id;
	natl i, com_base_prio = PRIO;

	com_setup();

	for(i = 0; i < S; ++i) {
		p_des = &com[i];

		if ( (p_des->mutex = sem_ini(1)) == 0xFFFFFFFF) {
			flog(LOG_ERR, "com: impossibile creare mutex");
			return false;
		}
		if ( (p_des->sincr = sem_ini(0)) == 0xFFFFFFFF) {
			flog(LOG_ERR, "com: impossibile creare sincr");
			return false;
		}

		id = activate_pe(estern_com, i, com_base_prio - i, LIV, com_irq[i]);
		if (id == 0xFFFFFFFF) {
			flog(LOG_ERR, "com: impossibile creare proc. esterno");
			return false;
		}

	}
	flog(LOG_INFO, "com: inizializzate %d seriali", S);
	return true;
}
// )


////////////////////////////////////////////////////////////////////////////////
//                         GESTIONE DELLA TASTIERA                            //
////////////////////////////////////////////////////////////////////////////////


struct des_kbd {
	bool escape;
	int pause;
	ioaddr iRBR;
	natl id;
} kbd;

natb pausecode[6] = { 0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5 };

natw kbd_read()
{
	natb c;

	inputb(kbd.iRBR, c);

	if (c == pausecode[kbd.pause]) {
		if (++kbd.pause >= 6) {
			kbd.pause = 0;
			return 0xe17f;
		}
		return 0;
	} else {
		kbd.pause = 0;
	}

	if (c == 0xe0) {
		kbd.escape = true;
		return 0;
	}
	
	if (kbd.escape) {
		kbd.escape = false;
		return 0xe000 | c;
	}

	return c;
}

// smistatore dei caratteri dalla tastiera reale alle tastiere virtuali
void vkbd_putchar(natw code);
void estern_kbd(int h)
{
	natw code;

	for(;;) {
		code = kbd_read();
		if (code) 
			vkbd_putchar(code);				
		wfi();
	}
}
// Interruzione hardware della tastiera
const int KBD_IRQ = 1;

bool kbd_init()
{

	kbd.escape = false;
	kbd.pause = 0;
	kbd.iRBR = (ioaddr)0x60;

	if ( (kbd.id = activate_pe(estern_kbd, 0, PRIO, LIV, KBD_IRQ)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "kbd: impossibile creare estern_kbd");
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//    TASTIERE VIRTUALI                                                      //
///////////////////////////////////////////////////////////////////////////////

// sottrazione modulo 'mod' (il '%' del C/C++ calcola il valore sbagliato)
// p1 e p2 devono essere compresi tra 0 e mod-1
inline int circular_sub(int p1, int p2, int mod)
{
	int dist = p1 - p2;
	if (dist < 0) dist += mod;
	return dist;
}

// somma modulo 'mod'
inline int circular_sum(int p1, int p2, int mod)
{
	return (p1 + p2) % mod;
}



const natl VKBD_BUF_SIZE = 20;
const int MAX_VKBD = 5;
const int VKBD_CONSOLE = MAX_VKBD - 1;

struct des_vkbd {
	natl mutex;
	natl intr;
	bool intr_enabled;
	bool intr_pending;
	int intr_waiting;
	natw buf[VKBD_BUF_SIZE];
	natl first;
	natl last;
	natl nchar;
	natb leds;
};

des_vkbd array_desvkbd[MAX_VKBD];
des_vkbd *vkbd_active;

bool vkbd_init()
{
	for (int i = 0; i < MAX_VKBD; i++) {
		des_vkbd *k = &array_desvkbd[i];

		if ( (k->mutex = sem_ini(1)) == 0xFFFFFFFF) {
			flog(LOG_ERR, "vkbd: impossibile creare mutex");
			return false;
		}
		if ( (k->intr = sem_ini(0)) == 0xFFFFFFFF) {
			flog(LOG_ERR, "vkbd: impossibile creare intr");
			return false;
		}
		k->intr_enabled = false;
		k->intr_pending = false;
		k->intr_waiting = 0;
		k->first = k->last = k->nchar = 0;
	}
	flog(LOG_INFO, "vkbd: inizializzate %d tastiere virtuali", MAX_VKBD);
	return true;
}

extern "C" void c_vkbd_wfi(int v)
{
	if (v >= MAX_VKBD) {
		flog(LOG_WARN, "(wfi) vkbd inesistente: %d", v);
		abort_p();
	}

	des_vkbd *k = &array_desvkbd[v];

	sem_wait(k->mutex);
	while (!k->intr_pending) {
		k->intr_waiting++;
		sem_signal(k->mutex);
		sem_wait(k->intr);
		sem_wait(k->mutex);
	}
	sem_signal(k->mutex);
}


extern "C" void c_vkbd_switch(int v);
extern "C" void c_vmon_switch(natl v);
void vkbd_nuovo_car(des_vkbd* k, natw code, bool clear = false)
{
	sem_wait(k->mutex);

	if (!clear && k->nchar >= VKBD_BUF_SIZE)
		goto out;

	if (clear) 
		k->first = k->last = k->nchar = 0;

	k->buf[k->last] = code;
	k->last = circular_sum(k->last, 1, VKBD_BUF_SIZE);
	k->nchar++;

	if (k->intr_enabled && !k->intr_pending) {
		k->intr_pending = true;
		if (k->intr_waiting > 0) {
			k->intr_waiting--;
			sem_signal(k->intr);
		}
	}
out:
	sem_signal(k->mutex);
}

void vkbd_putchar(natw code)
{
	if (vkbd_active)
		vkbd_nuovo_car(vkbd_active, code);
}

extern "C" void c_vkbd_send(int v, natw code, bool clear)
{
	if (v < -2 || v >= MAX_VKBD) {
		flog(LOG_WARN, "(send) vkbd inesistente: %d", v);
		abort_p();
	}

	des_vkbd *k;

	if (v == -1) {
		for (int i = 0; i < MAX_VKBD; i++) {
			k = &array_desvkbd[i];
			vkbd_nuovo_car(k, code, clear);
		}
	} else {
		if (v == -2) v = VKBD_CONSOLE;
		k = &array_desvkbd[v];
		vkbd_nuovo_car(k, code, clear);
	}
}

extern "C" natw c_vkbd_read(int v)
{
	natw code = 0;

	if (v >= MAX_VKBD) {
		flog(LOG_WARN, "(read) vkbd inesistente: %d", v);
		abort_p();
	}

	des_vkbd *k = &array_desvkbd[v];

	sem_wait(k->mutex);

	if (k->nchar <= 0)
		goto out;

	code = k->buf[k->first];
	k->first = circular_sum(k->first, 1, VKBD_BUF_SIZE);
	k->nchar--;

	k->intr_pending = false;

	if (k->nchar > 0 && k->intr_enabled) {
		k->intr_pending = true;
		if (k->intr_waiting > 0) {
			k->intr_waiting--;
			sem_signal(k->intr);
		}
	}
out:
	sem_signal(k->mutex);
	return code;
}

extern "C" void c_vkbd_intr_enable(int v, bool enable)
{
	if (v >= MAX_VKBD) {
		flog(LOG_WARN, "(intr_en) vkbd inesistente: %d", v);
		abort_p();
	}

	des_vkbd *k = &array_desvkbd[v];

	sem_wait(k->mutex);
	if (enable) {
		if (!k->intr_enabled && k->nchar > 0) {
			k->intr_pending = true;
			if (k->intr_waiting > 0) {
				k->intr_waiting--;
				sem_signal(k->intr);
			}
		}
		k->intr_enabled = true;
	} else {
		k->intr_enabled = false;
	}

	sem_signal(k->mutex);
}

const natl VKBD_LED_CAPSLOCK  = 1L;
const natl VKBD_LED_NUMLOCK   = 2L;
const natl VKBD_LED_SCROLLOCK = 4L;

extern "C" void kbd_set_leds(natb leds);

extern "C" void c_vkbd_leds(int v, natb led, bool on)
{
	if (v >= MAX_VKBD) {
		flog(LOG_WARN, "(leds) vkbd inesistente: %d", v);
		abort_p();
	}

	des_vkbd *k = &array_desvkbd[v];
	if (on) 
		k->leds |= led;
	else
		k->leds &= ~led; 

	if (k == vkbd_active)
		kbd_set_leds(k->leds);
}


extern "C" void c_vkbd_switch(int v)
{
	if (v >= MAX_VKBD) {
		flog(LOG_WARN, "(switch) vkbd inesistente: %d", v);
		abort_p();
	}

	des_vkbd *k = &array_desvkbd[v];

	vkbd_active = k;
	kbd_set_leds(vkbd_active->leds);
}


///////////////////////////////////////////////////////////////////////////////
//  MONITOR VIRTUALI                                                         //
///////////////////////////////////////////////////////////////////////////////

const natl SQRT_NUM_SCREEN = 2;
const natl NUM_SCREEN = 4;

const natl MAX_VMON = NUM_SCREEN;
natw *VIDEO_MEM_BASE = reinterpret_cast<natw *>(0x000b8000);
const int VIDEO_MEM_SIZE = 80 * 25;

const natl FONT_H = 16;
const natl FONT_W = 8;
const natb ch_bitmap[][FONT_H] = {
#include "font.txt"
};
const natl FONT_NUM = sizeof(ch_bitmap) / FONT_H;
natw max_off;

// descrittore di monitor virtuale
struct des_vmon {
	natw* buf;
	natl ch_x, ch_y;
	// text mode
	natw* video;
	natl cursor;
	natl cursor_size;
	// graph mode
	natl x, y;
	natl w, h;
	natl ubar, dbar;
};

des_vmon array_desvmon[MAX_VMON];
des_vmon *vmon_active;

extern "C" void console_cursor(unsigned int);
extern "C" void console_set_cursor_size(int size);

natb* framebuffer = 0;


static inline void write_pixel(natl x, natl y, natb c) {
	framebuffer[y * VID_W + x] = c;
}

static inline void write_screen(des_vmon* t, natl x, natl y, natb c) {
	write_pixel(t->x + x, t->y + y, c);
}

void write_gc(natl x, natl y, natw v) {
	natb fg = (v >> 8) & 0x0F;
	natb bg = (v >> 12) & 0x07;
	natb c = v;
	if (c >= FONT_NUM) 
		return;
	for (natl i = 0; i < FONT_H; i++) {
		natb w = ch_bitmap[c][i];
		for (natl j = 0; j < FONT_W; j++) {
			write_pixel(x + j, y + i, (w & 0x80) ? fg: bg);
			w <<= 1;
		}
	}
}

void color_bars(des_vmon *t, natb color) {
	//upbar
	for (natl k = 0; k < t->ubar; k++)
		for (natl j = 0; j < t->w; j++) 
			write_screen(t, j, k, color);

	//downbar
	natl downbar = t->h - t->dbar;
	for (natl k = downbar; k < t->h; k++)
		for (natl j = 0; j < t->w; j++) 
			write_screen(t, j, k, color);
}

void framebuffer_cursor(des_vmon *t) {
	natl riga = t->cursor / t->ch_x;
	if (riga > t->ch_y)
		return;
	natl colonna = t->cursor % t->ch_x;
	for (natl i = FONT_H - 1; i >= t->cursor_size; i--) 
		for (natl j = 0; j < FONT_W; j++) 
			framebuffer[(t->y + t->ubar + riga * FONT_H + i) * VID_W +
				    t->x + colonna * FONT_W + j] ^= 0x0F;

}

void framebuffer_update_cursor(des_vmon *t, natl off) {
	if (off == t->cursor)
		return;
	framebuffer_cursor(t);
	t->cursor = off;
	framebuffer_cursor(t);
}


bool vmon_init()
{
	framebuffer = static_cast<natb*>(graphic_mode());
	if (framebuffer)
		flog(LOG_INFO, "modalita' grafica");

	for (natl s = 0; s < MAX_VMON; s++) {
		des_vmon *t = &array_desvmon[s];
		t->cursor = 0;
		t->cursor_size = 14;
		
		if (framebuffer) {
			t->w = VID_W / SQRT_NUM_SCREEN;
			t->h = VID_H / SQRT_NUM_SCREEN;
	 		t->x = t->w * (s % SQRT_NUM_SCREEN);
			t->y = t->h * (s / SQRT_NUM_SCREEN);
			t->ubar = 32;
			t->dbar = 16;
			t->ch_x = t->w / FONT_W;
			t->ch_y = (t->h - t->ubar - t->dbar) / FONT_H;

			color_bars(t, 0x09);	
			////divisori verticali
			//for (int k = 0; k < t->h; k++) {
			//	write_screen(s, 0, k, 0x0B);
			//	write_screen(s, t->w - 1, k, 0x0B);
			//}

			////divisori orizzontali
			//for (int k = 0; k < t->w; k++) {
			//	write_screen(s, k, t->h - 1, 0x0B);
			//	write_screen(s, k, 0, 0x0B);
			//}
			framebuffer_cursor(t);
		} else {
			t->ch_x = 80;
			t->ch_y = 25;
		}
		t->video = t->buf = static_cast<natw*>(mem_alloc((t->ch_x * t->ch_y) * sizeof(natw)));
	} 
	flog(LOG_INFO, "vmon: inizializzati %d monitor virtuali", MAX_VMON);
	return true;
}

static void vmon_put(natl v, natl off, natw code)
{
	des_vmon *t = &array_desvmon[v];
	if (framebuffer) {
		natl riga = off / t->ch_x;
		natl colonna = off % t->ch_x;
		write_gc(t->x + colonna * FONT_W,
		         t->y + riga * FONT_H + t->ubar, code);
		if (off == t->cursor)
			framebuffer_cursor(t);
	}
	t->video[off] = code;
}

static inline natw vmon_get(natl v, natl off)
{
	return array_desvmon[v].video[off];
}

extern "C" void c_vmon_write_n(natl v, int off, natw vetti[], int quanti)
{
	if (v >= MAX_VMON) {
		flog(LOG_WARN, "(write) vmon inesistente: %d", v);
		abort_p();
	}

	des_vmon* t = &array_desvmon[v];
	int max_off = t->ch_x * t->ch_y;
	if (quanti <= 0 || off >= max_off || off + quanti <= 0)
		return;

	if (off < 0) {
		vetti += -off;
		quanti -= -off;
		off = 0;
	}
	if (off + quanti >= max_off) 
		quanti = max_off - off;
	for (int i = 0; i < quanti; i++)
		vmon_put(v, off + i, vetti[i]);
}
void light_to(natl v)
{
	des_vmon* t = &array_desvmon[v];

	if (vmon_active) 
		color_bars(vmon_active, 0x09);
	color_bars(t, 0x0B);

}

extern "C" void c_vmon_switch(natl v)
{
	if (v >= MAX_VMON) {
		flog(LOG_WARN, "(switch) vmon inesistente: %d", v);
		abort_p();
	}
	if (framebuffer) {
		light_to(v);
		vmon_active = &array_desvmon[v];
	} else {
		if (vmon_active) {
			memcpy(vmon_active->buf, VIDEO_MEM_BASE, VIDEO_MEM_SIZE * 2);
			vmon_active->video = vmon_active->buf;
		}
		vmon_active = &array_desvmon[v];
		vmon_active->video = VIDEO_MEM_BASE;
		memcpy(VIDEO_MEM_BASE, vmon_active->buf, VIDEO_MEM_SIZE * 2);
		console_set_cursor_size(vmon_active->cursor_size);
		console_cursor(vmon_active->cursor);
	}
}

extern "C" bool c_vmon_getsize(natl v, natw& maxx, natw& maxy)
{
	if (v >= MAX_VMON) {
		return false;
	}
	des_vmon *t = &array_desvmon[v];
	maxx = t->ch_x;
	maxy = t->ch_y;
	return true;
}

			
	
extern "C" void c_vmon_setcursor(natl v, natl off)
{
	if (v >= MAX_VMON) {
		flog(LOG_WARN, "(cursor) vmon inesistente: %d", v);
		abort_p();
	}
	des_vmon *t = &array_desvmon[v];
	if (t == vmon_active) {
		if (framebuffer) {
			framebuffer_update_cursor(t, off);
		} else {
			t->cursor = off;
			console_cursor(t->cursor);
		}
	}
}


extern "C" void c_vmon_cursor_shape(natl v, natl shape)
{
	if (v >= MAX_VMON) {
		flog(LOG_WARN, "(cshape) vmon inesistente: %d", v);
		abort_p();
	}

	natl size = 14;
	if (shape == 1) 
		size = 2;
	des_vmon *t = &array_desvmon[v];
	if (framebuffer)
		framebuffer_cursor(t);
	t->cursor_size = size;
	if (framebuffer)
		framebuffer_cursor(t);
	else
		console_set_cursor_size(t->cursor_size);

}

////////////////////////////////////////////////////////////////////////////////
//                         GESTIONE DELLA CONSOLE [9.5]                       //
////////////////////////////////////////////////////////////////////////////////

natw COLS = 80; 	// [9.5]
natw ROWS = 25;	// [9.5]
natl VIDEO_SIZE = COLS * ROWS;	// [9.5]

struct des_vid {
	int vmon;
	natl x, y;
	natb attr;
};

const natl MAX_CODE = 29; // [9.5]

struct des_ckbd { // [9.5]
	int vkbd;
	addr punt;
	natl cont;
	bool shift;
	natb tab[MAX_CODE];
	natb tabmin[MAX_CODE];
	natb tabmai[MAX_CODE];
};

struct des_console { // [9.5]
	natl mutex;
	natl sincr;
	des_ckbd kbd;
	des_vid vid;
};

extern "C" des_console console; // [9.5]

void scroll(des_vid *p_des)	// [9.5]
{
	int vmon = p_des->vmon;
	for (natl i = 0; i < VIDEO_SIZE - COLS; i++) 
		vmon_put(vmon, i, vmon_get(vmon, i + COLS));
	for (natl i = 0; i < COLS; i++)
		vmon_put(vmon, VIDEO_SIZE - COLS + i, 0 | p_des->attr << 8);
	p_des->y--;
}

void writeelem(natb c) {	// [9.5]
	des_vid* p_des = &console.vid;
	int vmon = p_des->vmon;
	switch (c) {
	case '\r':
		p_des->x = 0;
		break;
	case '\n':
		p_des->y++;
		if (p_des->y >= ROWS)
			scroll(p_des);
		break;
	case '\b':
		if (p_des->x > 0 || p_des->y > 0) {
			if (p_des->x == 0) {
				p_des->x = COLS - 1;
				p_des->y--;
			} else
				p_des->x--;
		}
		break;
	default:
		vmon_put(vmon, p_des->y * COLS + p_des->x, c | p_des->attr << 8);
		p_des->x++;
		if (p_des->x >= COLS) {
			p_des->x = 0;
			p_des->y++;
		}
		if (p_des->y >= ROWS) 
			scroll(p_des);
		break;
	}
	c_vmon_setcursor(vmon, p_des->y * COLS + p_des->x);
}

void writeseq(cstr seq)	// [9.5]
{
	const natb* pn = static_cast<const natb*>(seq);
	while (*pn != 0) {
		writeelem(*pn);
		pn++;
	}
}

extern "C" void c_writeconsole(cstr buff) // [9.5]
{
	des_console *p_des = &console;
	sem_wait(p_des->mutex);
	writeseq(buff);
	writeseq("\r\n");
	sem_signal(p_des->mutex);
}

//extern "C" void go_inputkbd(interfkbd_reg indreg); // [9.5]
//extern "C" void halt_inputkbd(interfkbd_reg indreg); // [9.5]

void startkbd_in(des_ckbd* p_des, str buff) // [9.5]
{
	p_des->punt = buff;
	p_des->cont = 80;
	//go_inputkbd(p_des->indreg);
	c_vkbd_intr_enable(p_des->vkbd, true);
}

extern "C" void c_readconsole(str buff, natl& quanti) // [9.5]
{
	des_console *p_des;

	p_des = &console;
	sem_wait(p_des->mutex);
	startkbd_in(&p_des->kbd, buff);
	sem_wait(p_des->sincr);
	quanti = p_des->kbd.cont;
	sem_signal(p_des->mutex);
}

natb converti(des_ckbd* p_des, natb c) { // [9.5]
	natb cc;
	natl pos = 0;
	while (pos < MAX_CODE && p_des->tab[pos] != c)
		pos++;
	if (pos == MAX_CODE)
		return 0;
	if (p_des->shift)
		cc = p_des->tabmai[pos];
	else
		cc = p_des->tabmin[pos];
	return cc;
}

void estern_ckbd(int h) // [9.5]
{
	des_console *p_des = &console;
	natw C;
	natb a, c;
	bool fine;

	for(;;) {
		//halt_inputkbd(p_des->kbd.indreg);
		c_vkbd_intr_enable(p_des->kbd.vkbd, false);

		//inputb(p_des->kbd.indreg.iRBR, c);
		C = c_vkbd_read(p_des->kbd.vkbd);
		if (C == 0xFFFF)
			terminate_p();

		c = (natb)C;
		
		fine = false;
		switch (c) {
		case 0x2a: // left shift make code
			p_des->kbd.shift = true;
			break;
		case 0xaa: // left shift break code
			p_des->kbd.shift = false;
			break;
		default:
			if (c < 0x80) {
				a = converti(&p_des->kbd, c);
				if (a == 0)
					break;
				if (a == '\b') {
					if (p_des->kbd.cont < 80) {
						p_des->kbd.punt = static_cast<natb*>(p_des->kbd.punt) - 1;
						p_des->kbd.cont++;
						writeseq("\b \b");
					}
				} else if (a == '\r' || a == '\n') {
					fine = true;
					p_des->kbd.cont = 80 - p_des->kbd.cont;
					*static_cast<natb*>(p_des->kbd.punt) = 0;
					writeseq("\r\n");
				} else {
					*static_cast<natb*>(p_des->kbd.punt) = a;
					p_des->kbd.punt = static_cast<natb*>(p_des->kbd.punt) + 1;
					p_des->kbd.cont--;
					writeelem(a);
					if (p_des->kbd.cont == 0) {
						fine = true;
						p_des->kbd.cont = 80;
					}
				}
			}
			break;
		}
		if (fine == true) 
			sem_signal(p_des->sincr);
		else
			//go_inputkbd(p_des->kbd.indreg);
			c_vkbd_intr_enable(p_des->kbd.vkbd, true);
		//nwfi(master);
		c_vkbd_wfi(p_des->kbd.vkbd);
	}
}

// (* inizializzazioni
bool con_vid_init(int vmon);

extern "C" void c_iniconsole(natb cc)
{
	des_vid *p_des = &console.vid;
	p_des->attr = cc;
	int vmon = p_des->vmon;
	for (natl i = 0; i < VIDEO_SIZE; i++) 
		vmon_put(vmon, i, i | p_des->attr << 8);
	p_des->x = p_des->y = 0;
	c_vmon_setcursor(vmon, p_des->y * COLS + p_des->x);
}

bool con_kbd_init(int vkbd)
{
	console.kbd.vkbd = vkbd;
	if (activate_p(estern_ckbd, 0, PRIO, LIV) == 0xFFFFFFFF) {
		flog(LOG_ERR, "console: impossibile creare estern_ckbd");
		return false;
	}
	return true;
}

extern "C" des_vid vid;

bool con_vid_init(int vmon)
{
	c_vmon_getsize(vmon, COLS, ROWS);
	VIDEO_SIZE = COLS * ROWS;
	des_vid *p_des = &console.vid;
	p_des->vmon = vmon;
	for (natl i = 0; i < VIDEO_SIZE; i++) 
		vmon_put(vmon, i, 0 | p_des->attr << 8);
	c_vmon_setcursor(vmon, p_des->y * COLS + p_des->x);
	flog(LOG_INFO, "vid: video inizializzato");
	return true;
}

bool console_init(int vkbd, int vmon) {
	des_console *p_des = &console;

	if ( (p_des->mutex = sem_ini(1)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "console: impossibile creare mutex");
		return false;
	}
	if ( (p_des->sincr = sem_ini(0)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "console: impossibile creare sincr");
		return false;
	}
	return con_kbd_init(vkbd) && con_vid_init(vmon);
}

// *)

/////////////////////////////////////////////////////////////////////////////////
//                  FUNZIONI DI LIBRERIA                                       //
/////////////////////////////////////////////////////////////////////////////////
//typedef char *va_list;

// Versione semplificata delle macro per manipolare le liste di parametri
//  di lunghezza variabile; funziona solo se gli argomenti sono di
//  dimensione multipla di 4, ma e' sufficiente per le esigenze di printk.
//
//#define va_start(ap, last_req) (ap = (char *)&(last_req) + sizeof(last_req))
//#define va_arg(ap, type) ((ap) += sizeof(type), *(type *)((ap) - sizeof(type)))
//#define va_end(ap)
/*
natl strlen(const char *s)
{
	natl l = 0;

	while(*s++)
		++l;

	return l;
}

char *strncpy(char *dest, const char *src, unsigned long l)
{
	unsigned long i;

	for(i = 0; i < l && src[i]; ++i)
		dest[i] = src[i];

	return dest;
}

static const char hex_map[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static void htostr(char *buf, natl l, int cifre = 8)
{
	for (int i = cifre - 1; i >= 0; --i) {
		buf[i] = hex_map[l % 16];
		l /= 16;
	}
}

static void itostr(char *buf, unsigned int len, long l)
{
	natl i, div = 1000000000, v, w = 0;

	if(l == (-2147483647 - 1)) {
		strncpy(buf, "-2147483648", 12);
		return;
	} else if(l < 0) {
		buf[0] = '-';
		l = -l;
		i = 1;
	} else if(l == 0) {
		buf[0] = '0';
		buf[1] = 0;
		return;
	} else
		i = 0;

	while(i < len - 1 && div != 0) {
		if((v = l / div) || w) {
			buf[i++] = '0' + (char)v;
			w = 1;
		}

		l %= div;
		div /= 10;
	}

	buf[i] = 0;
}

#define DEC_BUFSIZE 12

int vsnprintf(char *str, natl size, const char *fmt, va_list ap)
{
	natl in = 0, out = 0, tmp;
	char *aux, buf[DEC_BUFSIZE];
	natl cifre;

	while(out < size - 1 && fmt[in]) {
		switch(fmt[in]) {
			case '%':
				cifre = 8;
			again:
				switch(fmt[++in]) {
					case '1':
					case '2':
					case '4':
					case '8':
						cifre = fmt[in] - '0';
						goto again;
					case 'd':
						tmp = va_arg(ap, int);
						itostr(buf, DEC_BUFSIZE, tmp);
						if(strlen(buf) >
								size - out - 1)
							goto end;
						for(aux = buf; *aux; ++aux)
							str[out++] = *aux;
						break;
					case 'x':
						tmp = va_arg(ap, int);
						if(out > size - (cifre + 1)) 
							goto end;
						htostr(&str[out], tmp, cifre);
						out += cifre;
						break;
					case 's':
						aux = va_arg(ap, char *);
						while(out < size - 1 && *aux)
							str[out++] = *aux++;
						break;	
					case 'c':
						tmp = va_arg(ap, int);
						if (out < size - 1)
							str[out++] = tmp;
						break;
				}
				++in;
				break;
			default:
				str[out++] = fmt[in++];
		}
	}
end:
	str[out++] = 0;

	return out;
}

int snprintf(char *buf, unsigned long n, const char *fmt, ...)
{
	va_list ap;
	int l;

	va_start(ap, fmt);
	l = vsnprintf(buf, n, fmt, ap);
	va_end(ap);

	return l;
}

// copia n byte da src a dest
void *memcpy(void *dest, const void *src, unsigned int n)
{
	char       *dest_ptr = static_cast<char*>(dest);
	const char *src_ptr  = static_cast<const char*>(src);

	if (src_ptr < dest_ptr && src_ptr + n > dest_ptr)
		for (int i = n - 1; i >= 0; i--)
			dest_ptr[i] = src_ptr[i];
	else
		for (natl i = 0; i < n; i++)
			dest_ptr[i] = src_ptr[i];

	return dest;
}

// scrive n byte pari a c, a partire da dest
void *memset(void *dest, int c, unsigned int n)
{
	char *dest_ptr = static_cast<char*>(dest);

        for (natl i = 0; i < n; i++)
              dest_ptr[i] = static_cast<char>(c);

        return dest;
}

*/

// log formattato
void flog(log_sev sev, const char *fmt, ...)
{
	va_list ap;
	char buf[LOG_MSG_SIZE];

	va_start(ap, fmt);
	int l = vsnprintf(buf, LOG_MSG_SIZE, fmt, ap);
	va_end(ap);

	if (l > 1)
		log(sev, buf, l - 1);
}

struct des_mem {
	natl dimensione;
	des_mem* next;
};

des_mem* memlibera = 0;
natl mem_mutex;

natl allinea(natl v, natl a)
{
	return (v % a == 0 ? v : ((v + a - 1) / a) * a);
}
natl padding(addr p, natl align)
{
	natl pad = (align - (natl)p) & (align - 1);
	return pad;
}

void* mem_alloc(natl dim, natl balign)
{
	natl quanti = allinea(dim, sizeof(des_mem));
	natl align = 1U << balign;
	if (align < sizeof(des_mem))
			align = sizeof(des_mem);

	sem_wait(mem_mutex);
	
	des_mem *prec = 0, *scorri = memlibera;
	while (scorri != 0 && scorri->dimensione < quanti + padding(scorri + 1, align)) {
		prec = scorri;
		scorri = scorri->next;
	}

	addr p = 0;
	if (scorri != 0) {
		p = scorri + 1; // puntatore al primo byte dopo il descrittore
		natl pad = padding(p, align);
		p = static_cast<natb*>(p) + pad;

		if (pad > 0) {
			addr pnuovo = static_cast<natb*>(p) - sizeof(des_mem);
			des_mem* nuovo = static_cast<des_mem*>(pnuovo);

			nuovo->dimensione = scorri->dimensione - pad;
			nuovo->next = scorri->next;
			scorri->dimensione = pad - sizeof(des_mem);
			scorri->next = nuovo;

			prec = scorri;
			scorri = scorri->next;
		} 

		if (scorri->dimensione - quanti >= sizeof(des_mem) + sizeof(int)) {

			addr pnuovo = static_cast<natb*>(p) + quanti;
			des_mem* nuovo = static_cast<des_mem*>(pnuovo);

			nuovo->dimensione = scorri->dimensione - quanti - sizeof(des_mem);
			scorri->dimensione = quanti;

			nuovo->next = scorri->next;
			if (prec != 0) 
				prec->next = nuovo;
			else
				memlibera = nuovo;

		} else {

			if (prec != 0)
				prec->next = scorri->next;
			else
				memlibera = scorri->next;
		}	
		scorri->next = reinterpret_cast<des_mem*>(0xdeadbeef);
		
	}
	sem_signal(mem_mutex);

	return p;
}

void free_interna(addr indirizzo, natl quanti);

void mem_free(void* p)
{
	if (p == 0) return;
	des_mem* des = reinterpret_cast<des_mem*>(p) - 1;
	sem_wait(mem_mutex);
	free_interna(des, des->dimensione + sizeof(des_mem));
	sem_signal(mem_mutex);
}

void free_interna(addr indirizzo, natl quanti)
{
	if (quanti == 0) return;
	des_mem *prec = 0, *scorri = memlibera;
	while (scorri != 0 && scorri < indirizzo) {
		prec = scorri;
		scorri = scorri->next;
	}
	if (prec != 0 && (natb*)(prec + 1) + prec->dimensione == indirizzo) {
		if (scorri != 0 && static_cast<natb*>(indirizzo) + quanti == (addr)scorri) {
			
			prec->dimensione += quanti + sizeof(des_mem) + scorri->dimensione;
			prec->next = scorri->next;
		} else {
			prec->dimensione += quanti;
		}
	} else if (scorri != 0 && static_cast<natb*>(indirizzo) + quanti == (addr)scorri) {
		des_mem salva = *scorri; 
		des_mem* nuovo = reinterpret_cast<des_mem*>(indirizzo);
		*nuovo = salva;
		nuovo->dimensione += quanti;
		if (prec != 0)
			prec->next = nuovo;
		else
			memlibera = nuovo;
	} else if (quanti >= sizeof(des_mem)) {
		des_mem* nuovo = reinterpret_cast<des_mem*>(indirizzo);
		nuovo->dimensione = quanti - sizeof(des_mem);
		nuovo->next = scorri;
		if (prec != 0)
			prec->next = nuovo;
		else
			memlibera = nuovo;
	}
}

extern "C" natl end;

extern "C" void lib_init()
{
	mem_mutex = sem_ini(1);
	natl heap_start = allinea((natl)&end, sizeof(des_mem));
	free_interna((addr)heap_start, (NTAB_SIS_C+NTAB_SIS_P+NTAB_MIO_C)*DIM_MACROPAGINA - heap_start);
}

////////////////////////////////////////////////////////////////////
//                 SCHEDA AUDIO ES1370	(ENSONIQ PCI)             //
////////////////////////////////////////////////////////////////////

//////////* Sezione dati */////////////////

// costanti per la scheda audio Ensoniq AudioPci 1370
#define BUFFERSIZE (64*1024)				// dimensione del buffer per l'esecuzione
									// corrisponde alla dimensione della
									// pagina fisica
#define HALFBUFFERSIZE (BUFFERSIZE>>1)
#define BUFFERALIGN 4				// allineamento del buffer obbligatorio

// struttura per i registri della es1370
struct ens_reg{
	ioaddr control;			// Registro di controllo Interrupt/Chip 
	ioaddr status; 			// 0x04	Registro di stato per Interrupt/Chip 
							// 		sola lettura

	ioaddr uart_data; 		// 0x08	Registro UART
	union{
		ioaddr uart_status; 	
							// 0x09	Registro di stato UART
							//		sola lettura
		ioaddr uart_control; 	
							// 0x09	Registro di controllo UART
							//		sola scrittura
	};
	ioaddr uart_res;		// 0x0a	Registro riservato UART

	ioaddr mem_page;		// 0x0c	Registro per le pagine di memoria
	
	ioaddr codec_1370; 		// 0x10	Registro di scrittura nel CODEC
							//		sola scrittura
							
	ioaddr channel_status; 	// 0x1c   
	ioaddr serial;			// 0x20	Registro di controllo 
							//		per l'interfaccia seriale 

	ioaddr dac1_count; 		// 0x24	Registro di conteggio 
							//		dei campioni per DAC1
	ioaddr dac2_count; 		// 0x28	Registro di conteggio 
							//		dei campioni per DAC2
	ioaddr adc_count;  		// 0x2c	Registro di conteggio 
							//		dei campioni per ADC

	union{
	ioaddr dac1_frame; 		// 0x30	Pagina 0x0c; DAC1 indirizzo buffer
	ioaddr adc_frame;  		// 0x30	Pagina 0x0d; ADC indirizzo buffer
	ioaddr uart_fifo;  		// 0x30	Pagina 0x0e; registro coda UART
	};

	union{
	ioaddr dac1_size;  		// 0x34	Pagina 0x0c; DAC1 dimensione buffer
	ioaddr adc_size;	 	// 0x34	Pagina 0x0d; ADC dimensione buffer
	};

	union{
	ioaddr dac2_frame; 		// 0x38 Pagina 0x0c; DAC2 indirizzo buffer
	ioaddr phantom_frame; 	// 0x38 Pagina 0x0d: Indirizzo buffer "phantom"
	};

	union{
	ioaddr dac2_size;  		// 0x3c	Pagina 0x0c DAC2 dimensione buffer
	ioaddr phantom_count; 	// 0x3c Pagina 0x0d: 
							//		contatore buffer "phantom"
	};
};


// struttura dati per un suono da eseguire
// (descrittore di canale)

struct suono {
	int quanti;		// dimensione (in byte) del file sonoro
	
	natw oldsize; 	// vecchio contatore di longword
	addr puntatore_brano;	// indirizzo virtuale del suono da eseguire
	addr buffer;		// indirizzo virtuale del primo byte del
						// buffer (uguale)
	natl mutex;		// semaforo di mutua esclusione
	natl sincr;		// semaforo di sincronizzazione
};	


// struttura descrittore di scheda audio
struct des_snd {
	ens_reg indreg;
			// indirizzi dei registri
	natl chip_mutex;
			// semaforo di mutua esclusione per il CODEC
	natl es_mutex;
			// semaforo di mutua esclusione per la scheda audio
	natb irq;
			// piedino di interruzione
	bool presente;	
			// è opportuno settare un booleano nel descrittore
			// in questo modo evito che il modulo utente chiami
			// funzioni audio inesistenti			
	natb chip_reg[0x1A];
			// poichè il chip è accessibile in sola scrittura
			// qualsiasi modifica rispetto allo stato di reset
			// deve essere mantenuta in una struttura dati
			// per essere letta, e deve essere protetta
			// tramite semaforo di mutua esclusione

		
	natl ctrl;	// registro di controllo
	natl sctrl;	// registro -seriale- di controllo
	natl cssr;	// registro di stato
	natb uartc;	// registro di controllo UART
	
	// seppur questi registri (al contrario dei registri del CODEC)
	// sono anche leggibili, conviene tenerne una copia in memoria
	// perchè alcuni controlli possano essere fatti anche senza dover
	// accedere alla periferica

	suono canali[3];
			// canali[0] il canale ADC
			// canali[1] il canale DAC1
			// canali[2] il canale DAC2			
};


des_snd ensoniq;


//////////* Sezione testo */////////////////

// la pci_read prende come parametro un valore w (parola)
// formato dalla periferica e dalla funzione nel bus
// di cui vogliamo leggere lo spazio di configurazione

natw make_w(natb dev, natb fun)
{
	natw res= (dev << 3) | fun;
	return res;
}



// alcune funzioni di utilità per scrivere nel buffer circolare
// utilizzato dalla scheda audio
void copia_snd_buffer(addr puntatore_brano, addr buffersuono, int oldsize, int n_long)
{
	int giro=BUFFERSIZE>>2;
	
	int* brano= (int*) (puntatore_brano);
	int* buffer=(int*) (buffersuono);
	for(int i=0; i<n_long; i++)
		buffer[(i+oldsize)%giro]=brano[i];
}

void copia_snd_buffer_B( addr puntatore_brano, addr buffersuono, int oldsize,  int n)
{
	int giro=BUFFERSIZE;
	
	natb* brano = (natb*) (puntatore_brano);
	natb* buffer=(natb*) (buffersuono);
	for(int i=0; i<n; i++)
		buffer[(i+oldsize)%giro]=brano[i];
}


// crea silenzio
void mute_snd_buffer(addr buffersuono, int oldsize, int n_long)
{	
	int giro=BUFFERSIZE>>2;
	int* buffer=(int*) (buffersuono);
	for(int i=0; i<n_long; i++)
		buffer[(i+oldsize)%giro]=0x00;
}
void mute_snd_buffer_B(addr buffersuono, int oldsize, int n)
{
	int giro=BUFFERSIZE;
	
	natb* buffer=(natb*) (buffersuono);
	for(int i=0; i<n; i++)
		buffer[(i+oldsize)%giro]=0x00;
}


#if 0
// FUNZIONI MIXER

// accesso al chip ak4531 per la gestione del mixer

void ak4531_write(natb reg, natb data)
{
	des_snd * p_des;
	p_des = & ensoniq;
	
	natl statuslong;
	natl info = ES_CODEC_WRITE(reg,data);
	
	inputl(p_des->indreg.status, statuslong);

	if((statuslong & ES_1370_CSTAT)==0)
	{
		outputl(info, p_des->indreg.codec_1370);		
		return;
	}
	udelay(100);
	
	inputl(p_des->indreg.status, statuslong);
	if(!(statuslong & (ES_1370_CSTAT))){
		outputl(info , p_des->indreg.codec_1370);
		inputl(p_des->indreg.codec_1370, info);
		return;
	}
	flog(LOG_WARN, "Errore nella scrittura del CODEC; stato attuale: %x", statuslong);
}



// reset del CODEC
void ak4531_reset()
{	
	ak4531_write(AK4531_RESET, 0x03);	
	ak4531_write(AK4531_RESET, 0x02);
}

// inizializzazione del codec
bool ak4531_init(ioaddr base)
{
	des_snd * p_des;
	p_des= &ensoniq;
		
	ak4531_reset();
	
	p_des->chip_reg[AK4531_LMASTER]		= 0x80 ;
	p_des->chip_reg[AK4531_RMASTER]		= 0x80 ;
	p_des->chip_reg[AK4531_LVOICE]		= 0x86 ;
	p_des->chip_reg[AK4531_RVOICE]		= 0x86 ;
	p_des->chip_reg[AK4531_LFM]		= 0x86 ;
	p_des->chip_reg[AK4531_RFM]		= 0x86 ;
	p_des->chip_reg[AK4531_LCD]		= 0x86 ;
	p_des->chip_reg[AK4531_RCD]		= 0x86 ;
	p_des->chip_reg[AK4531_LLINE]		= 0x86 ;
	p_des->chip_reg[AK4531_RLINE]		= 0x86 ;
	p_des->chip_reg[AK4531_LAUXA]		= 0x86 ;
	p_des->chip_reg[AK4531_RAUXA]		= 0x86 ;
	p_des->chip_reg[AK4531_MONO1]		= 0x86 ;
	p_des->chip_reg[AK4531_MONO2]		= 0x86 ;
	p_des->chip_reg[AK4531_MIC]		= 0x86 ;
	p_des->chip_reg[AK4531_MONO_OUT]	= 0x80 ;
	p_des->chip_reg[AK4531_OUT_SW1]		= 0x00 ;
	p_des->chip_reg[AK4531_OUT_SW2]		= 0x00 ;
	p_des->chip_reg[AK4531_LIN_SW1]		= 0x00 ;
	p_des->chip_reg[AK4531_RIN_SW1]		= 0x00 ;
	p_des->chip_reg[AK4531_LIN_SW2]		= 0x00 ;
	p_des->chip_reg[AK4531_RIN_SW2]		= 0x00 ;
	p_des->chip_reg[AK4531_RESET]		= 0x00 ;
	p_des->chip_reg[AK4531_CLOCK]		= 0x03 ;
	p_des->chip_reg[AK4531_AD_IN]		= 0x00 ;
	p_des->chip_reg[AK4531_MIC_GAIN]	= 0x00 ;

//** DA PENSARCI **********************************************	
//	for(int i=AK4531_LMASTER; i<=AK4531_MIC_GAIN; i++)
//		if(i!=AK4531_CLOCK && i!= AK4531_RESET)
//			ak4531_write(i, p_des->chip_reg[i]);
//**************************************************************
				// i dati iniziali sono presi dal foglio di specifiche
				// del CODEC AK4531
				// e equivalgono ai valori al reset asincrono
				// questa parte, perciò, non è strettamente necessaria
				// ma potrebbe essere una scelta oculata
				// per essere certi della coerenza dei valori nel CODEC
				
	if ((p_des->chip_mutex = sem_ini(1)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "com: impossibile creare mutex");
		return false;
		}
	return true;
}


// primitive per il modulo utente
extern "C" void ak4531_setMixer(natb &err, natb port, natb value)
{
	des_snd * p_des;
	p_des= &ensoniq;
	
	if(!p_des->presente)
	{
		flog(LOG_WARN, "Scheda audio non presente!");
		err = -2; 	
		return;
	}
	err=0;
	
	// *(le primitive non devono mai fidarsi dei parametri
	
	if(		port<AK4531_LMASTER || port >AK4531_MIC_GAIN 
		|| 	port==AK4531_RESET 	|| port==AK4531_CLOCK)
	{						
		flog(LOG_WARN, "Tentativo di accedere a una porta non corretta");
		err=-1;
		return;
	}
	
	// e non permettiamo di effettuare il reset o modificare il clock da utente *)

	sem_wait(p_des->es_mutex);	
	sem_wait(p_des->chip_mutex);
	p_des->chip_reg[port]=value;

	ak4531_write(port, value);
	sem_signal(p_des->chip_mutex);
	sem_signal(p_des->es_mutex);
}

extern "C" void ak4531_getMixer(natb &err, natb &value, natb port)
{
	des_snd * p_des;
	p_des= &ensoniq;
	if(!p_des->presente)
	{
		flog(LOG_WARN, "Scheda audio non presente!");
		err = -2; 	
		return;
	}		
	err=0;
	
	// *(le primitive non devono mai fidarsi dei parametri
	
	if(port<AK4531_LMASTER || port >AK4531_MIC_GAIN){
		flog(LOG_WARN, "Tentativo di accedere a una porta non corretta");
		err=-1;
		return;
	}
	//	*)	
	sem_wait(p_des->chip_mutex);
		// poichè leggiamo da memoria, e non dal CODEC (non possiamo)
		// non ha senso bloccare la periferica: facciamo solo sì che
		// durante la lettura nessuno possa scrivere nel registro CODEC

	
	value= p_des->chip_reg[port];
	
	sem_signal(p_des->chip_mutex);
}
#endif

// FUNZIONI INIZIALIZZAZIONE
void gestisci_int_dac(int channel);
void estern_snd(int h);
	// dichiarazione processo esterno

void ensoniq_init(natw l)
{
	des_snd * p_des;
	p_des = & ensoniq;
	p_des->presente= false;

	natl base_address = pci_read(l, 0x10, 4);
	base_address = base_address & 0xFFFE; 			
			// resettiamo il bit meno significativo, a uno da specifiche

	p_des->indreg.control		= base_address | ES_CONTROL;		
	p_des->indreg.status 		= base_address | ES_STATUS; 
	p_des->indreg.uart_data 	= base_address | ES_UART_DATA; 
	p_des->indreg.uart_status	= base_address | ES_UART_STATUS; 
	p_des->indreg.uart_res		= base_address | ES_UART_RES;		
	p_des->indreg.mem_page		= base_address | ES_MEM_PAGE;
	p_des->indreg.codec_1370 	= base_address | ES_CODEC_1370;
	p_des->indreg.channel_status	= base_address | ES_CHANNEL_STATUS; 
	p_des->indreg.serial		= base_address | ES_SERIAL;	
	p_des->indreg.dac1_count	= base_address | ES_DAC1_COUNT;
	p_des->indreg.dac2_count	= base_address | ES_DAC2_COUNT; 
	p_des->indreg.adc_count		= base_address | ES_ADC_COUNT; 
	p_des->indreg.dac1_frame	= base_address | ES_DAC1_FRAME;
	p_des->indreg.dac1_size		= base_address | ES_DAC1_SIZE;
	p_des->indreg.dac2_frame	= base_address | ES_DAC2_FRAME;
	p_des->indreg.dac2_size		= base_address | ES_DAC2_SIZE;

	
	if ((p_des->es_mutex = sem_ini(1)) == 0xFFFFFFFF)
	{
		flog(LOG_ERR, "es1370: impossibile creare mutex");
		return;
	}
	
	p_des->ctrl =	0x00000000;	// registro di controllo
	p_des->sctrl =	0x00000000;	// registro -seriale- di controllo
	p_des->cssr =	0x00000000;	// registro di stato
	p_des->uartc =	0x00;	// registro di controllo UART

	natb irq = pci_read(l, 0x3C, 1);
	p_des->irq=irq;	

	if(activate_pe(estern_snd, 0, PRIO, LIV, irq)==0xFFFFFFFF)
	{
  		flog(LOG_ERR, "es1370: impossibile creare processo esterno");
		return;	
	}
			
#if 0			
	if(!ak4531_init(base_address))
		return false;		
#endif

	for(int i=0; i<3; i++)
	{	if (((p_des->canali[i].mutex = sem_ini(1)) == 0xFFFFFFFF) || 
		    ((p_des->canali[i].sincr = sem_ini(0)) == 0xFFFFFFFF)){
		    		flog(LOG_ERR, "es1370: impossibile creare mutex");
				return;
		    }
	}
	
	outputl(p_des->ctrl, p_des->indreg.control);
	outputl(p_des->sctrl, p_des->indreg.serial);
	outputl(p_des->cssr, p_des->indreg.status);
	outputb(p_des->uartc, p_des->indreg.uart_status);

	flog(LOG_INFO, "es1370: io=%4x, irq=%d", base_address, irq);

	p_des->presente= true;

}


// cerca se è installata una scheda audio e chiama la corrispondente 
// funzione di inizializzazione
// per ora si limita a controllare se la ES1370 è installata

void rilascia_dac(int);


// gestione delle interruzioni per i canali di esecuzione
void gestisci_int_dac(int channel)
{	
	des_snd * p_des;
	p_des= &ensoniq;
	
	natl BUFFERLONG = BUFFERSIZE>>2;
			// numero delle parole lunghe contenute in un buffer
	natl appoggio;
	natw current;
	int difflong;
	int copia_n;
	ioaddr dac1_size= (channel==1)? p_des->indreg.dac1_size : p_des->indreg.dac2_size;
								  
	int oldsize=p_des->canali[channel].oldsize;
			// prendiamo l'indice dell'ultima copia nel buffer
	addr puntatore_brano=p_des->canali[channel].puntatore_brano;
	addr buffer=p_des->canali[channel].buffer;
	int ancora = p_des->canali[channel].quanti;
	

	sem_wait(p_des->es_mutex);
	outputl(0x0000000C, p_des->indreg.mem_page);
	inputl(dac1_size, appoggio);
	sem_signal(p_des->es_mutex);
	current = (appoggio& 0xFFFF0000)>>16;
			// in current ora c'è l'indice del buffer che è stato
			// copiato nella scheda audio
	
	difflong = (current >= oldsize)? (current - oldsize) 
			: (BUFFERLONG) - oldsize + current;
			// il buffer è circolare, current potrebbe essere minore di
			// oldsize
	
	if(ancora<=0)
	{
		// abbiamo un numero di byte da scrivere negativo, ovvero
		// il brano è stato tutto copiato, ma non eseguito: 
		// riempiamo gradualmente il buffer di 0x00, ovvero silenzio
		// il loop farà il resto
		
		mute_snd_buffer(buffer, oldsize, difflong);
		p_des->canali[channel].quanti -= (difflong<<2);
		if(p_des->canali[channel].quanti<=(~BUFFERSIZE)+1)
			rilascia_dac(1);
		// se abbiamo quanti < -BUFFERSIZE (complemento a due)
		// significa che il buffer è stato riempito tutto di 0: 
		// possiamo staccare 
		
		p_des->canali[channel].oldsize = current;
		// altrimenti ricopiamo il valore di current in oldsize
		return;
	}
	
	copia_n= (difflong < (ancora>>2))? difflong:(ancora>>2);	
	copia_snd_buffer(puntatore_brano, buffer, oldsize, copia_n);
		// copiamo un numero di parole lunghe pari:
		// 	alle parole lunghe copiate dalla scheda audio o
		//	alle parole lunghe che rimangono nel brano
	
	p_des->canali[channel].puntatore_brano = (int*)puntatore_brano + copia_n;		
	p_des->canali[channel].quanti -= (copia_n<<2);
	ancora = p_des->canali[channel].quanti;
		// aggiorniamo il puntatore all'interno del brano
		// e il contatore di byte del brano
		
	if(difflong>copia_n)
	{	
		// il brano era finito: abbiamo copiato le ultime long
		// dobbiamo ora copiare gli ultimi byte del brano
		// e riempire il resto del buffer di silenzio
		puntatore_brano = p_des->canali[channel].puntatore_brano;
		natl copia_nB = copia_n << 2;
		natl oldsizeB = (oldsize << 2) + copia_nB;
		
		copia_snd_buffer_B(puntatore_brano, buffer, oldsizeB, ancora);
		copia_nB = (difflong<<2) - copia_nB - ancora;
		oldsizeB += ancora;
		mute_snd_buffer_B(buffer, oldsizeB, copia_nB);
		p_des->canali[channel].quanti-=(ancora + copia_nB);
	}	
	p_des->canali[channel].oldsize = current;
		// aggiorniamo l'indice all'interno del buffer
}	

	
// Processo esterno per la scheda audio
void estern_snd(int h)
{
	des_snd * p_des;
	p_des = & ensoniq;
	natl status;

	for(;;)
	{
		sem_wait(p_des->es_mutex);
		inputl(p_des->indreg.status, status);	
		sem_signal(p_des->es_mutex);

		if(!(status & ES1370_INTR))
		{	
			goto out;
		}
		
		if (status & ES1370_ADC)
		;
		if (status & ES1370_DAC1)
		{
			sem_wait(p_des->es_mutex);
			p_des->sctrl &= ~(ES1370_P1_INT_EN);	
			outputl(p_des->sctrl, p_des->indreg.serial);
			p_des->sctrl |= (ES1370_P1_INT_EN);	
			outputl(p_des->sctrl, p_des->indreg.serial);
			sem_signal(p_des->es_mutex);
			gestisci_int_dac(1);
			// disabilitazione e riabilitazione delle interruzioni
			// e gestione della interruzione
		}	
		if (status & ES1370_DAC2)
		;
	out:
		wfi();
	}
}


// Preparazione del canale di esecuzione 1
void prepara_dac1(des_snd * p_des, addr puntatore, natl size, natl rate, natl mode)
{
	sem_wait(p_des->es_mutex);
	p_des->ctrl &= ~(ES1370_DAC1_EN);
	outputl(p_des->ctrl, p_des->indreg.control);
	sem_signal(p_des->es_mutex);

	// disabilitamo il canale
	
	addr buffersuono = phys_alloc(BUFFERSIZE, BUFFERALIGN);
	// allochiamo il buffer
	
	if(buffersuono==0)
	{
		flog(LOG_WARN, "Impossibile creare buffer sonoro");
		abort_p();
	}
	
	natb shift;
	
	switch(mode & 0x03){
		case 3: shift=2; break;
		case 2:
		case 1: shift=1; break;
		case 0: shift=0; break;
	}
	// shift rappresenta la grandezza di un frame in log2
	// mode = 00 -> modalità 8 bit mono: frame = 1 byte
	// mode = 01 -> modalità 8 bit stereo: frame = 2 byte
	// mode = 10 -> modalità 16 bit mono: frame = 2 byte
	// mode = 11 -> modalità 16 bit stereo: frame = 4 byte

	natl dac1_count = (BUFFERSIZE >> (shift+1));
	// diciamo alla scheda audio di lanciare un'interruzione dopo
	// aver eseguito mezzo buffer
	
	natb ratectrl;
	switch (rate) 
	{
		case 5512: ratectrl=0; break;
		case 11025: ratectrl=1; break;
		case 22050: ratectrl=2; break;
		case 44100: ratectrl=3; break;
	}
	
	natl dac1_size= BUFFERSIZE >> 2;
	
	copia_snd_buffer(puntatore, buffersuono, 0, BUFFERSIZE >> 2);
	
	p_des->canali[1].buffer = buffersuono;
	p_des->canali[1].puntatore_brano = (natb*)puntatore + BUFFERSIZE;
	p_des->canali[1].quanti = size - BUFFERSIZE;
	p_des->canali[1].oldsize=0;
	// salvataggio nel descrittore di canale
	// dell'indirizzo del buffer, il puntatore all'interno del brano
	// quantità dei dati da scrivere
	// e indice all'interno del buffer
	
	sem_wait(p_des->es_mutex);
	p_des->ctrl |= (ratectrl&0x03)<<12;
	outputl(p_des->ctrl, p_des->indreg.control);	
	outputl(0x0000000C, p_des->indreg.mem_page);	
	outputl(dac1_count-1, p_des->indreg.dac1_count);
	outputl(dac1_size-1, p_des->indreg.dac1_size);
	outputl(natl(buffersuono), p_des->indreg.dac1_frame);
	
	p_des->sctrl &= ~(ES1370_P1_LOOP_SEL  | ES1370_P1_SCT_RLD | 0x03 | ES1370_P1_PAUSE);
			// Loop mode; continua a suonare, quando il count dei samples va a zero
				// lancia un'interruzione, se abilitato
			// da tenere basso per specifiche
			// 0x03 serve da maschera per la modalità del canale
			// metto a 0 il bit di pause, che equivale a mettere il canale in modalità
				// play
			
	p_des->sctrl |= (mode & 0x00000003) | ES1370_P1_INT_EN;
			// mode: modalità del canale
			// abilitazione a lanciare interruzioni			
	outputl(p_des->sctrl, p_des->indreg.serial);				
	sem_signal(p_des->es_mutex);
	
	// scrittura nella scheda audio dei parametri
}

// questa funzione serve a eseguire 3 comandi: play, stop e pause
// per ora implementa solo il canale di esecuzione 1
void es1370_ioctl(des_snd * p_des, int channel, int cmd)
{
	sem_wait(p_des->es_mutex);
	switch(channel)
	{
		case 0: break;
		
		case 1:
			p_des->ctrl &= ~(ES1370_DAC1_EN);
			outputl(p_des->ctrl, p_des->indreg.control);
			
			if(cmd != ES1370_STOP)
			{
				natl what=0;
				what |= ES1370_P1_PAUSE;
				if(cmd == ES1370_PAUSE)
				{	p_des->sctrl |= what;
				}
				else
				{
					p_des->sctrl &= ~what;					
				}	
				outputl(p_des->sctrl, p_des->indreg.serial);					
				p_des->ctrl |= (ES1370_DAC1_EN);
			}
			else
				p_des->ctrl &= ~(ES1370_P1_INT_EN);	
							
				outputl(p_des->ctrl, p_des->indreg.control);
			break;
		case 2: break;		
	}
	sem_signal(p_des->es_mutex);
}

// funzione chiamata alla fine della gestisci_int_dac
// quando cioè è stato scritto un buffer di silenzio
void rilascia_dac(int channel)
{
	des_snd * p_des;
	p_des= &ensoniq;	
	
	es1370_ioctl(p_des, channel, ES1370_STOP);	
		
	phys_free(p_des->canali[channel].buffer);	
		//ripulitura buffer
	
	sem_signal(p_des->canali[channel].mutex);
	sem_signal(p_des->canali[channel].sincr);
	flog(LOG_INFO, "Canale %d rilasciato", channel);	
}


// primitive per il modulo utente
extern "C" void c_es1370_play(addr puntatore, natl size, natl rate, natl mode, int channel)
{
	des_snd * p_des;
	p_des= &ensoniq;
	
	if(!p_des->presente)
	{
		flog(LOG_WARN, "Errore nell'acquisizione del canale: scheda audio non presente!");
		abort_p();
	}
	if(channel<=0 || channel>2)
	{
		flog(LOG_WARN, "Errore nell'acquisizione del canale: canale %d inesistente", channel);
		abort_p();
	}
	
	if(channel==1)
	{
		switch(rate)
		{
			case 5512: case 11025:
			case 22050: case 44100: break;
			default:
				flog(LOG_WARN, "Tentativo di eseguire un file con rate non valido per il canale DAC1");
				abort_p();
		}
			// Il canale DAC1 ha un rate fisso: se cerchiamo di eseguire un suono
			// con un rate diverso su quel canale, meglio se non iniziamo neanche a inizializzarlo
	}
	sem_wait(p_des->canali[channel].mutex);
			// semaforo di mutua esclusione sul canale: aspettiamo che
			// il canale venga liberato
	switch(channel)
	{
//		case 0: prepara adc(p_des, rate, mode); 
//				record adc(p_des, ind_fisico, size); break;
		case 1: prepara_dac1(p_des, puntatore, size, rate, mode); 
				es1370_ioctl(p_des, channel, ES1370_RESUME);
				
//		case 2: prepara_dac2(p_des, rate, mode); break;		
	}	
	flog(LOG_INFO, "Acquisito canale %d", channel);
	sem_wait(p_des->canali[channel].sincr);
			// aspettiamo che il brano sia finito (o che venga mandato
			// uno stop anzitempo)
}

extern "C" void c_es1370_sndcmd(int channel, int cmd)
{
	des_snd * p_des;
	p_des= &ensoniq;
	if(!p_des->presente)
	{
		flog(LOG_WARN, "Errore nell'invio dei comandi: scheda audio non presente!");
		abort_p();
	}
	
	if(cmd<0 || cmd > ES1370_PAUSE)
	{	flog(LOG_WARN, "Errore nella sound command");
		abort_p();
	}
	if(cmd!=ES1370_STOP)
		es1370_ioctl(p_des, channel, cmd);
	else
		if(channel!=0)
			rilascia_dac(channel);
			
	// la primitiva può fidarsi dei canali, 
	// tanto in caso siano sbagliati la ioctl non fa nulla
}

////////////////////////////////////////////////////////////////////////////////
//                 Driver UHCI (Universal Host Controller)                    //
////////////////////////////////////////////////////////////////////////////////

#define NUM_FRAME_POINTER 1024
#define ROOT_HUB_PORTS		2

//allineamento dei TD
#define ALIGN_QH	4
#define ALIGN_TD	5

//viene definito un timeout per le operazioni a controllo di programma espresso in cicli di clock
#define TIMEOUT		1000000000

#define USB_CODE		0x000c0300


#define USB_MAXCONFIG		1
#define USB_MAXINTERFACES	3
#define USB_MAXENDPOINTS	3


//Macro per indirizzare i registri interfaccia dell'uhci 

#define	W_USBCMD 0
#define   W_USBCMD_RS  0x0001 /* Run/Stop */
#define   W_USBCMD_HCRESET 0x0002 /* Host reset */
#define   W_USBCMD_GRESET  0x0004 /* Global reset */
#define   W_USBCMD_EGSM  0x0008 /* Global Suspend Mode */
#define   W_USBCMD_FGR  0x0010 /* Force Global Resume */
#define   W_USBCMD_SWDBG  0x0020 /* SW Debug mode */
#define   W_USBCMD_CF  0x0040 /* Config Flag */
#define   W_USBCMD_MAXP  0x0080 /* Max Packet (0 = 32, 1 = 64) */

 #define W_USBSTS 2
#define   W_USBSTS_USBINT  0x0001 /* USB Interrupt */
#define   W_USBSTS_ERROR  0x0002 /* USB Error Interrupt */
#define   W_USBSTS_RD  0x0004 /* Resume Detect */
#define   W_USBSTS_HSE  0x0008 /* Host System Error */
#define   W_USBSTS_HCPE  0x0010 /* Host Controller Process Error */
#define   W_USBSTS_HCH  0x0020 /* HC Halted */

 #define W_USBINTR 4
#define   W_USBINTR_TIMEOUT 0x0001 /* Timeout/CRC Interrupt Enable */
#define   W_USBINTR_RESUME 0x0002 /* Resume interrupt enable */
#define   W_USBINTR_IOC  0x0004 /* Interrupt On Complete enable */
#define   W_USBINTR_SP  0x0008 /* Short packet interrupt enable */

 #define W_FRNUM  6
 #define D_FRBASEADD 8
 #define B_SOFMOD 12

 #define W_PORTSC1 16
 #define W_PORTSC2 18
#define   W_USBPORTSC_CCS 0x0001 /* Current Connect Status ("device present") */
#define   W_USBPORTSC_CSC 0x0002 /* Connect Status Change */
#define   W_USBPORTSC_PE 0x0004 /* Port Enable/Disable */
#define   W_USBPORTSC_PEC 0x0008 /* Port Enable/Disable Change */
#define   W_USBPORTSC_LS 0x0030 /* Line Status */
#define   W_USBPORTSC_RD 0x0040 /* Resume Detect */
#define   W_USBPORTSC_LSDA 0x0100 /* Low Speed Device Attached */
#define   W_USBPORTSC_PR 0x0200 /* Port Reset */
#define   W_USBPORTSC_SUSP 0x1000 /* Suspend */

//macro di utilità per i qh e frame_list_vett

#define UHCI_PTR_TERM		0x0001
#define UHCI_PTR_QH		0x0002
#define UHCI_PTR_DEPTH		0x0004

#define	WAIT_FOR_HOTPLUG	100

//richieste standard
#define USBREQ_SET_ADDRESS		0x0500
#define USBREQ_SET_CONFIGURATION	0x0900
#define USBREQ_GET_DESCRIPTOR		0x0680


#define DES_DEVICE_VALUE	0x0100
#define DES_CONFIGURATION_VALUE	0x0200
#define DES_INTERFACE_VALUE	0x0402
#define DES_ENDPOINT_VALUE	0x0500


//macro di utilità per i td

// per manipolare i bit di _td.cont_st

#define TD_CTRL_SPD  (1 << 29) /* Short Packet Detect */
#define TD_CTRL_C_ERR_MASK (3 << 27) /* Error Counter bits */
#define TD_CTRL_LS  (1 << 26) /* Low Speed Device */
#define TD_CTRL_IOS  (1 << 25) /* Isochronous Select */
#define TD_CTRL_IOC  (1 << 24) /* Interrupt on Complete */
#define TD_CTRL_ACTIVE  (1 << 23) /* TD Active */
#define TD_CTRL_STALLED  (1 << 22) /* TD Stalled */
#define TD_CTRL_DBUFERR  (1 << 21) /* Data Buffer Error */
#define TD_CTRL_BABBLE  (1 << 20) /* Babble Detected */
#define TD_CTRL_NAK  (1 << 19) /* NAK Received */
#define TD_CTRL_CRCTIMEO (1 << 18) /* CRC/Time Out Error */
#define TD_CTRL_BITSTUFF (1 << 17) /* Bit Stuff Error */
#define TD_CTRL_ACTLEN_MASK 0x7ff /* actual length, encoded as n - 1 */

#define TD_CTRL_ANY_ERROR (TD_CTRL_STALLED | TD_CTRL_DBUFERR | \
     TD_CTRL_BABBLE | TD_CTRL_CRCTIME | TD_CTRL_BITSTUFF)

#define uhci_status_bits(ctrl_sts) (ctrl_sts & 0xFE0000)
#define uhci_actual_length(ctrl_sts) ((ctrl_sts) & TD_CTRL_ACTLEN_MASK) /* 1-based */


//per manipolare il bit di validità  di _td.link

#define UHCI_TD_LINK_VALID  0x0001 /*Invalidate next element*/

//per manipolare i bit di _td.token

#define uhci_maxlen(token) ((token) << 21)
#define uhci_toggle(token) ((token) << 19)
#define uhci_endpoint(token) ((token) << 15)
#define uhci_devaddr(token) ((token) << 8)

//Valori dei PID validi

#define PID_OUT  0xe1
#define PID_IN  0x69
#define PID_SETUP 0x2d

//classi usb devices
#define USB_CLASS_PER_INTERFACE		0
#define USB_CLASS_AUDIO			1
#define USB_CLASS_COMM			2
#define USB_CLASS_HID			3
#define USB_CLASS_PHYSICAL		5
#define USB_CLASS_PRINTER		7
#define USB_CLASS_MASS_STORAGE		8
#define USB_CLASS_HUB			9
#define USB_CLASS_DATA			10
#define USB_CLASS_SMART_CARD		11
#define USB_CLASS_CONTENT_SEC		12
#define USB_CLASS_VIDEO			13
#define USB_CLASS_PERSONAL_HEALTH	14
#define USB_CLASS_DIAGNOSTIC		0xdc
#define USB_CLASS_WIRELESS_CONT		0xe0
#define USB_CLASS_MISCELLANEOUS		0xef


#define USB_CLASS_APP_SPEC		0xfe
#define USB_CLASS_VENDOR_SPEC		0xff

//definizione delle strutture dati

//struttura per le richieste standard
//8 byte lunghezza standard
struct control_data{ 
	natw req;
	natw value;
	natw index;
	natw length;
}__attribute__((aligned(8)));


//descrittore di un device generico
struct device_des {
	natb  bLength;
	natb  bDescriptorType;
	natw bcdUSB;
	natb  bDeviceClass;
	natb  bDeviceSubClass;
	natb  bDeviceProtocol;
	natb  bMaxPacketSize0;
	natw idVendor;
	natw idProduct;
	natw bcdDevice;
	natb  iManufacturer;
	natb  iProduct;
	natb  iSerialNumber;
	natb  bNumConfigurations;
};

//descrittore di un endpoint
struct endpoint_des {
	natb  bLength;
	natb  bDescriptorType;
	natb  bEndpointAddress;
	natb  bmAttributes;
	natw  wMaxPacketSize;
	natb  bInterval;
};

//descrittore di un interfaccia
struct interface_des {
	//natb  bLength;
	natb  bDescriptorType;
	natb  bInterfaceNumber;
	natb  bAlternateSetting;
	natb  bNumEndpoints;
	natb  bInterfaceClass;
	natb  bInterfaceSubClass;
	natb  bInterfaceProtocol;
	natb  iInterface;
	endpoint_des endpoint[USB_MAXENDPOINTS];
};

//descrittore di una configurazione
struct config_des {
	natb  bLength;
	natb  bDescriptorType;
	natw wTotalLength;
	natb  bNumInterfaces;
	natb  bConfigurationValue;
	natb  iConfiguration;
	natb  bmAttributes;
	natb  MaxPower;
	interface_des interface[USB_MAXINTERFACES];
};

//struttura che rappresenta un device attaccato al bus usb
struct usb_dev {
	//indirizzo del device
	natl address;
	//descrittore di device
	device_des dev_des;	
	//descrittore di configurazione , interfaccia e endpoints
	config_des config[USB_MAXCONFIG];
	//indica se il device è configurato
	bool configured;
	//indica la porta del root hub a cui è connesso
	natl port;
};

//tipo di struttura
enum type_elem {TD = 0 , QH =1};

//tipo di PID
enum PID {IN , OUT , SETUP};

//quattro doppie parole che fisicamente compongono il TD
struct _td{
	natl link;//link FISICO all'elemento successivo
	natl cont_st; //control&status
	natl token;//packet header
	natl buffer;//link fisico ai dati da trasferire
};

//descrittore td
struct td_des{
	//parte hw del descrittore
	_td values;
	//campi dati che identificano il trasferimento
	addr link_pointer; //link LOGICO all'elemento successivo
	type_elem ty_succ;  //tipo dell'elemento successivo
	//campi dati che identifiano il transferimento
	natw maxlen; //valore massimo possibile 1023 (usb spec.)
	natw endpoint;//endpoint 
	natw dev_addr;//indirizzo del device
	natb td_pid;//packet Identification
	//dati da trasferire
	addr buffer_pointer; //puntatore LOGICO al buffer (grandezza specificata dal campo maxlen)
};

//due doppie parole che fisicamente compongono il QH
struct _qh{
	natl link;
	natl elem;
};

//descrittore qh
struct qh_des{
	//parte hw del descrittore
	_qh values;
	//elemento successivo Horizontal Execution
	addr link_pointer; //successivo elemento
	type_elem ty_succ;  //tipo dell'elemento successivo
	//elemento Vertical execution
	addr elem_link_pointer; //successivo elemento
	type_elem elem_ty_succ;  //tipo dell'elemento successivo
};

//struttura che contiene il frame list vett
struct uhci_framelist {
	natl frame[NUM_FRAME_POINTER];
};

//descrittore di dispositivo UHCI
struct uhci_des{
	//indirizzo base
	ioaddr usbiobase;
	//indirizzo fisico base frame list (la frame list è una pagina di memoria 4KB 1024*4 byte)
	addr frame_list;
	//serie di variabili che indicano lo stato del controllore
	bool configured;//il controllore è configurato?(anche il root hub)
	int last_num_assigned;//ultimo numero assegnato ad una periferica usb
	int num_frames;//numero di frame che devono essere trasferiti in questa sessione
	bool active;//indica se il trasferimento dell'uhci è attivo
	//semafori di mutua esclusione
	natl mutex_cont;
	natl mutex_int;
	natl mutex_bulk;
	//semaforo di sincronizzazione
	natl sinc;
	//struttura per le richieste standard (la lascio in memoria per evitare di allocarla/deallocarla spesso)
	control_data cont;
	//vettore dei frame list
	uhci_framelist* frame_list_vett;
	//struttura dello scheletro dei trasferimenti
	td_des iso;//primo elemento della catena di trasferimenti isocroni (la catena si sviluppa in linea
	//orizzontale in maniera dinamica)
	qh_des* interrupt;//primo elemento della catena di trasferimenti di tipo interrupt (in verticale)
	qh_des* control;//primo elemento della catena di trasferimenti di tipo control (in verticale)
	qh_des* bulk;//primo elemento della catena di trasferimenti di tipo bulk (in verticale)
	//puntatore alle lista di descrittori dei dispositivi collegati a ciascuna porta
	usb_dev* dev_att [ROOT_HUB_PORTS];
};

//puntatore al descrittore uhci
static uhci_des* uhci_att=0;


//definizioni e strutture dati per l'hub driver

//macro per i bit di stato delle porte
#define WPORTSTS_PC	0 //port connection
#define WPORTSTS_PE	1 //port enable
#define WPORTSTS_PS	2 //port suspend
#define WPORTSTS_PR	4 //port reset
#define WPORTSTS_LS	9 //low speed device

//macro per i bit di status change delle porte
#define WPORTC_PC	0 //port connection change
#define WPORTC_PE	1 //port enable change
#define WPORTC_PS	2 //port suspend change
#define WPORTC_PR	4 //port reset change (reset processing complete)

//tipo del descrittore per gli hub
#define HUB_DESCRIPTOR	0x29

//richieste
#define HUB_GET_DESCRIPTOR 0x06A0
#define HUB_GET_STATUS	0x00A3
#define HUB_SET_PORT_FEATURE 0x0323
#define HUB_CLEAR_PORT_FEATURE 0x0123

//features
#define PORT_POWER 0x08
#define C_PORT_CONNECTION 16

struct hub_port_status {
	natw wPortStatus;
	natw wPortChange;
};

struct hub_des {
	natb bDescLength;
	natb bDescriptorType;
	natb bNbrPorts;
	natw wHubCharacteristics;
	natb PwrOn2PwrGood;
	natb bHubContrCurrent;
	//i due campi che seguono sono due bitmap con un bit per ogni porta fino ad un massimo di 255 porte + eventuali altri bit aggiuntivi
	//nel nostro caso le porte sono al più 8
	natb DeviceRemovable;
	natb PortPwrCtrlMask;
};

struct hub {
	//descrittore specifico
	hub_des desc;
	//stato delle porte dell'hub
	hub_port_status status;
	//identificatore del processo che gestisce l'hub
	natl proc_id;
	//l'hub è presente
	bool present;
	//(semaforo) il processo è terminato
	natl term;
	//numero della porta a cui è connesso l'hub
	natl port;
	//bitmap che viene restituita facendo il polling dello status change endpoint
	natw bitmap;
};

//puntatore alla lista degli hub (da spostare in uhci_des)
hub* hub_att;


//funzioni che operano sui registri del controllore

//invia il global reset all'uhci
void uhci_reset()
{
	outputw(W_USBCMD_GRESET, (ioaddr)uhci_att->usbiobase + W_USBCMD);
	//devo aspettare almeno 50 millisecondi
	delay(1);
	outputw(0, (ioaddr)uhci_att->usbiobase + W_USBCMD);
	//devo attendere almeno 10 millisecondi
	delay(1);
	flog(LOG_INFO, "inviato UHCI global reset");
}


//invia il segnale di host controller reset
void uhci_start()
{
	natw reg=0;
	bool stato=true;
	outputw(W_USBCMD_HCRESET, (ioaddr)uhci_att->usbiobase + W_USBCMD);
	do{
		inputw((ioaddr)uhci_att->usbiobase + W_USBCMD,reg);
		if((reg & W_USBCMD_HCRESET)==0)
			stato=false;
	}while(stato);
	flog(LOG_INFO, "UHCI attivato");
}


//restituisce in connected la word che definisce lo stato della porta port
void port_connect(natl port, natw& connected) {
	ioaddr port_addr = uhci_att->usbiobase + W_PORTSC1 + 2 * port;
	natw prev;
	inputw(port_addr,prev);
	connected=prev;
		
}
//resetta il bit 1 del registro di stato della porta port (status change bit)
void change_status_reset (natl port) {

	ioaddr port_addr = uhci_att->usbiobase + W_PORTSC1 + 2 * port;
	natw prev;
	inputw(port_addr,prev);
	prev=prev | 0x0002; //per resettare il bit di status change bisogna scriverci 1
	outputw(prev, port_addr);
}


//fa partire i trasferimenti del controllore UHCI false=STOP true=RUN

void start_uhci_transfer(bool act)
{
	natw prev;
	inputw((ioaddr)uhci_att->usbiobase+W_USBCMD,prev);
	if(act==true) 
		prev=prev | 1;
	else
		prev=prev & 0xfffe;
	outputw(prev,(ioaddr)uhci_att->usbiobase+W_USBCMD);
	uhci_att->active=act;
}

//abilita o disabilita il controllore a interrompere 

void usb_interrupt_enable(bool en) {

	if(en==true)
		outputw((natw)0x000f,(ioaddr)uhci_att->usbiobase+W_USBINTR);
	if(en==false)
		outputw((natw)0x0000,(ioaddr)uhci_att->usbiobase+W_USBINTR);

} 

//funzioni per manipolare i QH


//setta il campo link del QH (puntatore alla coda successiva)
void set_qh_link(qh_des* des, addr succ)
{
	natl add=(natl)succ;
	if(succ!=0)
	{
		des->link_pointer=succ;
		des->ty_succ=QH;
		add=add | UHCI_PTR_QH;
	}
	else
	{
		des->link_pointer=0;
		des->ty_succ=QH;
		add= UHCI_PTR_TERM;//va messo a 1 per invalidare il link
	
	}
	des->values.link=add;
}

//setta il campo element del QH (puntatore al primo elemento della coda)
void set_qh_elem(qh_des* des,addr succ,type_elem next)
{
	if(next==QH)
	{
		natl add=(natl)succ;
		if(succ!=0)
		{
			des->elem_link_pointer=succ;
			des->elem_ty_succ=next;
			if(next==QH)
				add=add | UHCI_PTR_QH;
		}
		else
		{
			des->elem_link_pointer=0;
			des->elem_ty_succ=QH;
			add= UHCI_PTR_TERM;//va messo a 1 per invalidare il link
		
		}
		des->values.elem=add;
	}
	else
	{
		natl add=(natl)succ;
		if(succ!=0)
		{
			des->elem_link_pointer=succ;
			des->elem_ty_succ=next;
			if(next==QH)
				add=add | UHCI_PTR_QH;
		}
		else
		{
			des->elem_link_pointer=0;
			des->elem_ty_succ=QH;
			add= UHCI_PTR_TERM;//va messo a 1 per invalidare il link
	
		}
		des->values.elem=add;
	}
}


//funzioni per manipolare i TD

//ritorna lo stato di un TD
int get_td_status(td_des* td)
{
	natl status=0;
	status = (td->values.cont_st >> 16) & 0xff;
	return status;
}


//libera un TD o una catena di TD
void td_free(td_des* td)
{	td_des* del;
	while(td!=0)
	{	del=td;
		td=(td_des*)td->link_pointer;
		memset(del, 0, sizeof(td_des));
		phys_free(del);
		}
}

//setta il campo link del TD

void set_td_link(td_des* des,addr succ,bool bf=false)
{
	natl add=(natl)succ;
	if(succ!=0)
	{
		des->link_pointer=succ;
		des->ty_succ=TD;
		if(bf==false)
		{
			add &= (natl)0xfffffff0;
			add |= UHCI_PTR_DEPTH;
		}
	}
	else
	{
		des->link_pointer=0;
		des->ty_succ=QH;
		add= UHCI_PTR_TERM;//va messo a 1 per invalidare il link
	
	}
	des->values.link=add;
}

//setta il campo control&status del TD
void set_td_cont_st(td_des* des,bool isc_sel,bool ioc, natl port)
{
	natl att=0;
	natw reg=0;
	port_connect(port, reg); 
	if((reg & W_USBPORTSC_LSDA)==1) //dispositivo a bassa velocità
		att |=  (TD_CTRL_LS) | (TD_CTRL_ACTIVE)   |   (TD_CTRL_SPD)       |    (TD_CTRL_C_ERR_MASK);
	else
		att |=  (TD_CTRL_ACTIVE)   |   (TD_CTRL_SPD)       |    (TD_CTRL_C_ERR_MASK);
	if(isc_sel==false)
		att=att & ~(TD_CTRL_IOS);
	else
		att=att | (TD_CTRL_IOS);
	if(ioc==false)
		att=att & ~(TD_CTRL_IOC);
	else
		att=att | (TD_CTRL_IOC);
	des->values.cont_st=att;
}

//setta il campo token del td
void set_td_token(td_des* des,natl maxlen,int data_toggle,natl endpoint,natl dev_addr,natl tdpid)
{
	natl att=0;
	att = (0 & 0x0007ff00);
	des->maxlen=maxlen-1;
	des->endpoint=endpoint;
	des->dev_addr=dev_addr;
	des->td_pid=tdpid;
	att = att | uhci_maxlen(maxlen-1);
	if(data_toggle==1)
		att=att | uhci_toggle(1);	//determina quale tipo di data PID ci si aspetta
	att = att | uhci_endpoint(endpoint);
	att = att | uhci_devaddr(dev_addr);
	att |= tdpid;	//IN, OUT o SETUP
	
	des->values.token=att;
}

//setta il campo buffer del td
void set_td_buffer(td_des* des,addr buff)
{
	des->values.buffer=(natl)buff; //link fisico
	des->buffer_pointer=buff; //link logico
}

//crea una catena di td restituendone i puntatori al primo e all'ultimo
td_des* create_td_chain(natl max_packet_length,void* buffer,natl length,natl tpid,natl dev,natl endpoint,td_des*& last,int& toggle, natl port)
{
	int num=(length/max_packet_length);
	if((length % max_packet_length)!=0)
		num++;
	td_des* ini;
	td_des* fin;
	addr punt=(addr)buffer;
	int i=0;
	for(;i<num;i++)
	{
		length=length-max_packet_length;
		td_des* des=(td_des*)phys_alloc(sizeof(td_des),ALIGN_TD);
		memset(des,0,sizeof(td_des));
		set_td_cont_st(des,false,false, port);
		set_td_token(des,max_packet_length,(i+toggle)%2,endpoint,dev,tpid);
		set_td_buffer(des,punt);
		set_td_link(des,0);
		if(i==0)
			ini=des;
		else
		{
			if(i!=(num-1))
				set_td_link(fin,des,true);
			else
				set_td_link(fin,des);
		}
		fin=des;
		natl temp=(natl)punt + max_packet_length;
		punt=(addr) temp;
	}
	toggle=num%2; 
	last=fin;
	return ini;
}

//crea una lista di td per le richieste standard restituendo un puntatore al primo
td_des* create_control_message(natw action, natw dev, natw value, addr buffer,natl size, natl port, natl ind=0)
{	td_des* stat;
	int toggle;
	td_des* last;
	td_des* int_ch;
	memset(&uhci_att->cont,0,sizeof(control_data));
	uhci_att->cont.req=action;
	td_des* td_base=(td_des*)phys_alloc(sizeof(td_des),ALIGN_TD);
	memset(td_base,0,sizeof(td_des));
	switch(action)
	{
		case USBREQ_SET_ADDRESS:
			uhci_att->cont.value=value;
			uhci_att->cont.index=0;
			uhci_att->cont.length=0;
			set_td_link(td_base,0);
			set_td_cont_st(td_base,false,false, port);
			set_td_token(td_base,8,0,0,0,PID_SETUP);
			set_td_buffer(td_base,(addr)&uhci_att->cont);
			last=td_base;
			break;
		case USBREQ_SET_CONFIGURATION:
			uhci_att->cont.value=value;
			uhci_att->cont.index=0;
			uhci_att->cont.length=0;
			set_td_link(td_base,0);
			set_td_cont_st(td_base,false,false, port);
			set_td_token(td_base,8,0,0,dev,PID_SETUP);
			set_td_buffer(td_base,(addr)&uhci_att->cont);
			last=td_base;
			break;
		case USBREQ_GET_DESCRIPTOR:
		case HUB_GET_DESCRIPTOR:
		case HUB_GET_STATUS:
			uhci_att->cont.value=value;
			uhci_att->cont.index=ind;
			uhci_att->cont.length=size;
			set_td_link(td_base,0);
			set_td_cont_st(td_base,false,false, port);
			set_td_token(td_base,8,0,0,dev,PID_SETUP);
			set_td_buffer(td_base,(addr)&uhci_att->cont);
			toggle=1;
			int_ch=create_td_chain(8,buffer,size,PID_IN,dev,0,last,toggle, port);
			set_td_link(td_base,int_ch,true);
			break;
		case HUB_SET_PORT_FEATURE:
		case HUB_CLEAR_PORT_FEATURE:
			uhci_att->cont.value=value;
			uhci_att->cont.index=ind;
			uhci_att->cont.length=0;
			set_td_link(td_base,0);
			set_td_cont_st(td_base,false,false, port);
			set_td_token(td_base,8,0,0,dev,PID_SETUP);
			set_td_buffer(td_base,(addr)&uhci_att->cont);
			last=td_base;
			break;
	}
	if(action==USBREQ_GET_DESCRIPTOR|| action==HUB_GET_DESCRIPTOR || action==HUB_GET_STATUS)
	{
		stat=(td_des*)phys_alloc(sizeof(td_des),ALIGN_TD);
		memset(stat,0,sizeof(td_des));
		set_td_link(stat,0);
		set_td_cont_st(stat,false,true, port);
		set_td_token(stat,0,toggle,0,dev,PID_OUT);
		set_td_buffer(stat,0);
		set_td_link(last,stat,true);
	}
	else
	{
		stat=(td_des*)phys_alloc(sizeof(td_des),ALIGN_TD);
		memset(stat,0,sizeof(td_des));
		set_td_link(stat,0);
		set_td_cont_st(stat,false,true, port);
		set_td_token(stat,0,1,0,dev,PID_IN);
		set_td_buffer(stat,0);
		set_td_link(last,stat);
	}
	return td_base;
}

//funzioni per il debug

//stampa i campi di una catena di descrittori
void dbg_td_chain(td_des* td)
{
	int i=0;
	while(td!=0)//stampo lo stato di tutti i descrittori di trasferimento
	{	flog(LOG_DEBUG,"%x", td->values.link);
		flog(LOG_DEBUG,"%x", td->values.cont_st);
		flog(LOG_DEBUG,"%x", td->values.token);
		flog(LOG_DEBUG,"%x", td->values.buffer);
		flog(LOG_DEBUG,"link %x", td->link_pointer);
		flog(LOG_DEBUG,"\n");
		td=(td_des*)td->link_pointer;
		i++;
	}
}

//stampa i campi del QH
void dbg_qh(qh_des* qh)
{
	flog(LOG_DEBUG, "Campo Link: %d \n", qh->values.link);
	flog(LOG_DEBUG,"campo Elem: %d \n", qh->values.elem);
}

//stampa a schermo il contenuto dei registri del controllore
void uhci_dbg(){
	natw reg=0;
	inputw((ioaddr)uhci_att->usbiobase + W_USBSTS,reg);
	flog(LOG_DEBUG,"stato: %d", reg);
	inputw((ioaddr)uhci_att->usbiobase + W_USBCMD,reg);
	flog(LOG_DEBUG,"comando: %d", reg);
	inputw((ioaddr)uhci_att->usbiobase + W_PORTSC1,reg); 
	flog(LOG_DEBUG,"port 1: %d", reg);
	inputw((ioaddr)uhci_att->usbiobase + W_PORTSC2,reg); 
	flog(LOG_DEBUG,"port 2: %d", reg);
}

//funzioni di inizializzazione

//inizializza lo scheletro dei descrittori di trasferimento con le tre QH per i tre tipi di trasferimento
//le tre QH devono formare un loop per ottimizzare il tempo!
bool uhci_skel_init()
{
	if (! (uhci_att->interrupt=(qh_des*)phys_alloc(sizeof(qh_des), ALIGN_QH)) ) {
		flog(LOG_ERR, "uhci: heap fisico insufficiente");
		goto error;
	}
	if (! (uhci_att->control=(qh_des*)phys_alloc(sizeof(qh_des), ALIGN_QH)) ) {
		flog(LOG_ERR, "uhci: heap fisico insufficiente");
		goto error1;
	}
	if (! (uhci_att->bulk=(qh_des*)phys_alloc(sizeof(qh_des), ALIGN_QH)) ) {
		flog(LOG_ERR, "uhci: heap fisico insufficiente");
		goto error2;
	}
	set_qh_link(uhci_att->interrupt,uhci_att->control);
	set_qh_link(uhci_att->control,uhci_att->bulk);
	set_qh_link(uhci_att->bulk,uhci_att->interrupt);//bandwidth reclamation LOOP
	//set_qh_link(&uhci_att->bulk,0);
	//dbg_qh(uhci_att->interrupt);
	//dbg_qh(uhci_att->control);
	//dbg_qh(uhci_att->bulk);
	set_qh_elem(uhci_att->interrupt,0,TD);
	set_qh_elem(uhci_att->control,0,TD);
	set_qh_elem(uhci_att->bulk,0,TD);
	//inizializzo i semafori di mutua esclusione per i qh
	if ( (uhci_att->mutex_bulk=sem_ini(1)) == 0xFFFFFFFF ) {
		flog(LOG_WARN, "uhci: semafori insufficienti");
		goto error2;
	}
	if ( (uhci_att->mutex_cont=sem_ini(1)) == 0xFFFFFFFF ) {
		flog(LOG_WARN, "uhci: semafori insufficienti");
		goto error2;
	}
	if ( (uhci_att->mutex_int=sem_ini(1)) == 0xFFFFFFFF ) {
		flog(LOG_WARN, "uhci: semafori insufficienti");
		goto error2;
	}
	for(int i=0;i<1024;i++)
		uhci_att->frame_list_vett->frame[i]=(natl)uhci_att->interrupt | 2;
	return true;

error2: phys_free(uhci_att->control);
error1:	phys_free(uhci_att->interrupt);
error:	return false;
}

//funzioni per la gestione dei dispositivi

//abilita la porta port del root hub e invia il segnale di reset al device connesso
void port_enable(natl port)
{
	natw prev;
	ioaddr port_addr = uhci_att->usbiobase + W_PORTSC1 + 2 * port;
	inputw(port_addr,prev);
	prev=prev | W_USBPORTSC_PR;
	outputw(prev,port_addr); //invio il segnale di reset
	prev=prev & ~W_USBPORTSC_PR;
	delay(1);
	outputw(prev,port_addr);
	delay(1);
	inputw(port_addr,prev);
	prev=prev | W_USBPORTSC_PE;
	outputw(prev,port_addr); //abilito la porta
	delay(1);
	inputw(port_addr,prev);
	if(!(prev & W_USBPORTSC_PE))
	{	prev=prev | W_USBPORTSC_PE;
		outputw(prev,port_addr);
		delay(2);
	}
}
//disabilita la porta port se non viene disattivata dal controller uhci
void port_disable(natl port)
{
	natw prev;
	ioaddr port_addr = uhci_att->usbiobase + W_PORTSC1 + 2 * port;
	inputw(port_addr,prev);
	if(prev & W_USBPORTSC_PE) {
		prev=prev | W_USBPORTSC_PE;
		outputw(prev,port_addr); //disabilito la porta
		delay(1);
		}

}

//funzioni per i trasferimenti di controllo

//assegna un indirizzo al device
void set_address(natl port){	
	td_des* td=create_control_message(USBREQ_SET_ADDRESS,0,uhci_att->dev_att[port]->address,0,0, port);
	//dbg_td_chain(td);
	sem_wait(uhci_att->mutex_cont);
	set_qh_elem(uhci_att->control,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->control,0,TD);
	sem_signal(uhci_att->mutex_cont);
	td_free(td);
}

//assegna una configurazione al device
void set_configuration(natl conf, natl port)
{	td_des* td=create_control_message(USBREQ_SET_CONFIGURATION,uhci_att->dev_att[port]->address,conf,0,0, port);
	//dbg_td_chain(td);
	sem_wait(uhci_att->mutex_cont);
	set_qh_elem(uhci_att->control,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->control,0,TD);
	sem_signal(uhci_att->mutex_cont);
	td_free(td);
}

//riempie i descrittori del device
void get_device_desc(natl port)
{	//recupero il descrittore del device
	td_des* td=create_control_message(USBREQ_GET_DESCRIPTOR,uhci_att->dev_att[port]->address,DES_DEVICE_VALUE,&(uhci_att->dev_att[port]->dev_des),sizeof(device_des), port);
	//dbg_td_chain(td);
	sem_wait(uhci_att->mutex_cont);
	set_qh_elem(uhci_att->control,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->control,0,TD);
	sem_signal(uhci_att->mutex_cont);
	td_free(td);
	delay(1);
	
	//recupero il descrittore della configurazione (al massimo ce n'è una sola), di interfaccia e di endpoint
	td=create_control_message(USBREQ_GET_DESCRIPTOR,uhci_att->dev_att[port]->address,DES_CONFIGURATION_VALUE,&(uhci_att->dev_att[port]->config),sizeof(config_des), port);
	//dbg_td_chain(td);
	sem_wait(uhci_att->mutex_cont);
	set_qh_elem(uhci_att->control,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->control,0,TD);
	sem_signal(uhci_att->mutex_cont);
	td_free(td);

}


//recupera il driver specifico per gli hub
void get_hub_des(natl port) {

	hub_att->port=port;
	//recupero il descrittore specifico degli hub
	td_des* td=create_control_message(HUB_GET_DESCRIPTOR,uhci_att->dev_att[port]->address,HUB_DESCRIPTOR, &(hub_att->desc),sizeof(hub_des),port);
	//dbg_td_chain(td);
	sem_wait(uhci_att->mutex_cont);
	set_qh_elem(uhci_att->control,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->control,0,TD);
	sem_signal(uhci_att->mutex_cont);
	td_free(td);
	
}

//recupera lo stato della porta hub_port
void get_hub_port_status(natl hub_port) {

	//recupero il descrittore specifico degli hub
	td_des* td=create_control_message(HUB_GET_STATUS,uhci_att->dev_att[hub_att->port]->address,0, &(hub_att->status),4,hub_att->port,hub_port);
	//dbg_td_chain(td);
	sem_wait(uhci_att->mutex_cont);
	set_qh_elem(uhci_att->control,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato0
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->control,0,TD);
	sem_signal(uhci_att->mutex_cont);
	td_free(td);
	
}

//funzione per il polling dello status change endpoint degli hub
void poll_status_change() {
	td_des* stat;
	int toggle;
	natl endpoint=0xf & uhci_att->dev_att[hub_att->port]->config[0].interface[0].endpoint[0].bEndpointAddress;
	td_des* td= create_td_chain(8, &hub_att->bitmap, 8, PID_IN, uhci_att->dev_att[hub_att->port]->address, endpoint, stat, toggle, hub_att->port);
	sem_wait(uhci_att->mutex_int);
	set_qh_elem(uhci_att->interrupt,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->interrupt,0,TD);
	sem_signal(uhci_att->mutex_int);
	td_free(td);
}

//azzera il bit di status change
void hub_port_clear(natl hub_port) {
	td_des* td=create_control_message(HUB_CLEAR_PORT_FEATURE,uhci_att->dev_att[hub_att->port]->address, C_PORT_CONNECTION,0,0, hub_att->port, hub_port);
	//dbg_td_chain(td);
	sem_wait(uhci_att->mutex_cont);
	set_qh_elem(uhci_att->control,td,TD);
	//dbg_qh(uhci_att->control);
	//attendo che il trasferimento sia completato
	sem_wait(uhci_att->sinc);
	set_qh_elem(uhci_att->control,0,TD);
	sem_signal(uhci_att->mutex_cont);
	td_free(td);
}


//stampa nel log di sistema il tipo di periferica connessa al bus
const char* usb_get_type(int type)
{
	const char *ret = "sconosciuto";	
	switch(type)
	{
		case USB_CLASS_AUDIO:
			ret = "audio device";
			break;
		case USB_CLASS_COMM:
			ret = "communication device";
			break;
		case USB_CLASS_HID:
			ret = "human interface device";
			break;
		case USB_CLASS_PHYSICAL:
			ret = "physical interface device";
			break;
		case USB_CLASS_PRINTER:
			ret = "printer device";
			break;
		case USB_CLASS_MASS_STORAGE:
			ret = "mass storage device";
			break;
		case USB_CLASS_HUB:
			ret = "hub device";
			break;
		case USB_CLASS_DATA:
			ret = "data device";
			break;
		case USB_CLASS_SMART_CARD:
			ret = "smart card device";
			break;
		case USB_CLASS_CONTENT_SEC:
			ret = "content security device";
			break;
		case USB_CLASS_VIDEO:
			ret = "video device";
			break;
		case USB_CLASS_PERSONAL_HEALTH:
			ret = "personal health device";
			break;
		case USB_CLASS_DIAGNOSTIC:
			ret = "diagnostic device";
			break;
		case USB_CLASS_WIRELESS_CONT:
			ret = "wireless controller device";
			break;
		case USB_CLASS_MISCELLANEOUS:
			ret = "miscellaneous device";
			break;
		case USB_CLASS_APP_SPEC:
			ret = "application specific device";
			break;
		case USB_CLASS_VENDOR_SPEC:
			ret = "vendor specific device";
			break;
		default :
			break;
	}
	return ret;
}

//ricerca il tipo di periferica nei descrittori
void print_dev_des(natl port)
{
	const char *type;
	if(uhci_att->dev_att[port]->dev_des.bDeviceClass!=0) //se è uguale a 0, ogni interfaccia specifica il proprio class code
		type = usb_get_type(uhci_att->dev_att[port]->dev_des.bDeviceClass);
	else
		type = usb_get_type(uhci_att->dev_att[port]->config[0].interface[0].bInterfaceClass);
	flog(LOG_INFO,"periferica di tipo: %s", type);
}


//hub driver
void hub_driver (int a) {
	hub_att->term=sem_ini(0);
	hub_att->port=a;
	hub_att->present=true;
	//recupero il descrittore
	get_hub_des(hub_att->port);
	flog(LOG_INFO, "porte: %d", hub_att->desc.bNbrPorts);
	bool start=true;
	while(hub_att->present) {
		poll_status_change();
		for(int i=1; i<=hub_att->desc.bNbrPorts; i++) {
				if((hub_att->bitmap & (1<<(i)))!=0) {
					get_hub_port_status(i);
					if(((hub_att->status.wPortStatus & 1) && (hub_att->status.wPortChange & 1)) || ((hub_att->status.wPortStatus & 1) && start))
						flog(LOG_INFO, "dispositivo connesso sulla porta %d dell'hub", i);
					if((!(hub_att->status.wPortStatus & 1) && (hub_att->status.wPortChange & 1)) || (!(hub_att->status.wPortStatus & 1) && start))
						flog(LOG_INFO, "dispositivo disconnesso sulla porta %d dell'hub", i);
					hub_port_clear(i);
					}
			delay(1);
		}
		start=false;
		delay(WAIT_FOR_HOTPLUG*2); 
	}
	//segnalo che il processo sta terminando, è quindi possibile eliminare le strutture dati dell'hub
	sem_signal(hub_att->term);
	terminate_p();
}


//configura un dispositivo appena connesso
void configure_device(natl port) {
	//abilito la porta
	port_enable(port);
	//alloco il descrittore
	uhci_att->dev_att[port]=(usb_dev*)phys_alloc(sizeof(usb_dev));
	memset(uhci_att->dev_att[port],0,sizeof(usb_dev));

	uhci_att->dev_att[port]->address=0;
	uhci_att->dev_att[port]->configured=false;
	//recupero i descrittori	
	get_device_desc(port);
	
	//assegno il primo indirizzo disponibile al device
	uhci_att->last_num_assigned++;
	uhci_att->dev_att[port]->address=uhci_att->last_num_assigned;
	//assegno il primo indirizzo disponibile
	set_address(port);
	//attendo 10 ms per far si che il device acquisisca l'indirizzo
	delay(1);
	//assegno la prima configurazione al device	
	set_configuration(1, port);
	//attendo 10 ms per far si che il device acquisisca la configurazione
	delay(1);
	//il controllore è stato configurato
	uhci_att->dev_att[port]->configured=true;
	//stampo il tipo di device attaccato
	print_dev_des(port);
	
	if(uhci_att->dev_att[port]->dev_des.bDeviceClass==USB_CLASS_HUB) {
		//alloco il descrittore specifico
		hub_att=(hub*)phys_alloc(sizeof(hub));
		memset(hub_att, 0, sizeof(hub));
		hub_att->proc_id=activate_p(hub_driver, port, PRIO, LIV);
		}
}

//rimuove un dispositivo appena disconnesso
void remove_device(natl port) {
	//disabilito la porta
	port_disable(port);
	//se il dispositivo è un hub elimino il suo descrittore specifico
	if(uhci_att->dev_att[port]->dev_des.bDeviceClass==USB_CLASS_HUB) {
	//faccio terminare il processo
	hub_att->present=false;
	sem_wait(hub_att->term);
	phys_free(hub_att);
	}
	//elimino il descrittore di dispositivo
	phys_free(uhci_att->dev_att[port]);
}

//effettua il polling sulle porte del root hub
void polling (int a) { 
	natw status;
	bool start=true;
	while(true){
		for(natl i=0; i<ROOT_HUB_PORTS; ++i) {
			port_connect(i, status);
			natw connected=status & 1;
			natw change=status & 2;
			if(start) {
				if(connected) {
					flog(LOG_INFO, "dispositivo connesso sulla porta %d", i+1);
					configure_device(i);				
					}
				if(!connected)
					flog(LOG_INFO, "nessun dispositivo connesso sulla porta %d", i+1);
				}

			if(change && connected && !start) {
				flog(LOG_INFO, "dispositivo connesso sulla porta %d", i+1);
				configure_device(i);
				}
			if(change && !connected &&!start) {
				flog(LOG_INFO, "dispositivo disconnesso sulla porta %d", i+1);
				remove_device(i);
				}
			if (change)
				change_status_reset(i);
			}
		start=false;
	delay(WAIT_FOR_HOTPLUG);
	}
		terminate_p();
	
}

//corpo del processo esterno
void usb_int (int a) {
	
	natw reg=0;
	for(;;) {
		inputw((ioaddr)uhci_att->usbiobase+W_USBSTS, reg);
		
		if((W_USBSTS_ERROR & reg)!=0) { //l'interruzione è dovuta ad un errore nel trasferimento
			flog(LOG_WARN, "trasferimento dati fallito");
			reg=reg & 2;		//resetto il bit del registro di controllo che indica perchè è avvenuta l'interruzione scrivendoci 1
			}
		if((W_USBSTS_USBINT & reg)!=0) { //il trasferimento è avvenuto in modo corretto
			//flog(LOG_INFO, "ok");
			reg=reg & 1;
			}
		outputw(reg, (ioaddr)uhci_att->usbiobase+W_USBSTS);
		//segnalo che il trasferimento è stato completato
		sem_signal(uhci_att->sinc);
		wfi();
	}

}

//inizializzazione
void uhci_init(natw l) {
	addr phys_frame_list;
	ioaddr base;
	natb irq;

	//alloco lo spazio per il descrittore
	if (uhci_att) {
		flog(LOG_WARN, "troppi controllori UHCI");
		goto error1;
	}

	if (!(uhci_att=(uhci_des*)phys_alloc(sizeof(uhci_des)))) {
		flog(LOG_WARN, "uhci: heap virtuale insufficiente");
		goto error1;
	}
	memset(uhci_att,0,sizeof(uhci_des));
	
	//trovo l'indirizzo del registro base del controllore
	base=(ioaddr)pci_read(l, 0x20, 2);
	base=base & 0xfffffffc;
	uhci_att->usbiobase=base;
	
	//trovo il piedino su cui viene mandata l'interruzione
	irq=pci_read(l, 0x3c, 1);

	//inizializzo il semaforo di sincronizzazione
	if ( (uhci_att->sinc=sem_ini(0)) == 0xFFFFFFFF) {
		flog(LOG_WARN, "uhci: semafori insufficienti");
		goto error2;
	}
	
	//creo il framelist
	if (! (uhci_att->frame_list_vett=(uhci_framelist*)mem_alloc(sizeof(uhci_framelist),12)) ) {
		flog(LOG_WARN, "uhci: heap virtuale insufficiente");
		goto error2;
	}
	if (!resident(uhci_att->frame_list_vett, sizeof(uhci_framelist), true)) {
		flog(LOG_WARN, "uhci: impossibile rendere residente la framelist");
		goto error3;
	}
	//cerco l'indirizzo fisico del frame list
	phys_frame_list=trasforma(uhci_att->frame_list_vett);
	//la struttura è inizializzata ad 1 per invalidare tutte le voci
	memset(uhci_att->frame_list_vett->frame,1,(sizeof(natl)*NUM_FRAME_POINTER));
	//inizializzo i registri dell'uhci
	uhci_reset();
	uhci_start();
	//il numero zero è già assegnato di default (spec. USB)
	uhci_att->last_num_assigned=0;
	//inizializzo lo scheletro delle strutture dati dell'uhci
	if (!uhci_skel_init())
		goto error3;
	//abilito le interruzioni sul controllore uhci
	usb_interrupt_enable(true);
	//inizializzo il frame base register
	outputl((natl)phys_frame_list,(ioaddr)uhci_att->usbiobase + D_FRBASEADD);
	//segnalo che il controllore è configurato
	outputw((natw)0, (ioaddr)uhci_att->usbiobase + W_FRNUM);
	outputw((natw)/*W_USBCMD_SWDBG | */W_USBCMD_RS | W_USBCMD_CF | W_USBCMD_MAXP,(ioaddr)uhci_att->usbiobase + W_USBCMD);
	uhci_att->configured=true;
	if (activate_p(polling, 0, PRIO, LIV) == 0xFFFFFFFF) {
		flog(LOG_WARN, "uhci: impossibile creare processo polling");
		goto error3;
	}
	if (activate_pe(usb_int, 0, PRIO, LIV, irq) == 0xFFFFFFFF) {
		flog(LOG_WARN, "uhci: impossibile creare processo esterno");
		goto error3;
	}
	flog(LOG_INFO, "UHCI configurato correttamente");
	return;

error3:	resident(uhci_att->frame_list_vett, sizeof(uhci_framelist), false);
	mem_free(uhci_att->frame_list_vett);
error2: phys_free(uhci_att);
error1: return;
}
	

////////////////////////////////////////////////////////////////////////////////
//                 INIZIALIZZAZIONE DEL SOTTOSISTEMA DI I/O                   //
////////////////////////////////////////////////////////////////////////////////

natw pci_loc(natb bus, natb dev, natb fun)
{
	return (bus << 8L) | (dev << 3L) | fun;
}

natb pci_get_bus(natw l)
{
	return (l >> 8) & 0xFF;
}

natb pci_get_dev(natw l)
{
	return (l >> 3) & 0x1F;
}

natb pci_get_fun(natw l)
{
	return l & 0x7;
}


struct pci_driver {
	natw vendorID;
	natw deviceID;
	void (*init)(natw l);
};

void pci_no_init(natw l)
{}

pci_driver pci_drivers[] = {
	{ 0x8086, 0x1237, pci_no_init }, 	// host bridge
	{ 0x8086, 0x7000, pci_no_init },	// ISA bridge
	{ 0x8086, 0x7010, pci_no_init },	// IDE controller
	{ 0x8086, 0x7020, uhci_init   },
	{ 0x8086, 0x7113, pci_no_init }, 	// PCI bridge
	{ 0x1013, 0x00b8, pci_no_init },	// VGA controller
	{ 0x10ec, 0x8139, eth_init    },
	{ ES_VENDOR_ID, ES_DEVICE_ID, ensoniq_init },
	{ 0xFFFF, 0xFFFF, 0 }
};

const natl PCI_DRIVERS = sizeof(pci_drivers) / sizeof(pci_driver);
const natl PCI_NOTFOUND = PCI_DRIVERS;

natl pci_find_driver(natw vendorID, natw deviceID)
{
	natl r = 0;
	while (r < PCI_DRIVERS) {
		if (pci_drivers[r].vendorID == vendorID &&
		    pci_drivers[r].deviceID == deviceID)
			break;
		r++;
	}
	return r;
}

bool pci_init()
{
	for (natb bus = 0; bus < 255; bus++) {
		for (natb dev = 0; dev < 32; dev++) {
			for (natb fun = 0; fun < 8; fun++) {
				natw vendorID, deviceID;
				natw l = pci_loc(bus, dev, fun);

				vendorID = pci_read(l, 0, 2);

				if (vendorID == 0xFFFF)
					continue;

				deviceID = pci_read(l, 2, 2);
				natl r = pci_find_driver(vendorID, deviceID);
				if (r == PCI_NOTFOUND)
					flog(LOG_WARN, "pci %2x.%2x.%2x (%4x:%4x): nessun driver", 
							pci_get_bus(l), pci_get_dev(l), pci_get_fun(l),
							vendorID, deviceID);
				else
					(*pci_drivers[r].init)(l);
			}
		}
	}
	return true;
}


/*
Format of Bytes

There are 4 formats possible for the date/time RTC bytes:

    * Binary or BCD Mode
    * Hours in 12 hour format or 24 hour format 

The format is controlled by Status Register B. On some CMOS/RTC chips,
 the format bits in Status Reg B cannot be changed. So your code needs to
 be able to handle all four possibilities, and it should not try to modify 
Status Register B's settings. So you always need to read Status Register 
B first, to find out what format your date/time bytes will arrive in.

    * Status Register B, Bit 1 (value = 2): Enables 24 hour format if set
    * Status Register B, Bit 2 (value = 4): Enables Binary mode if set 

Binary mode is exactly what you would expect the value to be. 
If the time is 1:59:48 AM, then the value of hours would be 1,
 minutes would be 59 = 0x3b, and seconds would be 48 = 0x30.

In BCD mode, each of the two hex nibbles of the byte is modified to "display" a decimal number. So 1:59:48 has hours = 1, minutes = 0x59 = 89, seconds = 0x48 = 72. To convert BCD back into a "good" binary value, use: binary = ((bcd / 16) * 10) + (bcd & 0xf) [Optimised: binary = (bcd >> 1) + (bcd >> 3) + (bcd & 0xf)].

24 hour time is exactly what you would expect. Hour 0 is midnight to 1am, hour 23 is 11pm.

12 hour time is annoying to convert back to 24 hour time. If the hour is pm, 
then the 0x80 bit is set on the hour byte.
 So you need to mask that off. (This is true for both binary and BCD modes.) Then, midnight is 12, 1am is 1, etc. Note that carefully: midnight is not 0 -- it is 12 -- this needs to be handled as a special case in the calculation from 12 hour format to 24 hour format (by setting 12 back to 0)!

For the weekday format: Sunday = 1, Saturday = 7.
*/
#define RTCaddress 0x70
#define RTCdata         0x71

#define SECONDS                0x00
#define MINUTES                  0x02
#define HOURS                     0x04 
#define WEEKDAY                0x06       
#define DAY_OF_MONTH   0x07
#define MONTH                     0x08
#define YEAR                         0x09
#define CENTURY  0x32   
#define REG_A        0x0A
#define REG_B        0x0B 
/*
//
// uscita di un byte su una porta di IO
*/

/*Primitiva che riporta l'ora del BIOS. Il formato dell'ora sara il seguente : 
 * Primo       Byte secondi 0-60 formato binario 
 * Secondo byte minuti 0-60 formato binario 
 * terzo byte ore 0-24 formato binario 
 */ 

extern "C" natl  c_get_time () { 
  
  natl time = 0; 
  natb byte=0; 
  natb century=0; 
  bool bcd=false;   // varibaile che atttiva la conversione tra BCD e binary mode 
  bool pm =false;   // variabile che attiva la conversioen da 12ore a 24

  byte=10;  // registro a  

  //flog(LOG_WARN, "TIME"); 
  do {
    outputb(REG_A, RTCaddress); 
    inputb(RTCdata, byte); 
    //  flog(LOG_WARN, "%x", byte); 
  } while( (byte & 0x80) );          // verifico che non stia aggiornando i contatiori


  // prelevo il registro di stutus
  outputb(REG_B,RTCaddress); 
  inputb(RTCdata,byte); 
  
  // verifico se il formato dellìorologio e binario o bcd
    bcd= (byte & 0x04) ? false:true;

    //  flog(LOG_WARN, "TIME %s ", (byte & 0x04) ?" BYNARY" : "BCD" ); 
    
      //prelevo la zona 
    outputb(REG_B, CENTURY); 
    inputb(RTCdata,byte); 
    century=byte; 

    //   flog(LOG_WARN,"COUNTURY  %d", century); 
   
    //secondi 
    outputb(SECONDS,RTCaddress); 
    inputb(RTCdata,byte);   
    if (bcd)
      byte =((byte/ 16) * 10) + (byte & 0xf);
    time = (natl)(byte); 

    //minuti
    outputb(MINUTES,RTCaddress); 
    inputb(RTCdata,byte);   
    if (bcd)
      byte =((byte/ 16) * 10) + (byte & 0xf);

    time  = ((natl)byte <<8 ) | time  ; 

    //ORE
    outputb( HOURS,RTCaddress); 
    inputb(RTCdata,byte);   
    pm= (byte&0x80) ? true: false;
    //    flog(LOG_WARN," ore am_pm :%d", pm); 

    if (bcd)
byte =((byte/ 16) * 10) + (byte & 0xf);

    if (pm) 
      byte+=12;
 
     byte=(byte +century) %24; 

     time  = ((natl)byte << 16  ) | time  ; 


  return time; 
}

/*Funzione che prelava la data corrente dal BIOS. Riporta la data nel seguente modo :
 *  primaùo byte giorno 1-31 
 * secondo byte  mese  1-12 
 * terzo byte anno 1-99
 **********************************************************************************************/

extern "C" natl  c_get_date ()  {

  natl date = 0; 
  natb byte=0; 
  bool  bcd=false;  
 
  //flog(LOG_WARN, "DATE"); 
  do {
    outputb(REG_A, RTCaddress); 
    inputb(RTCdata, byte); 
    // flog(LOG_WARN, "%x", byte); 
  } while( (byte & 0x80) );          // verifico che non stia aggiornando i contatiori


 
  // prelevo il registro di stutus
  outputb(REG_B,RTCaddress); 
  inputb(RTCdata,byte); 
  
  // verifico se il formato dellìorologio e binario o bcd
    bcd= (byte & 0x04) ? false:true;

    //giorno
    outputb(DAY_OF_MONTH,RTCaddress); 
    inputb(RTCdata,byte);   
    if (bcd)
 byte =((byte/ 16) * 10) + (byte & 0xf);
   date = (natl)(byte); 
   
    //minuti
    outputb(MONTH,RTCaddress); 
    inputb(RTCdata,byte);   
    //  flog(LOG_WARN, "%x", byte); 
     if (bcd)
	 byte =((byte/ 16) * 10) + (byte & 0xf);
    date  = ((natl)byte <<8 ) | date  ; 

    //ORE
    outputb(YEAR,RTCaddress); 
    inputb(RTCdata,byte);  
       if (bcd)
 byte =((byte/ 16) * 10) + (byte & 0xf);
    date  = ((natl)byte << 16  ) | date  ; 

   
  return date; 

}

// funzione che mi genera un numero psedo casuale 
/*
extern "C" natl c_random () { 
  static long long unsigned x= ((c_get_time() << 8 ) | c_get_date()); 
  long long unsigned m =4294967296; 
  long long unsigned a =1103515245; 
  natl c =12345; 

  x = (a*x +c) % m; 

  return (natl)x; 
}
*/ 
// inizializza i gate usati per le chiamate di IO
//
extern "C" void fill_io_gates(void);


//Funzione ceh inizializza il file system 
// dishiaro esterna perchè la inserisco in un file esterno nella cartella fs 

extern "C"  int  fs_init( void); 

// eseguita in fase di inizializzazione
//
extern "C" void cmain(int sem_io)
{
	fill_io_gates();
	lib_init();
	
	if (!kbd_init()) {
		flog(LOG_WARN, "kdb_init fallita");
		abort_p();
	}
	if (!vkbd_init()) {
		flog(LOG_WARN, "vkdb_init fallita");
		abort_p();
	}
	if (!com_init()) {
		flog(LOG_WARN, "com_init fallita");
		abort_p();
	}

	if (!vmon_init()) {
		flog(LOG_WARN, "vmon_init fallita");
		abort_p();
	}
	if (!console_init(VKBD_CONSOLE, 0)) {
		flog(LOG_WARN, "console_init fallita");
		abort_p();
	}
	// driver PCI
	if (!pci_init()) {
		flog(LOG_WARN, "pci_init fallita");
		abort_p();
	}

	if(!fs_init()){ 
	        flog(LOG_WARN, "fs_init fallita");  
	}
	
	sem_signal(sem_io);
	terminate_p();
}




