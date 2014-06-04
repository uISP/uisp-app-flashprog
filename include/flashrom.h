#ifndef FLASHPROG_H
#define FLASHPROG_H


/*  Note about versioning convention and extending: 
 *  Keep RQ_INVALID as the last element in the enum. 
 *  Any new elements must got BEFORE IT. 
 *  Adding new requests doesn't break API therefore API_VER is 
 *  NOT incremented. 
 *  e.g. only change API_VER if you reorder or change the behaviour 
 *  of any of the requests in a non-compatible 
 */


#define FLASHPROG_API_VER  1

enum { 
	RQ_VERINFO,
	RQ_SPI_SET_SPEED,
	RQ_SPI_CS,
	RQ_SPI_IO,
	/* Insert new requests here */ 
	RQ_INVALID
};


struct flashprog_verinfo {
	char pgmname[16]; /* Programmer name */
	uint16_t max_rq;  /* Maximum RQ number */
	uint16_t api_ver; /* API version */
	uint16_t spi_freq; /* Current SPI frequency */
	uint16_t cpu_freq; /* CPU frequency */
	uint16_t num_spi; /* Number of SPI devices here */
} __attribute__((packed));

struct flashprog_spi_device {
	void (*cs)          (unsigned char state);
	void (*set_speed)   (uint16_t speed_khz);
	void (*write_block) (unsigned char* data, uint16_t len);
	void (*read_block)  (unsigned char* data, uint16_t len);
};

struct flashprog_platform {
	struct flashprog_verinfo     vinfo;
	struct flashprog_spi_device *spidev;
};


extern struct flashprog_platform g_flashprog_platform;

#endif
