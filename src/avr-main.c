#include <arch/antares.h>
#include <avr/boot.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <generated/usbconfig.h>
#include <arch/vusb/usbportability.h>
#include <arch/vusb/usbdrv.h>
#include <flashrom.h>




/* PROTOCOL: 
   All communication is done via control transfers. 
   wIndex ALWAYS specifies the SPI interface number to work with during this request. 
   wValue is request-specific e.g. CS value.   
 */

static unsigned rq_len; /* This request length */

/* In case we're on avr and want to use only ONE fisrt SPI 
   turning this on will make a small performance boost;
*/


static struct flashprog_spi_device *spi; /* Active spi device for data stage */

ANTARES_INIT_LOW(dspi)
{
	spi = &g_flashprog_platform.spidev[0]; 
	/* Default to 0'th spi */
}

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;

#ifndef CONFIG_SINGLE_SPI_AVR_OPT
	/* Select an SPI to communicate with */
	if (rq->wIndex.word > g_flashprog_platform.vinfo.num_spi)
		rq->wIndex.word = 0;
	spi = &g_flashprog_platform.spidev[rq->wIndex.word];
#endif

	if (rq->bRequest == RQ_VERINFO) {
		usbMsgPtr = (uchar*) &g_flashprog_platform.vinfo;
		return sizeof(struct flashprog_verinfo);
	} else if (rq->bRequest == RQ_SPI_IO) { 
		rq_len = rq->wLength.word; 
		return USB_NO_MSG;
	} else if (rq->bRequest == RQ_SPI_CS) {
		spi->cs(rq->wValue.bytes[0]);
	} else if (rq->bRequest == RQ_SPI_SET_SPEED) {
		spi->cur_spi_spd_khz = spi->set_speed(rq->wValue.word);
	} else if (rq->bRequest == RQ_SPI_GET_SPEED) {
		usbMsgPtr = &spi->cur_spi_spd_khz;
		return sizeof(spi->cur_spi_spd_khz);
	}
	return 0;
}

#ifndef CONFIG_SINGLE_SPI_AVR_OPT
uchar usbFunctionWrite(uchar *data, uchar len)
{
	spi->write_block(data, len);
	rq_len -= len; 
	if (!rq_len) 
		return 1;
	else
		return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
	spi->read_block(data, len);
	return len;
}

#else

uchar usbFunctionWrite(uchar *data, uchar len)
{
	rq_len -= len; 
	while (len--) { 
		SPDR = *data++;
		/* Wait for the transmission to be completed */
		loop_until_bit_is_set(SPSR, SPIF);
	}

	if (!rq_len) 
		return 1;
	else
		return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
	char tmp = len;
	while (tmp--) { 
		SPDR = 0x0;
		/* Wait for the transmission to be completed */
		loop_until_bit_is_set(SPSR,SPIF);
		*data++ = SPDR;
	}
	return len;
}

#endif


ANTARES_INIT_HIGH(usb_init)
{
  	usbInit();
}


ANTARES_APP(usb_app)
{
	usbPoll();
}
