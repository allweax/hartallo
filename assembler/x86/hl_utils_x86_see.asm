
bits 32

global	_hl_utils_rdtsc_x86_asm
global _hl_utils_thread_get_core_id_x86_asm

section .data

section .text


_hl_utils_rdtsc_x86_asm:	
	rdtsc
	ret
	
;;; int32_t hl_utils_thread_get_core_id_x86_asm();
_hl_utils_thread_get_core_id_x86_asm:
	mov eax, 1
    cpuid
    shr ebx, 24
    mov eax, ebx
    ret
