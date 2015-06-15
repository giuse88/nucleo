# utente.s
#
#define ASM 1
#include <costanti.h>

# Tipi di interruzione per le chiamate di sistema e per le primitive di IO
#

.set tipo_a, TIPO_A
.set tipo_t, TIPO_T
.set tipo_si, TIPO_SI
.set tipo_w, TIPO_W
.set tipo_s, TIPO_S
.set tipo_d, TIPO_D
.set tipo_rl, TIPO_RL
.set tipo_re, TIPO_RE
.set tipo_l, TIPO_L
.set tipo_ep, TIPO_EP
.set tipo_dm, TIPO_DM
.set tipo_ud,  TIPO_UD
.set tipo_getid, TIPO_GETID
.set tipo_getidp, TIPO_GETIDP

.set io_tipo_pcir, IO_TIPO_PCIR
.set io_tipo_rsen, IO_TIPO_RSEN
.set io_tipo_rseln, IO_TIPO_RSELN
.set io_tipo_wsen, IO_TIPO_WSEN
.set io_tipo_wse0, IO_TIPO_WSE0
.set io_tipo_tr, IO_TIPO_TR
.set io_tipo_trl, IO_TIPO_TRL
.set io_tipo_tw, IO_TIPO_TW
.set io_tipo_rkbd, IO_TIPO_RKBD
.set io_tipo_ikbd, IO_TIPO_IKBD
.set io_tipo_wfikbd, IO_TIPO_WFIKBD
.set io_tipo_skbd, IO_TIPO_SKBD
.set io_tipo_smon, IO_TIPO_SMON
.set io_tipo_wmon, IO_TIPO_WMON
.set io_tipo_wcon, IO_TIPO_WCON
.set io_tipo_inic, IO_TIPO_INIC
.set io_tipo_cmon, IO_TIPO_CMON
.set io_tipo_gmon, IO_TIPO_GMON
.set io_tipo_lkbd, IO_TIPO_LKBD
.set io_tipo_kmon, IO_TIPO_KMON
.set io_tipo_pkbd, IO_TIPO_PKBD
.set io_tipo_rcon, IO_TIPO_RCON

#tipi per le primitive di trasmissione/ricezione eth
.set io_tipo_ethr, IO_TIPO_ETHR
.set io_tipo_etht, IO_TIPO_ETHT

#if 0
.set io_tipo_ak_set, 	IO_TIPO_AK_SET
.set io_tipo_ak_get, 	IO_TIPO_AK_GET
#endif
.set io_tipo_sndcommand, IO_TIPO_SNDCOMMAND
.set io_tipo_play, 	IO_TIPO_PLAY
#date e time 
.set io_tipo_get_time,  IO_TIPO_TIME 
.set io_tipo_get_date, IO_TIPO_DATE

	.text

	.global _activate_p
_activate_p:
	int $tipo_a
	ret

	.global _terminate_p
_terminate_p:
	int $tipo_t
	ret

	.global _end_program
_end_program:
	int $tipo_ep
	ret

	.global _sem_ini
_sem_ini:
	int $tipo_si
	ret

	.global _sem_wait
_sem_wait:
	int $tipo_w
	ret

	.global _sem_signal
_sem_signal:
	int $tipo_s
	ret

	.global _delay
_delay:
	int $tipo_d
	ret

	.global _udelay
_udelay:
	int $tipo_ud
	ret

	.global _dump
_dump:
	int $tipo_dm
	ret

	.global _pci_read
_pci_read:
	int $io_tipo_pcir
	ret

	.global _readse_n
_readse_n:
	int $io_tipo_rsen
	ret

	.global _readse_ln
_readse_ln:
	int $io_tipo_rseln
	ret

	.global _writese_n
_writese_n:
	int $io_tipo_wsen
	ret

	.global _writese_0
_writese_0:
	int $io_tipo_wse0
	ret

	.global _readconsole
_readconsole:
	int $io_tipo_rcon
	ret

	.global _writeconsole
_writeconsole:
	int $io_tipo_wcon
	ret

	.global _iniconsole
_iniconsole:
	int $io_tipo_inic
	ret

	.global _vkbd_read
_vkbd_read:
	int $io_tipo_rkbd
	ret

	.global _vkbd_intr_enable	
_vkbd_intr_enable:
	int $io_tipo_ikbd
	ret

	.global _vkbd_wfi
_vkbd_wfi:
	int $io_tipo_wfikbd
	ret

	.global _vkbd_send
_vkbd_send:
	int $io_tipo_pkbd
	ret

	.global _vkbd_switch
_vkbd_switch:
	int $io_tipo_skbd
	ret

	.global _vmon_switch
_vmon_switch:
	int $io_tipo_smon
	ret

	.global _vmon_write_n
_vmon_write_n:
	int $io_tipo_wmon
	ret

	.global _vmon_setcursor
_vmon_setcursor:
	int $io_tipo_cmon
	ret

	.global _vmon_getsize
_vmon_getsize:
	int $io_tipo_gmon
	ret

	.global _vmon_cursor_shape
_vmon_cursor_shape:
	int $io_tipo_kmon
	ret

	.global _vkbd_leds
_vkbd_leds:
	int $io_tipo_lkbd
	ret


	.global _eth_receive
_eth_receive:
	int $io_tipo_ethr
	ret
	
	.global _eth_transmit
_eth_transmit:
	int $io_tipo_etht
	ret

	.global _readlog
_readlog:
	int $tipo_rl
	ret

	.global _resident
_resident:
	int $tipo_re
	ret

	.global _log
_log:
	int $tipo_l
	ret

#if 0
	.global _setMixer
_setMixer:
	int $io_tipo_ak_set
	ret
	
	.global _getMixer
_getMixer:
	int $io_tipo_ak_get
	ret	
#endif	
	
	.global _sndcommand
_sndcommand:
	int $io_tipo_sndcommand
	ret	

	
	.global _play
_play:
	int $io_tipo_play
	ret		
	.global start, _start
start:
_start:
	call _lib_init
	jmp _main

	.global _get_id
_get_id:
	int $tipo_getid
	ret

	.global _get_id_p
_get_id_p:
	int $tipo_getidp
	ret
	.global _get_time 
_get_time : 
	int $io_tipo_get_time 
	ret
	.global _get_date
_get_date : 
	int $io_tipo_get_date
	ret  
