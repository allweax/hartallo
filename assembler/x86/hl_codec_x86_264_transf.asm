; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

%include "hl_math_x86_macros_sse.asm"

bits 32

global _hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2
global _hl_codec_264_transf_frw_residual4x4_asm_sse2
global _hl_codec_264_transf_frw_hadamard4x4_dc_luma_asm_sse2

section .data
	extern ___x86_globals_array4_thirty_twos

section .text

;;;
;;; void hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2(
;;;		int32_t bitDepth, // ebp + 8 // esp + 4
;;;		HL_ALIGNED(16) int32_t d[4][4], // ebp + 12 // esp + 8
;;;		HL_ALIGNED(16) int32_t r[4][4] // ebp + 16 // esp + 12
;;;		)
_hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2:
	;;;;;;;;;
	;;; e ;;;
	;;;;;;;;;
	mov eax, [esp + 8]; "d"
	movdqa xmm0, [eax]
	movdqa xmm1, [eax + 16]
	movdqa xmm2, [eax + 32]
	movdqa xmm3, [eax + 48]
	HL_MATH_TRANSPOSE4X4_SSE2 xmm0,xmm1,xmm2,xmm3
	pshufd xmm4, xmm0, 0xE4
	paddd xmm4, xmm2
	psubd xmm0, xmm2
	pshufd xmm5, xmm1, 0xE4
	psrad xmm5, 1
	psubd xmm5, xmm3
	psrad xmm3, 1
	paddd xmm3, xmm1
	
	;;;;;;;;;
	;;; f ;;;
	;;;;;;;;;
	movdqa xmm1, xmm0
	pshufd xmm2, xmm0, 0xE4
	paddd xmm1, xmm5
	psubd xmm2, xmm5
	movdqa xmm5, xmm3
	pshufd xmm3, xmm4, 0xE4
	psubd xmm3, xmm5
	pshufd xmm0, xmm4, 0xE4
	paddd xmm0, xmm5
	HL_MATH_TRANSPOSE4X4_SSE2 xmm0,xmm1,xmm2,xmm3
	
	;;;;;;;;;
	;;; g ;;;
	;;;;;;;;;
	movdqa xmm4, xmm0
	movdqa xmm5, xmm1
	paddd xmm4, xmm2
	psubd xmm0, xmm2
	psrad xmm5, 1
	psubd xmm5, xmm3
	psrad xmm3, 1
	paddd xmm3, xmm1
	
	;;;;;;;;;
	;;; h ;;;
	;;;;;;;;;
	movdqa xmm1, xmm4
	movdqa xmm2, xmm0
	paddd xmm1, xmm3
	psubd xmm4, xmm3
	paddd xmm2, xmm5
	psubd xmm0, xmm5
	
	;;;;;;;;;
	;;; r ;;;
	;;;;;;;;;
	pshufd xmm3, [___x86_globals_array4_thirty_twos], 0xE4
	paddd xmm1, xmm3
	paddd xmm2, xmm3
	psrad xmm1, 6
	psrad xmm2, 6
	paddd xmm0, xmm3
	paddd xmm4, xmm3
	psrad xmm0, 6
	psrad xmm4, 6
	
	;;;;;;;;;;;;;;
	;;; RESULT ;;;
	;;;;;;;;;;;;;;
	mov eax, [esp + 12]
	movdqa [eax], xmm1
	movdqa [eax + 16], xmm2
	movdqa [eax + 32], xmm0
	movdqa [eax + 48], xmm4
	
	ret
	

;;; void hl_codec_264_transf_frw_residual4x4_asm_sse2(
;;;		HL_ALIGNED(16) const int32_t in4x4[4][4], 
;;;		HL_ALIGNED(16)int32_t out4x4[4][4]
;;;		)
_hl_codec_264_transf_frw_residual4x4_asm_sse2:
	mov eax, [esp + 4]
	
	movdqa xmm4, [eax]
	movdqa xmm5, [eax + 16]
	movdqa xmm6, [eax + 32]
	movdqa xmm7, [eax + 48]
	
	movdqa xmm0, xmm4
	paddd xmm0, xmm5
	paddd xmm0, xmm6
	paddd xmm0, xmm7

	movdqa xmm1, xmm4
	pslld xmm1, 1
	paddd xmm1, xmm5
	psubd xmm1, xmm6
	psubd xmm1, xmm7
	psubd xmm1, xmm7
	
	movdqa xmm2, xmm4
	psubd xmm2, xmm5
	psubd xmm2, xmm6
	paddd xmm2, xmm7
	
	movdqa xmm3, xmm4
	pslld xmm5, 1
	pslld xmm6, 1
	psubd xmm3, xmm5
	paddd xmm3, xmm6
	psubd xmm3, xmm7
	
	HL_MATH_TRANSPOSE4X4_SSE2 xmm0,xmm1,xmm2,xmm3
	
	movdqa xmm4, xmm0
	movdqa xmm5, xmm1
	movdqa xmm6, xmm2
	movdqa xmm7, xmm3
	
	movdqa xmm0, xmm4
	paddd xmm0, xmm5
	paddd xmm0, xmm6
	paddd xmm0, xmm7

	movdqa xmm1, xmm4
	pslld xmm1, 1
	paddd xmm1, xmm5
	psubd xmm1, xmm6
	psubd xmm1, xmm7
	psubd xmm1, xmm7
	
	movdqa xmm2, xmm4
	psubd xmm2, xmm5
	psubd xmm2, xmm6
	paddd xmm2, xmm7
	
	movdqa xmm3, xmm4
	pslld xmm5, 1
	pslld xmm6, 1
	psubd xmm3, xmm5
	paddd xmm3, xmm6
	psubd xmm3, xmm7
	
	HL_MATH_TRANSPOSE4X4_SSE2 xmm0,xmm1,xmm2,xmm3
	
	mov eax, [esp + 8]
	movdqa [eax], xmm0
	movdqa [eax + 16], xmm1
	movdqa [eax + 32], xmm2
	movdqa [eax + 48], xmm3
	
	ret
	


;;;; void hl_codec_264_transf_frw_hadamard4x4_dc_luma_asm_sse2(
;;;		HL_ALIGNED(16) const int32_t in4x4[4][4], 
;;;		HL_ALIGNED(16) int32_t out4x4[4][4]
;;;		)
_hl_codec_264_transf_frw_hadamard4x4_dc_luma_asm_sse2:
	mov eax, [esp + 4]
	
	movdqa xmm4, [eax]
	movdqa xmm5, [eax + 16]
	movdqa xmm6, [eax + 32]
	movdqa xmm7, [eax + 48]
	
	movdqa xmm0, xmm4
	paddd xmm0, xmm5
	paddd xmm0, xmm6
	paddd xmm0, xmm7

	movdqa xmm1, xmm4
	paddd xmm1, xmm5
	psubd xmm1, xmm6
	psubd xmm1, xmm7
	
	movdqa xmm2, xmm4
	psubd xmm2, xmm5
	psubd xmm2, xmm6
	paddd xmm2, xmm7
	
	movdqa xmm3, xmm4
	psubd xmm3, xmm5
	paddd xmm3, xmm6
	psubd xmm3, xmm7
	
	HL_MATH_TRANSPOSE4X4_SSE2 xmm0,xmm1,xmm2,xmm3
	
	movdqa xmm4, xmm0
	movdqa xmm5, xmm1
	movdqa xmm6, xmm2
	movdqa xmm7, xmm3
	
	movdqa xmm0, xmm4
	paddd xmm0, xmm5
	paddd xmm0, xmm6
	paddd xmm0, xmm7

	movdqa xmm1, xmm4
	paddd xmm1, xmm5
	psubd xmm1, xmm6
	psubd xmm1, xmm7
	
	movdqa xmm2, xmm4
	psubd xmm2, xmm5
	psubd xmm2, xmm6
	paddd xmm2, xmm7
	
	movdqa xmm3, xmm4
	psubd xmm3, xmm5
	paddd xmm3, xmm6
	psubd xmm3, xmm7
	
	HL_MATH_TRANSPOSE4X4_SSE2 xmm0,xmm1,xmm2,xmm3
	
	psrad xmm0, 1
	psrad xmm1, 1
	psrad xmm2, 1
	psrad xmm3, 1

	mov eax, [esp + 8]
	movdqa [eax], xmm0
	movdqa [eax + 16], xmm1
	movdqa [eax + 32], xmm2
	movdqa [eax + 48], xmm3

	ret
	