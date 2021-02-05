#include <stddef.h>
#include <stdint.h>

//writes a 4 byte word to a register
static inline void mmio_write(uint32_t reg, uint32_t data)
{
	*(volatile uint32_t*)reg = data;
}

//returns 4 byte word in register
static inline uint32_t mmio_read(uint32_t reg)
{
	return *(volatile uint32_t*)reg;
}

// Loop <delay> times in a way that the compiler won't optimize away
static inline void delay(int32_t count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
            : "=r"(count): [count]"0"(count) : "cc");
}

enum
{
    // The GPIO registers base address.
    GPIO_BASE = 0x3F200000, // for raspi2 & 3, 0x20200000 for raspi1

    GPPUD = (GPIO_BASE + 0x94),
    GPPUDCLK0 = (GPIO_BASE + 0x98),

    // The base address for UART.
    UART0_BASE = 0x3F201000, // for raspi2 & 3, 0x20201000 for raspi1

    UART0_DR     = (UART0_BASE + 0x00),//0x03F201000
    UART0_RSRECR = (UART0_BASE + 0x04),//0x03F201004
    UART0_FR     = (UART0_BASE + 0x18),//0x03F201018
    UART0_ILPR   = (UART0_BASE + 0x20),//0x03F201020
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C),
};

void uart_init()
{
	//disables all aspects of UART hardware. this is UART's control register
    mmio_write(UART0_CR, 0x00000000);

    /*writing zero to GPPUD marks that some pins should be diabled*/
    mmio_write(GPPUD, 0x00000000);
    delay(150);

    /*
    marks the specific pins that are being disabled
	GPPUDCLK0 = 0x3F200098 00111111001000000000000010011000
	leftshift 1 14 place = 00000000000000000100000000000000
	leftshift 1 15 place = 00000000000000001000000000000000
	new value ==           00000000000000001100000000000000
    */
    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);

    mmio_write(GPPUDCLK0, 0x00000000);

    /*
    sets all flags in the Interrupt Clear Register 
    0x3F201044 = 00111111001000000001000001000100
    0x7FF =      00000000000000000000011111111111
    */
    mmio_write(UART0_ICR, 0x7FF);

    /*
    sets the baud rate connection. bits/sec that can go through a 
    serial port. This is 1.67, so put 1 in ibrd. then multiply
    .67 * 64 and then add .5. this is 40 and put into register fbrd*/
    mmio_write(UART0_IBRD, 1);
    mmio_write(UART0_FBRD, 40);

    /*
    this sets bit 4 5 and 6 of Line Control Register. setting bit
    4 means UART hardware will hold data in an 8 item deep FIFO, instead 
    of 1 item deep register. setting 5 and 6 means data wsent or recived 
    will have 8 bit-long words*/
    mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    /*
    this disbales all interrupts from the UART by wrting oen to the
    relevant bits of the Interrupt Mask Set Clear*/
    mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
            (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

    /*
    writes bits 0 8 and 9 to the Control Register. bit 0 enables
    UART hardware, bit 8 enables the ability to recived data, and bit
    9 enavles the ability to transmit data*/
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

//Reading ans writing text

/*FR == Flags Register %5th bit tells wether the read FIFO has 
any data to read, and 4th is wether the write FIFO can accept any 
data. DR == Data register*/
void uart_putc(unsigned char c)
{
	while( mmio_read(UART0_FR) & (1 << 5)){/*running*/}
	mmio_write(UART0_DR, c);
}

unsigned char uart_getc()
{
	while( mmio_read(UART0_FR) & (1 << 4)){/*running*/}
	return mmio_read(UART0_DR);
}

void uart_puts(const char* str)
{
	for(size_t i = 0; str[i] != '\0'; i++)
		uart_putc((unsigned char)str[i]);

}

/*
main function for kernel(duh). this is where control is transfere
to from boot.S. instead of standard C main function which is formatted
like int main(int argvc, char *argv[]);, it uses ARM Formatting. in ARM,
the convention is that the first three parameters passed to main are 
passed through r0, r1, and r2. When the bootloader loads our kernel, it also
places some information about hardware and command line used to run the
kernel in memory, called atags, and a pointer to atags is placed in r2 before
boot.S runs. While previous mentioning the paramters required in kernel main 
consisted of r2, which has a pointer to atags, a tags is passed*/
void kernel_main(uint32_t r0, uint32_t r1, unsigned atags)
{
	//declare as unsused
	(void) r0;
	(void) r1;
	(void) atags;

	uart_init();
	uart_puts("Hello, kernel bitches\r\n");

	while(1){
		uart_putc(uart_getc());
		uart_putc('\n');
	}
}