; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

;bits 32

%ifndef HL_MATH_X86_MACROS_SSE
    %define HL_MATH_X86_MACROS_SSE
    
    ; Fonction pointers
    extern _hl_math_tap6filter4x2_u16_partial
    extern _hl_math_tap6filter4x1_u32
    
%define _hl_math_tap6filter4x1_u32_asm				[_hl_math_tap6filter4x1_u32]
%define _hl_math_tap6filter4x2_u16_partial_asm		[_hl_math_tap6filter4x2_u16_partial]

;;; "ret" = HL_MATH_TRANSPOSE4X4_SSE2(xmm0, xmm1, xmm2, xmm3)
;;; "ret" will be in xmm0, xmm1, xmm2, xmm3
;;; FIXME: Parameters must not be xmm4...7
%macro HL_MATH_TRANSPOSE4X4_SSE2 4
    movdqa xmm4, %1
    movdqa xmm5, %3
    movdqa xmm6, %1
    movdqa xmm7, %3
	shufps xmm4, %2, 0x44	
	shufps xmm5, %4, 0x44	
	shufps xmm6, %2, 0xEE	
	shufps xmm7, %4, 0xEE
	pshufd %1, xmm4, 0xE4
	shufps %1, xmm5, 0x88
	pshufd %2, xmm4, 0xE4
	shufps %2, xmm5, 0xDD
	pshufd %3, xmm6, 0xE4
	shufps %3, xmm7, 0x88
	pshufd %4, xmm6, 0xE4
	shufps %4, xmm7, 0xDD
%endmacro

;;;
;;; HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41(E, F, G, H, I, J, RET)
;;; Parameters MUST NOT CONTAINS "esi"
;;; This macro changes "xmm0" and "xmm1"
;; Example: HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41 esp+0,esp+0,esp+0,esp+0,esp+0,esp+0,esp+0
%macro HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41 7
	movdqa xmm0, [%2]
	paddd xmm0, [%5]
	movdqa xmm1, [___x86_globals_array4_fives]
	pmulld xmm1, xmm0
	movdqa xmm0, [%1]
	psubd xmm0, xmm1
	movdqa xmm1, [%3]
	paddd xmm1, [%4]
	pmulld xmm1, [___x86_globals_array4_twenties]
	paddd xmm0, xmm1
	paddd xmm0, [%6]
	movdqa [%7], xmm0
%endmacro

;;;
;;; HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE41(E, F, G, H, I, J, RET)
;;; Parameters MUST NOT CONTAINS "esi"
;;; This macro changes "xmm0" and "xmm1"
;; Example: HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3 esp+0,esp+0,esp+0,esp+0,esp+0,esp+0,esp+0
%macro HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3 7
	movdqa xmm0, [%2]
	pshufd xmm1, [___x86_globals_array8_fives], 0xE4
	paddw xmm0, [%5]
	pmullw xmm1, xmm0
	pshufd xmm0, [%1], 0xE4
	psubw xmm0, xmm1
	pshufd xmm1, [%3], 0xE4
	paddw xmm1, [%4]
	pmullw xmm1, [___x86_globals_array8_twenties]
	paddw xmm0, xmm1
	paddw xmm0, [%6]
	movdqa [%7], xmm0
%endmacro


;;;
;;; void HL_MATH_CLIP2_4X1_ASM_SSE41(max, val, ret)
;;; This macro changes "xmm0"
;;;	Example:  HL_MATH_CLIP2_4X1_ASM_SSE41 ecx, eax, edx
%macro HL_MATH_CLIP2_4X1_ASM_SSE41 3	
	pxor xmm0, xmm0
	pmaxsd xmm0, [%2]
	pminsd xmm0, [%1]
	mov edx, [esp + 12]
	movdqa [%3], xmm0
%endmacro

%endif ; /* HL_MATH_X86_MACROS_SSE */
