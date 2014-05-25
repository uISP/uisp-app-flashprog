#include <arch/antares.h>
#include <avr/boot.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <generated/usbconfig.h>
#include <arch/vusb/usbportability.h>
#include <arch/vusb/usbdrv.h>


#define SPI_PORT PORTB
#define SCK PORTB5 /* port 13 */
#define MISO PORTB4 /* port 12 */
#define MOSI PORTB3 /* port 11 */
#define SS PORTB2 /* port 10 */
#define DDR_SPI DDRB


#define LED_ON  PORTC|=1<<2
#define LED_OFF PORTC&=~(1<<2)


inline void spi_cs(uchar s)
{
	if (s) 
		PORTB |= 1<<SS;
	else
		PORTB &= ~(1<<SS);
}


unsigned char spi_xfer(unsigned char c)
{
	unsigned char r;

	/* transmit c on the SPI bus */
	SPDR = c;
	/* Wait for the transmission to be comlpeted */
	loop_until_bit_is_set(SPSR,SPIF);
	r = SPDR;
	return r;
}


static unsigned rq_len;
usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;
	if (rq->bRequest == 0) { 
		rq_len = rq->wLength.word; 
		return USB_NO_MSG;
	} else if (rq->bRequest == 1) { 
		spi_cs(rq->wValue.bytes[0]);
	}
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
	uchar i;
	for(i=0; i<len; i++) { 
		spi_xfer(data[i]);
	}
	rq_len -= len; 
	if (!rq_len) 
		return 1;
	else
		return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
	uchar i;
	for(i=0; i<len; i++) { 
		data[i] = spi_xfer(0x0);
	}

	return len;
}


inline void usbReconnect()
{
	DDRD=0xff;
	_delay_ms(250);
	DDRD=0;
}

ANTARES_INIT_LOW(io_init)
{
	/* set SS low */
	SPI_PORT &= ~(1<<SS);
	/* Enable MOSI,SCK,SS as output like on
	http://en.wikipedia.org/wiki/File:SPI_single_slave.svg */
	DDR_SPI = (1<<MOSI)|(1<<SCK)|(1<<SS);
	/* Enable SPI Master, set the clock to F_CPU / 2 */
	/* CPOL and CPHA are 0 for SPI mode 0 (see wikipedia) */
	/* we use mode 0 like for the linux spi in flashrom*/
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR = (1<<SPI2X);

	DDRC=1<<2;
	PORTC=0xff;
 	usbReconnect();
}

ANTARES_INIT_HIGH(uinit)
{
  	usbInit();
}


ANTARES_APP(usb_app)
{
	usbPoll();
}
