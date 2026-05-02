











[org 0x7c00]           
[bits 16]              

KERNEL_OFFSET equ 0x1000   




boot_start:
    
    xor ax, ax          
    mov ds, ax          
    mov es, ax          
    mov ss, ax          
    mov sp, 0x7C00      

    mov [BOOT_DRIVE], dl  

    
    call load_kernel

    
    call switch_to_protected_mode

    jmp $               




load_kernel:
    
    mov si, 3           

.retry:
    
    xor ax, ax          
    mov dl, [BOOT_DRIVE]
    int 0x13

    
    mov di, 35          
    mov ch, 0           
    mov cl, 2           
    mov dh, 0           
    mov bx, KERNEL_OFFSET 

.read_sector_loop:
    mov ah, 0x02        
    mov al, 1           
    mov dl, [BOOT_DRIVE]
    int 0x13            
    jc .read_failed     

    
    add bx, 512         
    inc cl              
    cmp cl, 64          
    jl .next_iter
    mov cl, 1           
    inc dh              
.next_iter:
    dec di
    jnz .read_sector_loop
    jmp .read_ok

.read_failed:
    
    dec si
    jnz .retry          
    jmp disk_error      

.read_ok:
    ret

disk_error:
    
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    jmp $








gdt_start:

gdt_null:              
    dq 0

gdt_code:              
    dw 0xFFFF          
    dw 0x0000          
    db 0x00            
    db 10011010b       
    db 11001111b       
    db 0x00            

gdt_data:              
    dw 0xFFFF          
    dw 0x0000          
    db 0x00            
    db 10010010b       
    db 11001111b       
    db 0x00            

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   
    dd gdt_start                  


CODE_SEG equ gdt_code - gdt_start   
DATA_SEG equ gdt_data - gdt_start   




switch_to_protected_mode:
    cli                 
    lgdt [gdt_descriptor] 

    
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    
    jmp CODE_SEG:protected_mode_start




[bits 32]

protected_mode_start:
    
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000    

    
    jmp KERNEL_OFFSET




BOOT_DRIVE: db 0       




times 510 - ($ - $$) db 0   
dw 0xAA55                    