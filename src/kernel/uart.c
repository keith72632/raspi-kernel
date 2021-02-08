#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <common/stdlib.h>

/*
This kernel is coded for a raspi 2. Change variable below 
and in header file to your raspi model
*/
//int raspi = 2;
//writes a 4 byte word to a register
void mmio_write(uint32_t reg, uint32_t data)
{
	*(volatile uint32_t*)reg = data;
}

//returns 4 byte word in register
uint32_t mmio_read(uint32_t reg)
{
	return *(volatile uint32_t*)reg;
}

// Loop <delay> times in a way that the compiler won't optimize away
void delay(int32_t count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
            : "=r"(count): [count]"0"(count) : "cc");
}



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

    //write 0 to GPPUDCLK0 to make it take effect
    mmio_write(GPPUDCLK0, 0x00000000);

    /*
    sets all flags in the Interrupt Clear Register 
    0x3F201044 = 00111111001000000001000001000100
    0x7FF =      00000000000000000000011111111111
    */
    mmio_write(UART0_ICR, 0x7FF);

    // Set integer & fractional part of baud rate.
    // Divider = UART_CLOCK/(16 * Baud)
    // Fraction part register = (Fractional part * 64) + 0.5
    // Baud = 115200.
 
    // For Raspi3 and 4 the UART_CLOCK is system-clock dependent by default.
    // Set it to 3Mhz so that we can consistently set the baud rate
    /*
    if (raspi >= 3) {
        // UART_CLOCK = 30000000;
        unsigned int r = (((unsigned int)(&mbox) & ~0xF) | 8);
        // wait until we can talk to the VC
        while ( mmio_read(MBOX_STATUS) & 0x80000000 ) { }
        // send our message to property channel and wait for the response
        mmio_write(MBOX_WRITE, r);
        while ( (mmio_read(MBOX_STATUS) & 0x40000000) || mmio_read(MBOX_READ) != r ) { }
    }
    */

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