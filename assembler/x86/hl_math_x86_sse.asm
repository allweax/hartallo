; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

%include "hl_math_x86_macros_sse.asm"
%include "hl_globals_x86_sse.asm"

bits 32

global _hl_math_allzero16_asm_sse41
global _hl_math_allzero16_asm_sse2
global _hl_math_clip3_4x1_asm_sse41
global _hl_math_clip2_4x1_asm_sse41
global _hl_math_clip1_4x1_asm_sse41
global _hl_math_tap6filter4x1_u32_asm_sse41
global _hl_math_tap6filter4x4_u8_full_asm_sse3
global _hl_math_tap6filter4x4_u8_partial_asm_sse3
global _hl_math_tap6filter4x2_u16_partial_asm_sse3
global _hl_math_tap6filter4x2_u16_partial_asm_sse41
global _hl_math_addclip_4x4_asm_sse41
global _hl_math_addclip_4x4_u8xi32_asm_sse41
global _hl_math_addclip_8x8_asm_sse41
global _hl_math_addclip_16x16_asm_sse41
global _hl_math_transpose_4x4_asm_sse2
global _hl_math_sad4x4_u8_asm_sse2

extern _hl_math_tap6filter4x1_u32; SSE2 or SSE41 version

section .data
	extern ___x86_globals_array4_zeros
	extern ___x86_globals_array4_one_bits
	extern ___x86_globals_array4_ones
	extern ___x86_globals_array4_fives
	extern ___x86_globals_array4_twenties

section .text

;;;
;;; hl_bool_t hl_math_allzero16_cpp(HL_ALIGNED(16) int32_t (*block)[16])
_hl_math_allzero16_asm_sse41:	
	mov ecx, [esp + 4]	; &block
	xor eax, eax
	
	pshufd xmm1, [___x86_globals_array4_one_bits], 0xE4
	
	pshufd xmm0, [ecx], 0xE4
	ptest xmm0, xmm1
	jnz .end
	
	pshufd xmm0, [ecx + 16], 0xE4
	ptest xmm0, xmm1
	jnz .end
	
	pshufd xmm0, [ecx + 32], 0xE4
	ptest xmm0, xmm1
	jnz .end
	
	pshufd xmm0, [ecx + 48], 0xE4
	ptest xmm0, xmm1
	jnz .end
	
	mov eax, 1
	
	.end
	ret
	
	
;;;
;;; hl_bool_t hl_math_allzero16_cpp(HL_ALIGNED(16) int32_t (*block)[16])
_hl_math_allzero16_asm_sse2:
	mov dword ecx, [esp + 4]	; &block
	xor eax, eax
	
	pxor xmm1, xmm1
	
	pshufd xmm0, [ecx], 0xE4
	pcmpeqb xmm0, xmm1
	pmovmskb edx, xmm0
	cmp edx, 0xFFFF
	jnz .end
	
	pshufd xmm0, [ecx +  16], 0xE4
	pcmpeqb xmm0, xmm1
	pmovmskb edx, xmm0
	cmp edx, 0xFFFF
	jnz .end
	
	pshufd xmm0, [ecx + 32], 0xE4
	pcmpeqb xmm0, xmm1
	pmovmskb edx, xmm0
	cmp edx, 0xFFFF
	jnz .end
	
	pshufd xmm0, [ecx + 48], 0xE4
	pcmpeqb xmm0, xmm1
	pmovmskb edx, xmm0
	cmp edx, 0xFFFF
	jnz .end
	
	mov eax, 1
	
	.end
	ret

;;;
;;; void hl_math_clip3_4x1_asm_sse41(
;;;		const hl_int128_t* min,
;;;		const hl_int128_t* max,
;;;		const hl_int128_t* val,
;;;		hl_int128_t* ret)
_hl_math_clip3_4x1_asm_sse41:	
	mov eax, [esp + 4]	; &min
	mov ecx, [esp + 8]	; &max
	mov edx, [esp + 12]	; &val
	
	movdqa xmm0, [eax]
	pmaxsd xmm0, [edx] ; xmm0 = max(min, val)
	pminsd xmm0, [ecx]; xmm0 = min(max(min, val), max)
	mov edx, [esp + 16]	; &ret
	movdqa [edx], xmm0; ret = min(max(min, val), max)
		
	ret
	
;;;
;;; void hl_math_clip2_4x1_asm_sse41(
;;;		const hl_int128_t* max, // ebp + 8
;;;		const hl_int128_t* val, // ebp + 12
;;;		hl_int128_t* ret // ebp + 16
;;;		)
_hl_math_clip2_4x1_asm_sse41:	
	mov ecx, [esp + 4]
	mov edx, [esp + 8]
	
	pxor xmm0, xmm0
	pmaxsd xmm0, [edx]
	pminsd xmm0, [ecx]
	mov edx, [esp + 12]
	movdqa [edx], xmm0
		
	ret
	
;;;
;;; void hl_math_clip1_4x1_asm_sse41(
;;;		const hl_int128_t* val,
;;;		const hl_int128_t* BitDepth, // BitDepthY or BitDepthC. Only a single int32_t MUST be set (e.g. [8, 0, 0, 0]).
;;;		hl_int128_t* ret)
_hl_math_clip1_4x1_asm_sse41:
	push ebp
	mov ebp, esp
	
	mov ecx, [ebp + 8]; &val
	mov edx, [ebp + 12]; &BitDepth
	
	; xmm0 = ((1 << BitDepth) - 1) = max
	movdqa xmm0, [___x86_globals_array4_ones]
	pslld xmm0, [edx]
	psubd xmm0, [___x86_globals_array4_ones]
	
	;;
	;; call _hl_math_clip3_4x1_asm_sse41(min, max, val, ret)
	; xmm0 = max
	pxor xmm1, xmm1; min
	movdqa xmm2, [ecx]; val
	sub esp, 12; align on 16 bytes
	sub esp, 64; alloc 4x16 for parameters values
	mov ecx, esp; save parameter values start address
	movdqa [esp], xmm1; min
	movdqa [esp + 16], xmm0; max
	movdqa [esp + 32], xmm2; val
	; ret unset
	sub esp, 16; alloc 4x4 for parameter addesses; now ret memory is at [esp + 48 + 16] = [esp + 64]
	mov [esp], ecx
	add ecx, 16
	mov [esp + 4], ecx
	add ecx, 16
	mov [esp + 8], ecx
	add ecx, 16
	mov [esp + 12], ecx
	call _hl_math_clip3_4x1_asm_sse41
	movdqa xmm3, [esp + 64] ; xmm3 = *ret from clip3()
	mov eax, [ebp + 16]; eax = &ret from clip1()
	movdqa [eax], xmm3;
	
	mov esp,ebp ; MUST
	pop ebp
	ret
	

;;;
;;; Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
;;; void hl_math_tap6filter4x1_u32_asm_sse41(
;;;		const hl_int128_t* E, // esp + 4 %1
;;;		const hl_int128_t* F, // esp + 8 %2
;;;		const hl_int128_t* G, // esp + 12 %3
;;;		const hl_int128_t* H, // esp + 16 %4
;;;		const hl_int128_t* I, // esp + 20 %5
;;;		const hl_int128_t* J, // esp + 24 %6
;;;		hl_int128_t* RET)
_hl_math_tap6filter4x1_u32_asm_sse41:	
	mov ecx, [esp + 8]
	movdqa xmm0, [ecx]
	mov ecx, [esp + 20]
	paddd xmm0, [ecx]
	movdqa xmm1, [___x86_globals_array4_fives]
	pmulld xmm1, xmm0
	mov ecx, [esp + 4]
	movdqa xmm0, [ecx]
	psubd xmm0, xmm1
	mov ecx, [esp + 12]
	movdqa xmm1, [ecx]
	mov ecx, [esp + 16]
	paddd xmm1, [ecx]
	pmulld xmm1, [___x86_globals_array4_twenties]
	paddd xmm0, xmm1
	mov ecx, [esp + 24]
	paddd xmm0, [ecx]
	mov ecx, [esp + 28]
	movdqa [ecx], xmm0
	
	ret
	
;;; Full operation
;;; Used in MPEG codecs (e.g. H.264)
;;; Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
;;; RET = Clip(((Tap6Filter + 16) >> 5))
;;; Each parameter contains 16x8b packed values
;;; void hl_math_tap6filter4x4_u8_full_asm_sse3(
;;;		const hl_int128_t* e, // 16x8b // ebp + 8
;;;		const hl_int128_t* f, // 16x8b // ebp + 12
;;;		const hl_int128_t* g, // 16x8b // ebp + 16
;;;		const hl_int128_t* h, // 16x8b // ebp + 20
;;;		const hl_int128_t* i, // 16x8b // ebp + 24
;;;		const hl_int128_t* j, // 16x8b // ebp + 28
;;;		hl_int128_t* ret	  // 16x8b // ebp + 32
;;;		)
_hl_math_tap6filter4x4_u8_full_asm_sse3:
    ;prefetcht0 [esp + 4]
	push ebp
	mov ebp, esp
	
	and esp, -16; align on 16 bytes
    
    ; Local variables: 8x16 = 128 = _e(@112), _f(@96), _g(@80), _h(@64), _i(@48), _j(@32), ret_lo(@16), ret_hi(@0)
    sub esp, 128
	
    mov eax, [ebp + 8]
    movdqa xmm2, [eax]
    mov eax, [ebp + 12]
    movdqa xmm3, [eax]
    mov eax, [ebp + 16]
    movdqa xmm4, [eax]
    mov eax, [ebp + 20]
    movdqa xmm5, [eax]
    mov eax, [ebp + 24]
    movdqa xmm6, [eax]
    mov eax, [ebp + 28]
    movdqa xmm7, [eax]
	
    pxor xmm0, xmm0
    pshufd xmm1, xmm2, 0xE4
    punpcklbw xmm1, xmm0
    movdqa [esp + 112], xmm1
    pshufd xmm1, xmm3, 0xE4
    punpcklbw xmm1, xmm0
    movdqa [esp + 96], xmm1
    pshufd xmm1, xmm4, 0xE4
    punpcklbw xmm1, xmm0
    movdqa [esp + 80], xmm1
    pshufd xmm1, xmm5, 0xE4
    punpcklbw xmm1, xmm0
    movdqa [esp + 64], xmm1
    pshufd xmm1, xmm6, 0xE4
    punpcklbw xmm1, xmm0
    movdqa [esp + 48], xmm1
    pshufd xmm1, xmm7, 0xE4
    punpcklbw xmm1, xmm0
    movdqa [esp + 32], xmm1
    ;;; HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3(E, F, G, H, I, J, RET) ;;;
    HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3 esp+112,esp+96,esp+80,esp+64,esp+48,esp+32,esp+16

    pxor xmm0, xmm0
    punpckhbw xmm2, xmm0
    movdqa [esp + 112], xmm2
    punpckhbw xmm3, xmm0
    movdqa [esp + 96], xmm3
    punpckhbw xmm4, xmm0
    movdqa [esp + 80], xmm4
    punpckhbw xmm5, xmm0
    movdqa [esp + 64], xmm5
    punpckhbw xmm6, xmm0
    movdqa [esp + 48], xmm6
    punpckhbw xmm7, xmm0
    movdqa [esp + 32], xmm7
    ;;; HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3(E, F, G, H, I, J, RET) ;;;
    HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3 esp+112,esp+96,esp+80,esp+64,esp+48,esp+32,esp+0
    
    ;;; RET_LO/HI = ((RET + 16) >> 5) ;;;
    movdqa xmm0, [esp + 16]
    pshufd xmm2, [___x86_globals_array8_sixteens], 0xE4
    paddw xmm0, xmm2
    psraw xmm0, 5
    pshufd xmm1, [esp + 0], 0xE4
    paddw xmm1, xmm2
    psraw xmm1, 5
    
    ;;; Packs the 8 signed 16-bit integers from "ret_lo" and "ret_hi" into 8-bit unsigned integers and saturates.
	;;; Saturate = Clip(0, 255, val)
	packuswb xmm0, xmm1
	
	mov eax, [ebp + 32]
	movdqa [eax], xmm0
	
	mov esp,ebp
	pop ebp
	ret
	
;;; void hl_math_tap6filter4x4_u8_partial_asm_sse3(
;;;		const hl_int128_t* e, // 16x8b // esp + 4
;;;		const hl_int128_t* f, // 16x8b // esp + 8
;;;		const hl_int128_t* g, // 16x8b // esp + 12
;;;		const hl_int128_t* h, // 16x8b // esp + 16
;;;		const hl_int128_t* i, // 16x8b // esp + 20
;;;		const hl_int128_t* j, // 16x8b // esp + 24
;;;		hl_int128_t* ret_lo,  // 8x16b // esp + 28
;;;		hl_int128_t* ret_hi	  // 8x16b // esp + 32
;;;		)
_hl_math_tap6filter4x4_u8_partial_asm_sse3:
    ;prefetcht0 [esp + 4]
    pxor xmm0, xmm0
    
    mov eax, [esp + 4]
    movdqa xmm2, [eax]
    mov eax, [esp + 8]
    movdqa xmm3, [eax]
    mov eax, [esp + 12]
    movdqa xmm4, [eax]
    mov eax, [esp + 16]
    movdqa xmm5, [eax]
    mov eax, [esp + 20]
    movdqa xmm6, [eax]
    mov eax, [esp + 24]
    movdqa xmm7, [eax]
    
    punpcklbw xmm2, xmm0
    punpcklbw xmm3, xmm0
    punpcklbw xmm4, xmm0
    punpcklbw xmm5, xmm0
    punpcklbw xmm6, xmm0
    punpcklbw xmm7, xmm0
    
    ;;; HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3(E, F, G, H, I, J, RET) ;;;
	paddw xmm3, xmm6
	paddw xmm4, xmm5
	pmullw xmm3, [___x86_globals_array8_fives]
	psubw xmm2, xmm3
	pmullw xmm4, [___x86_globals_array8_twenties]
	paddw xmm2, xmm7
	paddw xmm2, xmm4
	mov eax, [esp + 28]
	movdqa [eax], xmm2
    
	mov eax, [esp + 4]
    movdqa xmm2, [eax]
    mov eax, [esp + 8]
    movdqa xmm3, [eax]
    mov eax, [esp + 12]
    movdqa xmm4, [eax]
    mov eax, [esp + 16]
    movdqa xmm5, [eax]
    mov eax, [esp + 20]
    movdqa xmm6, [eax]
    mov eax, [esp + 24]
    movdqa xmm7, [eax]
    
    punpckhbw xmm2, xmm0
    punpckhbw xmm3, xmm0
    punpckhbw xmm4, xmm0
    punpckhbw xmm5, xmm0
    punpckhbw xmm6, xmm0
    punpckhbw xmm7, xmm0
    
    ;;; HL_MATH_TAP6FILTER_4X4_U8_ASM_SSE3(E, F, G, H, I, J, RET) ;;;
	paddw xmm3, xmm6
	paddw xmm4, xmm5
	pmullw xmm3, [___x86_globals_array8_fives]
	psubw xmm2, xmm3
	pmullw xmm4, [___x86_globals_array8_twenties]
	paddw xmm2, xmm7
	paddw xmm2, xmm4
	mov eax, [esp + 32]
	movdqa [eax], xmm2
    
	ret
	

;;; void _hl_math_tap6filter4x2_u16_partial_asm_sse41(
;;;		const hl_int128_t* e, // 8x16b // esp + 4
;;;		const hl_int128_t* f, // 8x16b // esp + 8
;;;		const hl_int128_t* g, // 8x16b // esp + 12
;;;		const hl_int128_t* h, // 8x16b // esp + 16
;;;		const hl_int128_t* i, // 8x16b // esp + 20
;;;		const hl_int128_t* j, // 8x16b // esp + 24
;;;		hl_int128_t* ret_lo,  // 4x32b // esp + 28
;;;		hl_int128_t* ret_hi   // 4x32b // esp + 32
;;;		)
_hl_math_tap6filter4x2_u16_partial_asm_sse41:
    ;prefetcht0 [esp + 4]
    pxor xmm0, xmm0
    
    mov eax, [esp + 4]
    movdqa xmm2, [eax]
    mov eax, [esp + 8]
    movdqa xmm3, [eax]
    mov eax, [esp + 12]
    movdqa xmm4, [eax]
    mov eax, [esp + 16]
    movdqa xmm5, [eax]
    mov eax, [esp + 20]
    movdqa xmm6, [eax]
    mov eax, [esp + 24]
    movdqa xmm7, [eax]
    
    punpcklwd xmm2, xmm0
    punpcklwd xmm3, xmm0
    punpcklwd xmm4, xmm0
    punpcklwd xmm5, xmm0
    punpcklwd xmm6, xmm0
    punpcklwd xmm7, xmm0
    
    ;;; HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41(&_e, &_f, &_g, &_h, &_i, &_j, ret_lo) ;;;
    paddd xmm3, xmm6
    paddd xmm4, xmm5
	pmulld xmm3, [___x86_globals_array4_fives]
	psubd xmm2, xmm3
	pmulld xmm4, [___x86_globals_array4_twenties]
	paddd xmm2, xmm7
	paddd xmm2, xmm4
	mov eax, [esp + 28]
    movdqa [eax], xmm2
    
    mov eax, [esp + 4]
    movdqa xmm2, [eax]
    mov eax, [esp + 8]
    movdqa xmm3, [eax]
    mov eax, [esp + 12]
    movdqa xmm4, [eax]
    mov eax, [esp + 16]
    movdqa xmm5, [eax]
    mov eax, [esp + 20]
    movdqa xmm6, [eax]
    mov eax, [esp + 24]
    movdqa xmm7, [eax]
    
    punpckhwd xmm2, xmm0
    punpckhwd xmm3, xmm0
    punpckhwd xmm4, xmm0
    punpckhwd xmm5, xmm0
    punpckhwd xmm6, xmm0
    punpckhwd xmm7, xmm0
    
    ;;; HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41(&_e, &_f, &_g, &_h, &_i, &_j, ret_hi) ;;;
    paddd xmm3, xmm6
    paddd xmm4, xmm5
	pmulld xmm3, [___x86_globals_array4_fives]
	psubd xmm2, xmm3
	pmulld xmm4, [___x86_globals_array4_twenties]
	paddd xmm2, xmm7
	paddd xmm2, xmm4
	mov eax, [esp + 32]
	movdqa [eax], xmm2
	
	ret

;;; void hl_math_tap6filter4x2_u16_partial_asm_sse3(
;;;		const hl_int128_t* e, // 8x16b
;;;		const hl_int128_t* f, // 8x16b
;;;		const hl_int128_t* g, // 8x16b
;;;		const hl_int128_t* h, // 8x16b
;;;		const hl_int128_t* i, // 8x16b
;;;		const hl_int128_t* j, // 8x16b
;;;		hl_int128_t* ret_lo,  // 4x32b
;;;		hl_int128_t* ret_hi   // 4x32b
;;;		)
_hl_math_tap6filter4x2_u16_partial_asm_sse3:
    ;prefetcht0 [esp + 4]
	push ebp
	mov ebp, esp
	
	and esp, -16; align on 16 bytes
    
    ; Local variables: 8x16 = 128 = _e(@108), _f(@92), _g(@76), _h(@60), _i(@44), _j(@28)
    ; Parameter addresses: 7x4 = 28
    sub esp, 156    
    
    lea eax, [esp + 108]
    mov [esp], eax
    lea eax, [esp + 92]
    mov [esp + 4], eax
    lea eax, [esp + 76]
    mov [esp + 8], eax
    lea eax, [esp + 60]
    mov [esp + 12], eax
    lea eax, [esp + 44]
    mov [esp + 16], eax
    lea eax, [esp + 28]
    mov [esp + 20], eax
    
    mov eax, [ebp + 8]
    movdqa xmm1, [eax]
    mov eax, [ebp + 12]
    movdqa xmm2, [eax]
    mov eax, [ebp + 16]
    movdqa xmm3, [eax]
    mov eax, [ebp + 20]
    movdqa xmm4, [eax]
    mov eax, [ebp + 24]
    movdqa xmm5, [eax]
    mov eax, [ebp + 28]
    movdqa xmm6, [eax]
    
    pxor xmm0, xmm0
    punpcklwd xmm1, xmm0
    movdqa [esp + 108], xmm1
    punpcklwd xmm2, xmm0
    movdqa [esp + 92], xmm2
    punpcklwd xmm3, xmm0
    movdqa [esp + 76], xmm3
    punpcklwd xmm4, xmm0
    movdqa [esp + 60], xmm4
    punpcklwd xmm5, xmm0
    movdqa [esp + 44], xmm5
    punpcklwd xmm6, xmm0
    movdqa [esp + 28], xmm6
    
    ;;; hl_math_tap6filter4x1_u32(&_e, &_f, &_g, &_h, &_i, &_j, ret_lo) ;;;
    mov eax, [ebp + 32]
    mov [esp + 24], eax
    call _hl_math_tap6filter4x1_u32_asm
    
    mov eax, [ebp + 8]
    movdqa xmm1, [eax]
    mov eax, [ebp + 12]
    movdqa xmm2, [eax]
    mov eax, [ebp + 16]
    movdqa xmm3, [eax]
    mov eax, [ebp + 20]
    movdqa xmm4, [eax]
    mov eax, [ebp + 24]
    movdqa xmm5, [eax]
    mov eax, [ebp + 28]
    movdqa xmm6, [eax]
    
    pxor xmm0, xmm0
    punpckhwd xmm1, xmm0
    movdqa [esp + 108], xmm1
    punpckhwd xmm2, xmm0
    movdqa [esp + 92], xmm2
    punpckhwd xmm3, xmm0
    movdqa [esp + 76], xmm3
    punpckhwd xmm4, xmm0
    movdqa [esp + 60], xmm4
    punpckhwd xmm5, xmm0
    movdqa [esp + 44], xmm5
    punpckhwd xmm6, xmm0
    movdqa [esp + 28], xmm6
    
    ;;; hl_math_tap6filter4x1_u32(&_e, &_f, &_g, &_h, &_i, &_j, ret_hi) ;;;
    mov eax, [ebp + 36]
    mov [esp + 24], eax
    call _hl_math_tap6filter4x1_u32_asm
    
    mov esp,ebp
	pop ebp
	ret
	
;;; RET = Clip2(MAX, (M1 + M2)), MIN is ZERO
;;; FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
;;; Strides must be multiple of "4"
;;; void hl_math_addclip_4x4_asm_sse41(
;;;		HL_ALIGNED(16) const int32_t* m1, // ebp + 8 // esp + 4
;;;		int32_t m1_stride, // ebp + 12 // esp + 8
;;;		HL_ALIGNED(16) const int32_t* m2, // ebp + 16 // esp + 12
;;;		int32_t m2_stride, // ebp + 20 // esp + 16
;;;		HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */ // ebp + 24 // esp + 20
;;;		HL_ALIGNED(16) int32_t* ret, // ebp + 28 // esp + 24
;;;		int32_t ret_stride // ebp + 32 // esp + 28
;;;		)
_hl_math_addclip_4x4_asm_sse41:
	mov eax, [esp + 4] ; m1
	mov edx, [esp + 8] ; m1_stride
	movdqa xmm0, [eax]
	movdqa xmm1, [eax + edx*4]
	movdqa xmm2, [eax + edx*8]
	lea eax, [eax + edx*8]
	movdqa xmm3, [eax + edx*4]
	
	mov eax, [esp + 12] ; m2
	mov edx, [esp + 16] ; m2_stride
	movdqa xmm4, [eax]
	movdqa xmm5, [eax + edx*4]
	movdqa xmm6, [eax + edx*8]
	lea eax, [eax + edx*8]
	movdqa xmm7, [eax + edx*4]
	
	paddd xmm0, xmm4
	paddd xmm1, xmm5
	paddd xmm2, xmm6
	paddd xmm3, xmm7
	
	pxor xmm4, xmm4 ; min = 0
	mov eax, [esp + 20] ; max
	movdqa xmm5, [eax]
		
	pmaxsd xmm0, xmm4
	pmaxsd xmm1, xmm4
	pmaxsd xmm2, xmm4
	pmaxsd xmm3, xmm4
	
	pminsd xmm0, xmm5
	pminsd xmm1, xmm5
	pminsd xmm2, xmm5
	pminsd xmm3, xmm5
	
	mov eax, [esp + 24] ; ret
	mov edx, [esp + 28] ; ret_stride
	movdqa [eax], xmm0
	movdqa [eax + edx*4], xmm1
	movdqa [eax + edx*8], xmm2
	lea eax, [eax + edx*8]
	movdqa [eax + edx*4], xmm3
	
	ret
	
;;; void hl_math_addclip_4x4_u8xi32_asm_sse41(
;;;		HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride,
;;;		HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
;;;		HL_ALIGNED(16) uint8_t* ret, int32_t ret_stride
;;;		)
_hl_math_addclip_4x4_u8xi32_asm_sse41:
	push ebx
	pxor xmm0, xmm0
	
	mov eax, [esp + 8] ; m1
	mov ebx, [esp + 12] ; m1_stride
	mov ecx, [esp + 16] ; m2
	mov edx, [esp + 20] ; m2_stride
	shl edx, 2
	
	movd xmm1, [eax]
	punpcklbw xmm1, xmm0
	punpcklwd xmm1, xmm0
	paddd xmm1, [ecx]
	
	movd xmm2, [eax + ebx]
	punpcklbw xmm2, xmm0
	punpcklwd xmm2, xmm0
	paddd xmm2, [ecx + edx]
	
	movd xmm3, [eax + ebx*2]
	punpcklbw xmm3, xmm0
	punpcklwd xmm3, xmm0
	paddd xmm3, [ecx + edx*2]
	
	lea eax, [eax + ebx*2]
	lea ecx, [ecx + edx*2]
	movd xmm4, [eax + ebx]
	punpcklbw xmm4, xmm0
	punpcklwd xmm4, xmm0
	paddd xmm4, [ecx + edx]
	
	packusdw xmm1, xmm2
	packusdw xmm3, xmm4
	packuswb xmm1, xmm3
	
	mov eax, [esp + 24]; ret
	mov ecx, [esp + 28]; ret_side
	cmp ecx, 4
	je mov_all_in_one
	movd [eax], xmm1
	psrldq xmm1, 4
	movd [eax + ecx], xmm1
	psrldq xmm1, 4
	movd [eax + ecx*2], xmm1
	lea eax, [eax + ecx*2]
	psrldq xmm1, 4
	movd [eax + ecx], xmm1
	jmp end
mov_all_in_one:
	movdqa [eax], xmm1
end:
	pop ebx
	ret

	
;;; RET = Clip2(MAX, (M1 + M2))
;;; FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
;;; FIXME: No cache optimization. See "_hl_math_addclip_4x4_asm_sse41"
;;; Strides must be multiple of "4" and more than 8
;;; void hl_math_addclip_8x8_asm_sse41(
;;;		HL_ALIGNED(16) const int32_t* m1, // ebp + 8
;;;		int32_t m1_stride, // ebp + 12
;;;		HL_ALIGNED(16) const int32_t* m2, // ebp + 16
;;;		int32_t m2_stride, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */ // ebp + 24
;;;		HL_ALIGNED(16) int32_t* ret, // ebp + 28
;;;		int32_t ret_stride // ebp + 32
;;;		)
_hl_math_addclip_8x8_asm_sse41:
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	
	pxor xmm0, xmm0; min
	mov eax, [ebp + 24]
	movdqa xmm1, [eax]; max
	
	mov eax, [ebp + 8]; m1
	mov ebx, [ebp + 16]; m2
	mov ecx, [ebp + 28]; ret
	
	mov edx, [ebp + 12]; m1_stride
	mov esi, [ebp + 20]; m2_stride
	mov edi, [ebp + 32]; ret_stride
	
	shl edx, 2
	shl esi, 2
	shl edi, 2
	
	%rep 8
		movdqa xmm2, [eax]
		paddd xmm2, [ebx]
		pmaxsd xmm2, xmm0
		pminsd xmm2, xmm1
		movdqa [ecx], xmm2
		movdqa xmm2, [eax + 16]
		paddd xmm2, [ebx + 16]
		pmaxsd xmm2, xmm0
		pminsd xmm2, xmm1
		movdqa [ecx + 16], xmm2
		
		add eax, edx
		add ebx, esi
		add ecx, edi
	%endrep
	
	pop edi
	pop esi
	pop ebx
	pop ebp
	ret
	
	;;; RET = Clip2(MAX, (M1 + M2))
;;; FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
;;; FIXME: No cache optimization. See "_hl_math_addclip_4x4_asm_sse41"
;;; Strides must be multiple of "4" and more than 16
;;; void hl_math_addclip_8x8_asm_sse41(
;;;		HL_ALIGNED(16) const int32_t* m1, // ebp + 8
;;;		int32_t m1_stride, // ebp + 12
;;;		HL_ALIGNED(16) const int32_t* m2, // ebp + 16
;;;		int32_t m2_stride, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */ // ebp + 24
;;;		HL_ALIGNED(16) int32_t* ret, // ebp + 28
;;;		int32_t ret_stride // ebp + 32
;;;		)
_hl_math_addclip_16x16_asm_sse41:
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	
	pxor xmm0, xmm0; min
	mov eax, [ebp + 24]
	movdqa xmm1, [eax]; max
	
	mov eax, [ebp + 8]; m1
	mov ebx, [ebp + 16]; m2
	mov ecx, [ebp + 28]; ret
	
	mov edx, [ebp + 12]; m1_stride
	mov esi, [ebp + 20]; m2_stride
	mov edi, [ebp + 32]; ret_stride
	
	shl edx, 2
	shl esi, 2
	shl edi, 2
	
	%rep 16
		movdqa xmm2, [eax]
		paddd xmm2, [ebx]
		pmaxsd xmm2, xmm0
		pminsd xmm2, xmm1
		movdqa [ecx], xmm2
		movdqa xmm2, [eax + 16]
		paddd xmm2, [ebx + 16]
		pmaxsd xmm2, xmm0
		pminsd xmm2, xmm1
		movdqa [ecx + 16], xmm2
		movdqa xmm2, [eax + 32]
		paddd xmm2, [ebx + 32]
		pmaxsd xmm2, xmm0
		pminsd xmm2, xmm1
		movdqa [ecx + 32], xmm2
		movdqa xmm2, [eax + 48]
		paddd xmm2, [ebx + 48]
		pmaxsd xmm2, xmm0
		pminsd xmm2, xmm1
		movdqa [ecx + 48], xmm2
		
		add eax, edx
		add ebx, esi
		add ecx, edi
	%endrep
	
	pop edi
	pop esi
	pop ebx
	pop ebp
	ret
	
;;; void hl_math_transpose_4x4_asm_sse2(
;;;		const int32_t m[4][4], // ebp + 8 / esp + 4
;;;		int32_t ret[4][4] // ebp + 12 / esp + 8
;;;		)
_hl_math_transpose_4x4_asm_sse2:
    ;prefetcht0 [esp + 4]
	mov eax, [esp + 4]; m
	mov ecx, [esp + 8]; ret
	
	;prefetcht0 [eax]
	;prefetcht0 [ecx]
	
	movdqa xmm0, [eax]
	movdqa xmm1, [eax + 16]
	movdqa xmm2, [eax + 32]
	movdqa xmm3, [eax + 48]
	HL_MATH_TRANSPOSE4X4_SSE2 xmm0,xmm1,xmm2,xmm3
	movdqa [ecx], xmm0
	movdqa [ecx + 16], xmm1
	movdqa [ecx + 32], xmm2
	movdqa [ecx + 48], xmm3
	
	ret
	
	
;;; int32_t hl_math_sad4x4_u8_asm_sse2(
;;;		const uint8_t* b1, // esp + 4
;;;		int32_t b1_stride,  // esp + 8
;;;		const uint8_t* b2,  // esp + 12
;;;		int32_t b2_stride // esp + 16
;;;		)
_hl_math_sad4x4_u8_asm_sse2:	
	mov eax, [esp + 4]; b1
	mov ecx, [esp + 8]; b1_stride
	; xmm0 = _mm_set_epi8(b1[....])
	movd xmm0, [eax]
	movd xmm5, [eax + ecx]
	movd xmm6, [eax + ecx*2]
	lea eax, [eax + ecx*2]
	movd xmm7, [eax + ecx]
	pslldq xmm5, 4
	pslldq xmm6, 8
	pslldq xmm7, 12
	paddb xmm0, xmm5
	paddb xmm0, xmm6
	paddb xmm0, xmm7
	
	mov eax, [esp + 12]; b2
	mov ecx, [esp + 16]; b2_stride
	; xmm1 = _mm_set_epi8(b2[....])
	movd xmm1, [eax]
	movd xmm5, [eax + ecx]
	movd xmm6, [eax + ecx*2]
	lea eax, [eax + ecx*2]
	movd xmm7, [eax + ecx]
	pslldq xmm5, 4
	pslldq xmm6, 8
	pslldq xmm7, 12
	paddb xmm1, xmm5
	paddb xmm1, xmm6
	paddb xmm1, xmm7	
	
	; ret = sad(xmm0, xmm1)
	psadbw xmm0, xmm1
	movd eax, xmm0
	psrldq xmm0, 8
	movd edx, xmm0
	add eax, edx

	ret