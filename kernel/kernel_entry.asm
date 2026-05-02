








[bits 32]              
[extern kernel_main]   

global _start

extern __bss_start
extern __bss_end

_start:
    
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    shr ecx, 2          
    xor eax, eax        
    rep stosd           

    call kernel_main   
    jmp $              
