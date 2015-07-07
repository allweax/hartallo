; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

bits 32

global _hl_bits_clz32_asm_x86 
global _hl_bits_clz16_asm_x86
global _hl_bits_ctz32_asm_x86 
global _hl_bits_ctz16_asm_x86
global _hl_bits_bswap32_asm_x86
global _hl_bits_bittest32_asm_x86

section .data
	
section .text

;;;
;;; hl_size_t hl_bits_clz32_cpp(int32_t n)
_hl_bits_clz32_asm_x86:    
    mov eax, 32
    mov edx, [esp + 4]
    cmp edx, 0
    jz .end
    bsr eax, edx
    xor eax, 31
    
    .end
	ret
	
;;;
;;; hl_size_t hl_bits_clz16_cpp(int16_t n)
_hl_bits_clz16_asm_x86:    
    mov eax, 16
    mov edx, [esp + 4]
    shl edx, 16
    cmp edx, 0
    je .end
    bsr eax, edx
    xor eax, 31
    
    .end
	ret

;;;	
;;; hl_size_t hl_bits_ctz32_cpp(int32_t n)
_hl_bits_ctz32_asm_x86:   
    mov eax, 32
    mov edx, [esp + 4]
    cmp edx, 0
    je .end
    bsf eax, edx
    
    .end
	ret
	
;;;
;;; hl_size_t hl_bits_ctz16_cpp(int16_t n)
_hl_bits_ctz16_asm_x86:   
    mov eax, 16
    mov edx, [esp + 4]
    cmp edx, 0
    je .end
    bsf eax, edx
    
    .end
	ret
	
	
;;;
;;; int32_t hl_bits_bswap32_cpp(int32_t n)
_hl_bits_bswap32_asm_x86:   
    mov eax, [esp + 4]    
    bswap eax
        
	ret
	
;;;
;;; int32_t hl_bits_bittest32_cpp(int32_t n, int32_t p)
_hl_bits_bittest32_asm_x86:    
    mov edx, [esp + 4]
    mov ecx, [esp + 8]
    xor eax, eax
    bt edx, ecx
    jnc .end
    mov eax, 1
    
    .end
	ret
	