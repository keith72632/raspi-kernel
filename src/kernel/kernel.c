#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <common/stdio.h>
#include <common/stdlib.h>

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

//https://wiki.osdev.org/Raspberry_Pi_Bare_Bones