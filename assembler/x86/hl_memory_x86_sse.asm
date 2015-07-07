; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

%include "hl_globals_x86_sse.asm"

bits 32

global _hl_memory_copy4x4_asm_sse2
global _hl_memory_copy4x4_unaligned_asm_sse2
global _hl_memory_copy4x4_u32_to_u8_asm_sse41
global _hl_memory_copy4x4_u32_to_u8_stride4x4_asm_sse41
global _hl_memory_copy16x16_u8_to_u32_stride16x16_asm_sse2
global _hl_memory_copy16x16_u8_to_u32_stride16x4_asm_sse2
global _hl_memory_copy16x16_u8_stride16x4_asm_sse2
global _hl_memory_copy4x4_u8_to_u32_stride16x16_asm_sse2
global _hl_memory_copy4x4_u8_to_u32_stride16x4_asm_sse2
global _hl_memory_copy4x4_u8_stride16x4_asm_sse2
global _hl_memory_setzero4x4_asm_sse2
global _hl_memory_setzero16x16_asm_sse2

section .data
	
section .text

;;; void hl_memory_copy4x4_asm_sse2(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride)
_hl_memory_copy4x4_asm_sse2:
	mov eax, [esp + 12] ; p_src
	mov edx, [esp + 16]	; src_stride
	movdqa xmm0, [eax]
	movdqa xmm1, [eax + edx*4]
	movdqa xmm2, [eax + edx*8]
	lea eax, [eax + edx*8]
	movdqa xmm3, [eax + edx*4]
	
	mov eax, [esp + 4] ; p_dst
	mov edx, [esp + 8]	; dst_stride
	movdqa [eax], xmm0
	movdqa [eax + edx*4], xmm1
	movdqa [eax + edx*8], xmm2
	lea eax, [eax + edx*8]
	movdqa [eax + edx*4], xmm3
	
	ret
	
;;; void hl_memory_copy4x4_unaligned_asm_sse2(int32_t *p_dst, hl_size_t dst_stride, const int32_t *pc_src, hl_size_t src_stride)
_hl_memory_copy4x4_unaligned_asm_sse2:
	mov eax, [esp + 12] ; p_src
	mov edx, [esp + 16]	; src_stride
	movdqu xmm0, [eax]
	movdqu xmm1, [eax + edx*4]
	movdqu xmm2, [eax + edx*8]
	lea eax, [eax + edx*8]
	movdqu xmm3, [eax + edx*4]
	
	mov eax, [esp + 4] ; p_dst
	mov edx, [esp + 8]	; dst_stride
	movdqu [eax], xmm0
	movdqu [eax + edx*4], xmm1
	movdqu [eax + edx*8], xmm2
	lea eax, [eax + edx*8]
	movdqu [eax + edx*4], xmm3
	
	ret
	
;;;; void hl_memory_copy4x4_u32_to_u8_asm_sse41(
;;;;	uint8_t *p_dst, // esp + 4
;;;;	hl_size_t dst_stride, // esp + 8
;;;;	HL_ALIGNED(16) const uint32_t *pc_src, // esp + 12
;;;;	hl_size_t src_stride // esp + 16
;;;;	)
_hl_memory_copy4x4_u32_to_u8_asm_sse41:	
	mov eax, [esp + 12] ; pc_src
	mov edx, [esp + 16] ; src_stride
	movdqa xmm0, [eax]
	movdqa xmm1, [eax + edx*4]
	movdqa xmm2, [eax + edx*8]
	lea eax, [eax + edx*8]
	movdqa xmm3, [eax + edx*4]
	
	packusdw xmm0, xmm1
	packusdw xmm2, xmm3
	packuswb xmm0, xmm2
	
	mov eax, [esp + 4] ; p_dst
	mov edx, [esp + 8] ; dst_stride
	pextrd [eax], xmm0, 0
	pextrd [eax + edx], xmm0, 1
	pextrd [eax + edx*2], xmm0, 2
	lea eax, [eax + edx*2]
	pextrd [eax + edx], xmm0, 3
	
	ret
	
;;;; void hl_memory_copy4x4_u32_to_u8_stride4x4_asm_sse41(
;;;;	uint8_t *p_dst,
;;;;	HL_ALIGNED(16) const uint32_t *pc_src
;;;;	)
_hl_memory_copy4x4_u32_to_u8_stride4x4_asm_sse41:		
	mov eax, [esp + 8]
	movdqa xmm0, [eax]
	movdqa xmm1, [eax + 16]
	movdqa xmm2, [eax + 32]
	movdqa xmm3, [eax + 48]
	
	packusdw xmm0, xmm1
	packusdw xmm2, xmm3
	packuswb xmm0, xmm2
	
	mov eax, [esp + 4]
	movdqu [eax], xmm0
	
	ret
	
	
;;; void hl_memory_copy16x16_u8_to_u32_stride16x16_asm_sse2(
;;;;	HL_ALIGNED(16) uint32_t *p_dst, 
;;;;	HL_ALIGNED(16) const uint8_t *pc_src
;;;;	)
_hl_memory_copy16x16_u8_to_u32_stride16x16_asm_sse2:
	mov eax, [esp + 4] ; p_dst
	mov ecx, [esp + 8] ; pc_src
    pxor xmm0, xmm0
	
	%rep    16
		movdqa xmm2, [ecx]
		movdqa xmm3, xmm2
		punpcklbw xmm2, xmm0
		punpckhbw xmm3, xmm0
		movdqa xmm4, xmm2
		punpcklwd xmm4, xmm0
		punpckhwd xmm2, xmm0
		movdqa [eax], xmm4
		movdqa [eax + 16], xmm2
		movdqa xmm4, xmm3
		punpcklwd xmm4, xmm0
		punpckhwd xmm3, xmm0
		movdqa [eax + 32], xmm4
		movdqa [eax + 48], xmm3
		
		add eax, 64
		add ecx, 16
	%endrep
	
	ret
	
;;; void hl_memory_copy16x16_u8_to_u32_stride16x4_asm_sse2(
;;;;	HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, 
;;;;	HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/
;;;;	)
_hl_memory_copy16x16_u8_to_u32_stride16x4_asm_sse2:
	mov eax, [esp + 4] ; p_dst
	mov ecx, [esp + 8] ; pc_src
    pxor xmm0, xmm0
    xor edx, edx
    
    %assign i 0
   	%rep    16
		movd xmm1, [ecx]
		punpcklbw xmm1, xmm0
		punpcklwd xmm1, xmm0
		movdqa [eax], xmm1
		movd xmm1, [ecx + 4]
		punpcklbw xmm1, xmm0
		punpcklwd xmm1, xmm0
		movdqa [eax + 64], xmm1
		movd xmm1, [ecx + 8]
		punpcklbw xmm1, xmm0
		punpcklwd xmm1, xmm0
		movdqa [eax + 128], xmm1
		movd xmm1, [ecx + 12]
		punpcklbw xmm1, xmm0
		punpcklwd xmm1, xmm0
		movdqa [eax + 192], xmm1
		
		%assign i i+1
		%if i == 4 || i == 8 || i == 12
			add eax, 192
		%endif
   		
		add eax, 16
		add ecx, 16
    %endrep
    
	ret
	
	
;;; void hl_memory_copy4x4_u8_to_u32_stride16x16_asm_sse2(
;;;;	HL_ALIGNED(16) uint32_t *p_dst, 
;;;;	const uint8_t *pc_src
;;;;	)
_hl_memory_copy4x4_u8_to_u32_stride16x16_asm_sse2:
	mov eax, [esp + 4] ; p_dst
	mov ecx, [esp + 8] ; pc_src
    pxor xmm0, xmm0
    
    movd xmm1, [ecx]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax], xmm1
    movd xmm1, [ecx + 16]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax + 64], xmm1
    movd xmm1, [ecx + 32]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax + 128], xmm1
    movd xmm1, [ecx + 48]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax + 192], xmm1

	ret
	
;;; void hl_memory_copy16x16_u8_stride16x4_asm_sse2(
;;;;	uint8_t *p_dst/*stride=16*/, 
;;;;	HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/
;;;;	)
_hl_memory_copy16x16_u8_stride16x4_asm_sse2:
	mov eax, [esp + 4] ; p_dst
	mov ecx, [esp + 8] ; pc_src
    pxor xmm0, xmm0
    xor edx, edx
    
    %assign i 0
   	%rep    16
		movdqa xmm0, [ecx]
		movd [eax], xmm0
		psrldq xmm0, 4
		movd [eax + 16], xmm0
		psrldq xmm0, 4
		movd [eax + 32], xmm0
		psrldq xmm0, 4
		movd [eax + 48], xmm0
		
		%assign i i+1
		%if i == 4 || i == 8 || i == 12
			add eax, 48
		%endif
   		
		add eax, 4
		add ecx, 16
    %endrep
    
	ret
	
;;; static void hl_memory_copy4x4_u8_to_u32_stride16x4_asm_sse2(
;;;;	HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, 
;;;;	const uint8_t *pc_src/*stride=4*/
;;;;	)
_hl_memory_copy4x4_u8_to_u32_stride16x4_asm_sse2:
	mov eax, [esp + 4] ; p_dst
	mov ecx, [esp + 8] ; pc_src
    pxor xmm0, xmm0
    
    movd xmm1, [ecx]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax], xmm1
    movd xmm1, [ecx + 4]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax + 64], xmm1
    movd xmm1, [ecx + 8]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax + 128], xmm1
    movd xmm1, [ecx + 12]
    punpcklbw xmm1, xmm0
    punpcklwd xmm1, xmm0
    movdqa [eax + 192], xmm1

	ret
	
	
;;; void hl_memory_copy4x4_u8_stride16x4_asm_sse2(
;;;;	uint8_t *p_dst/*stride=16*/, 
;;;;	HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/
;;;;	)
_hl_memory_copy4x4_u8_stride16x4_asm_sse2:
	mov eax, [esp + 4] ; p_dst
	mov ecx, [esp + 8] ; pc_src
	
	movdqa xmm0, [ecx]
	movd [eax], xmm0
	psrldq xmm0, 4
	movd [eax + 16], xmm0
	psrldq xmm0, 4
	movd [eax + 32], xmm0
	psrldq xmm0, 4
	movd [eax + 48], xmm0
	
	ret
	
;;;
;;; void hl_memory_setzero4x4_asm_sse2(int32_t* p_mem)
_hl_memory_setzero4x4_asm_sse2:    
    mov eax, [esp + 4]
    ;prefetcht0 [eax]
	pxor xmm0, xmm0
	movdqa [eax], xmm0
	movdqa [eax + 16], xmm0
	movdqa [eax + 32], xmm0
	movdqa [eax + 48], xmm0
	ret
	
;;;
;;; void hl_memory_setzero16x16_asm_sse2(int32_t* p_mem)
_hl_memory_setzero16x16_asm_sse2:
    mov eax, [esp + 4]
    pxor xmm0, xmm0
    
	; http://www.tortall.net/projects/yasm/manual/html/manual.html#nasm-macro-rep
	%rep    4
		movdqa [eax], xmm0
		movdqa [eax + 16], xmm0
		movdqa [eax + 32], xmm0
		movdqa [eax + 48], xmm0
		movdqa [eax + 64], xmm0
		movdqa [eax + 80], xmm0
		movdqa [eax + 96], xmm0
		movdqa [eax + 112], xmm0
		movdqa [eax + 128], xmm0
		movdqa [eax + 144], xmm0
		movdqa [eax + 160], xmm0
		movdqa [eax + 176], xmm0
		movdqa [eax + 192], xmm0
		movdqa [eax + 208], xmm0
		movdqa [eax + 224], xmm0
		movdqa [eax + 240], xmm0
		add eax, 256
	%endrep
	
	ret
