// sys.h
// header per l' interfaccia offerta dal nucleo e dal modulo di IO
//

#ifndef SYS_H_
#define SYS_H_

#include <tipo.h>
#include <costanti.h>

extern "C" natl activate_p(void f(int), int a, natl prio, natl liv);
extern "C" void terminate_p();
extern "C" natl give_num();
extern "C" void end_program();
extern "C" natl sem_ini(int val);
extern "C" void sem_wait(natl sem);
extern "C" void sem_signal(natl sem);
extern "C" void delay(natl n);
extern "C" void readse_n(natl serial, natb vetti[], natl quanti, natb &errore);
extern "C" void readse_ln(natl serial, natb vetti[], natl &quanti, natb &errore);
extern "C" void writese_0(natl serial, natb vetto[], natl &quanti);
extern "C" void writese_n(natl serial, natb vetto[], natl quanti);
extern "C" bool resident(void* start,  natl quanti, bool residente = true);
extern "C" void dump(natl tipo);
extern "C" natl get_id();
extern "C" natl get_id_p();

extern "C" natl pci_read(natw l, natw regn, natl size);

const natl VKBD_LED_SCROLLOCK = 1L;
const natl VKBD_LED_NUMLOCK   = 2L;
const natl VKBD_LED_CAPSLOCK  = 4L;

extern "C" void vkbd_wfi(int v);
extern "C" natw vkbd_read(int v);
extern "C" void vkbd_intr_enable(int v, bool enable);
extern "C" void vkbd_switch(int v);
extern "C" void vkbd_send(int v, natw code, bool clear = false);
extern "C" void vkbd_leds(int v, natb led, bool on);
extern "C" void vmon_switch(int v);
extern "C" void vmon_write_n(int v, natl off, natw vetti[], int quanti);
extern "C" void vmon_setcursor(int v, natl off);
extern "C" bool vmon_getsize(int v, natl& maxx, natl& maxy);
extern "C" bool vmon_cursor_shape(int v, int shape);

extern "C" void readlog(log_msg& m);
extern "C" void log(log_sev sev, cstr msg, natl quanti);

extern "C" void iniconsole(natb cc);
extern "C" void readconsole(str buff, natl& quanti);
extern "C" void writeconsole(cstr buff);

extern "C" void eth_receive(natl eth_if, natb uvetti[], natl uquanti, ETH_ERR& rx_err);
extern "C" void eth_transmit(natl eth_if, natb uvetto[], natl uquanti, ETH_ERR& tx_err);

// funzioni per la scheda audio
#if 0
extern "C" void getMixer(natb &err, natb &value, natb port);
extern "C" void setMixer(natb &err, natb port, natb value);
#endif

extern "C" void play(addr puntatore, natl size, natl rate, natl mode, int channel);
extern "C" void sndcommand(int channel, int cmd);

extern "C" unsigned int get_time(); 
extern "C" unsigned int get_date(); 
#endif

