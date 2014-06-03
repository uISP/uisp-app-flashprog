#include <arch/antares.h>
#include <avr/boot.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <generated/usbconfig.h>
#include <arch/vusb/usbportability.h>
#include <arch/vusb/usbdrv.h>
#include <flashrom.h>


#define SPI_PORT PORTB
#define SCK PORTB5 /* port 13 */
#define MISO PORTB4 /* port 12 */
#define MOSI PORTB3 /* port 11 */
#define SS PORTB2 /* port 10 */
#define DDR_SPI DDRB


static void spi_cs(unsigned char s)
{
	if (s) 
		PORTB |= 1<<SS;
	else
		PORTB &= ~(1<<SS);
}

static void spi_write_block(unsigned char* data, uint16_t len)
{
	/* transmit c on the SPI bus */
	while (len--) { 
		SPDR = *data++;
		/* Wait for the transmission to be completed */
		loop_until_bit_is_set(SPSR,SPIF);
	}
}

static void spi_read_block(unsigned char* data, uint16_t len)
{
	/* transmit c on the SPI bus */
	while (len--) { 
		SPDR = 0x0;
		/* Wait for the transmission to be completed */
		loop_until_bit_is_set(SPSR,SPIF);
		*data++ = SPDR;
	}
}


void spi_set_speed(uint16_t spd_khz)
{
	/* set SS low */
	SPI_PORT &= ~(1<<SS);
	/* Enable MOSI,SCK,SS as output like on
	http://en.wikipedia.org/wiki/File:SPI_single_slave.svg */
	DDR_SPI = (1<<MOSI)|(1<<SCK)|(1<<SS);
	/* Enable SPI Master, set the clock to F_CPU / 2 */
	/* CPOL and CPHA are 0 for SPI mode 0 (see wikipedia) */
	/* we use mode 0 like for the linux spi in flashprog*/
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR = (1<<SPI2X);
	g_flashprog_platform.vinfo.spi_freq = 8000;
}



ANTARES_INIT_LOW(io_init)
{
	/* Initilize the LED */
	DDRC=1<<2;
	PORTC=0xff;

	/* Reconnect the usb */
	DDRD=0xff;
	_delay_ms(250);
	DDRD=0;
	spi_set_speed(-1); /* Set max SPI speed */
}



struct flashprog_spi_device sdevs[] = {
	{ 
		.cs          = spi_cs,
		.set_speed   = spi_set_speed,
		.write_block = spi_write_block,
		.read_block  = spi_read_block,
	},		
};



struct flashprog_platform g_flashprog_platform = {
	.vinfo = { 
		.pgmname  = "uISP-flashprog",
		.max_rq   = RQ_INVALID,
		.api_ver  = FLASHPROG_API_VER,
		.spi_freq = 8000,
		.cpu_freq = (F_CPU / 1000),
		.num_spi  = ARRAY_SIZE(sdevs), 
	},
	.spidev = sdevs,
};

