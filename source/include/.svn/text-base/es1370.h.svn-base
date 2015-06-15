// Specifiche della scheda audio ES1370
// http://dl.dropbox.com/u/3417379/ES1370.PDF

// Specifiche del CODEC:
// http://dl.dropbox.com/u/3417379/ak4531.pdf


// Vendor_ID e Device_ID della scheda audio
// Ensoniq 1370

#define ES_VENDOR_ID 0x1274
#define ES_DEVICE_ID 0x5000

// Clock divide ratio
#define ES_1370_SRCLOCK	   1411200

// indirizzi interni della scheda audio ES1370
	// per alcuni registri vengono specificati
	// alcuni shift di bit di cui vogliamo
	// controllare il settaggio

#define ES_CONTROL		0x00
//	#define   ES_1370_ADC_STOP	(1<<31)		/* disable capture buffer transfers */
//	#define   ES_1370_XCTL1 	(1<<30)		/* general purpose output bit */
//	#define   ES_1370_PCLKDIVO(o)	(((o)&0x1fff)<<16)/* clock divide ratio for DAC2 */
	#define   ES_1370_PCLKDIVM	((0x1fff)<<16)	/* mask for above */
//	#define   ES_1370_PCLKDIVI(i)	(((i)>>16)&0x1fff)/* clock divide ratio for DAC2 */
//	#define   ES_MSFMTSEL		(1<<15)		/* MPEG serial data format; 0 = SONY, 1 = I2S */
//	#define   ES_1370_M_SBB		(1<<14)		/* clock source for DAC - 0 = clock generator; 1 = MPEG clocks */
//	#define   ES_1370_WTSRSEL(o)	(((o)&0x03)<<12)/* fixed frequency clock for DAC1 */
//	#define   ES_1370_WTSRSELM	(0x03<<12)	/* mask for above */
//	#define   ES_1370_DAC_SYNC	(1<<11)		/* DAC's are synchronous */
//	#define   ES_CCB_INTRM		(1<<10)		/* CCB voice interrupts enable */
//	#define   ES_1370_M_CB		(1<<9)		/* capture clock source; 0 = ADC; 1 = MPEG */
//	#define   ES_1370_XCTL0		(1<<8)		/* generap purpose output bit */
//	#define   ES_BREQ		(1<<7)		/* memory bus request enable */
	#define   ES1370_DAC1_EN	(1<<6)		/* abilitazione canale di esecuzione DAC1 */
	#define   ES1370_DAC2_EN	(1<<5)		/* DAC2 playback channel enable */
	#define   ES1370_ADC_EN		(1<<4)		/* ADC capture channel enable */
//	#define   ES_UART_EN		(1<<3)		/* UART enable */
//	#define   ES_1370_CDC_EN	(1<<1)		/* Codec interface enable */
//	#define   ES_1370_SERR_DISABLE	(1<<0)		/* PCI serr signal disable */

#define ES_STATUS 		0x04 
	#define   ES1370_INTR           (1<<31)		/* Interruzione pendente */
	#define   ES_1370_CSTAT		(1<<10)		/* CODEC occupato o scrittura in corso */
//	#define   ES_1370_CBUSY         (1<<9)		/* CODEC is busy */
//	#define   ES_1370_CWRIP		(1<<8)		/* CODEC register write in progress */
//	#define   ES_1370_VC(i)		(((i)>>5)&0x03)	/* voice code from CCB module */
//	#define   ES_MCCB		(1<<4)		/* CCB interrupt pending */
//	#define   ES1370_UART		(1<<3)		/* UART interrupt pending */
	#define   ES1370_DAC1		(1<<2)		/* DAC1 channel interrupt pending */
	#define   ES1370_DAC2		(1<<1)		/* DAC2 channel interrupt pending */
	#define   ES1370_ADC		(1<<0)		/* ADC channel interrupt pending */

#define ES_UART_DATA  		0x08 
#define ES_UART_STATUS 		0x09 
#define ES_UART_RES		0x0a		
#define ES_MEM_PAGE		0x0c
	
#define ES_CODEC_1370  		0x10	

#define ES_CHANNEL_STATUS 	0x1c 
#define ES_SERIAL	 	0x20	
//	#define   ES_P2_END_INCO(o)	(((o)&0x07)<<19)/* binary offset value to increment / loop end */
	#define   ES1370_P2_END_INCM	(0x07<<19)	/* mask for above */
//	#define   ES_P2_END_INCI(i)	(((i)>>16)&0x07)/* binary offset value to increment / loop end */
//	#define   ES_P2_ST_INCO(o)	(((o)&0x07)<<16)/* binary offset value to increment / start */
	#define   ES1370_P2_ST_INCM		(0x07<<16)	/* mask for above */
//	#define   ES_P2_ST_INCI(i)	(((i)<<16)&0x07)/* binary offset value to increment / start */
//	#define   ES_R1_LOOP_SEL	(1<<15)		/* ADC; 0 - loop mode; 1 = stop mode */
	#define   ES1370_P2_LOOP_SEL	(1<<14)		/* DAC2; 0 - loop mode; 1 = stop mode */
	#define   ES1370_P1_LOOP_SEL	(1<<13)		/* DAC1; 0 - loop mode; 1 = stop mode */
	#define   ES1370_P2_PAUSE	(1<<12)		/* DAC2; 0 - play mode; 1 = pause mode */
	#define   ES1370_P1_PAUSE	(1<<11)		/* DAC1; 0 - play mode; 1 = pause mode */
	#define   ES1370_R1_INT_EN	(1<<10)		/* ADC interrupt enable */
	#define   ES1370_P2_INT_EN	(1<<9)		/* DAC2 interrupt enable */
	#define   ES1370_P1_INT_EN	(1<<8)		/* DAC1 interrupt enable */
	#define   ES1370_P1_SCT_RLD	(1<<7)		/* forza il caricamento del contatore dei campioni per DAC1 */
	#define   ES1370_P2_DAC_SEN		(1<<6)		/* quadno siamo in stop mode: 0 - DAC2 esegue zero; 1 = DAC2 esegue l'ultimo campione */
//	#define   ES_R1_MODEO(o)	(((o)&0x03)<<4)	/* ADC mode; 0 = 8-bit mono; 1 = 8-bit stereo; 2 = 16-bit mono; 3 = 16-bit stereo */
//	#define   ES_R1_MODEM		(0x03<<4)	/* mask for above */
//	#define   ES_R1_MODEI(i)	(((i)>>4)&0x03)
//	#define   ES_P2_MODEO(o)	(((o)&0x03)<<2)	/* DAC2 mode; -- '' -- */
	#define   ES1370_P2_MODEM		(0x03<<2)	/* mask for above */
//	#define   ES_P2_MODEI(i)	(((i)>>2)&0x03)
//	#define   ES_P1_MODEO(o)	(((o)&0x03)<<0)	/* DAC1 mode; -- '' -- */
//	#define   ES_P1_MODEM		(0x03<<0)	/* mask for above */
//	#define   ES_P1_MODEI(i)	(((i)>>0)&0x03)

/*


*/


#define ES_DAC1_COUNT 		0x24
#define ES_DAC2_COUNT 		0x28 
#define ES_ADC_COUNT		0x2c 
#define ES_DAC1_FRAME 		0x30
#define ES_DAC1_SIZE		0x34
#define ES_DAC2_FRAME 		0x38
#define ES_DAC2_SIZE		0x3c



// indirizzi interni del codec AK4531
// possono essere anche utilizzati come indici
// all'interno dell'array natb chip_reg[0x1A];

#define AK4531_LMASTER  0x00	/* master volume left */
#define AK4531_RMASTER  0x01	/* master volume right */
#define AK4531_LVOICE   0x02	/* channel volume left */
#define AK4531_RVOICE   0x03	/* channel volume right */
#define AK4531_LFM      0x04	/* FM volume left */
#define AK4531_RFM      0x05	/* FM volume right */
#define AK4531_LCD      0x06	/* CD volume left */
#define AK4531_RCD      0x07	/* CD volume right */
#define AK4531_LLINE    0x08	/* LINE volume left */
#define AK4531_RLINE    0x09	/* LINE volume right */
#define AK4531_LAUXA    0x0a	/* AUXA volume left */
#define AK4531_RAUXA    0x0b	/* AUXA volume right */
#define AK4531_MONO1    0x0c	/* MONO1 volume left */
#define AK4531_MONO2    0x0d	/* MONO1 volume right */
#define AK4531_MIC      0x0e	/* MIC volume */
#define AK4531_MONO_OUT 0x0f	/* Mono-out volume */
#define AK4531_OUT_SW1  0x10	/* Output mixer switch 1 */
#define AK4531_OUT_SW2  0x11	/* Output mixer switch 2 */
#define AK4531_LIN_SW1  0x12	/* Input left mixer switch 1 */
#define AK4531_RIN_SW1  0x13	/* Input right mixer switch 1 */
#define AK4531_LIN_SW2  0x14	/* Input left mixer switch 2 */
#define AK4531_RIN_SW2  0x15	/* Input right mixer switch 2 */
#define AK4531_RESET    0x16	/* Reset & power down */
#define AK4531_CLOCK    0x17	/* Clock select */
#define AK4531_AD_IN    0x18	/* AD input select */
#define AK4531_MIC_GAIN 0x19	/* MIC amplified gain */



// alcune "funzioni" di utilità

#define   ES_CODEC_WRITE(a,d) ((((a)&0xff)<<8)|(((d)&0xff)<<0))
	// per poter scrivere nel codec, bisogna scrivere nel registro
	// ES_CODEC_1370; i bit 15:8 corrispondono all'indirizzo interno
	// del CODEC, i bit 7:0 al dato che vogliamo inserire


// comandi per la IOCTL
#define ES1370_STOP 0
#define ES1370_RESUME 1
#define ES1370_PAUSE 2


