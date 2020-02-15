bits 32
section     .text
    align     4
    dd        0x1BADB002            ; Unique value for the bootloader to start (The Magic Number).
    dd        0x00
    dd        - (0x1BADB002+0x00)

global start
extern kmain                        ; This is a function that is going to located the C code.
start:
    cli                             ; Clears the interrupts
    call kmain                      ; Send processor to continue execution from the kmain function is C code.
    hlt                             ; Halt the cpu(pause it from executing from this address)
