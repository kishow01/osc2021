#include "uart.h"
#include "shell.h"
#include "buddy.h"

void main() {
    uart_init();
    buddy_init();

    shell_welcome_message();

    while(1) {
        shell_start();
    }
}