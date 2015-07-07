; Copyright (C) 2013-2014 Mamadou DIOP
; Copyright (C) 2013-2014 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

%include "hl_math_x86_macros_sse.asm"
%include "hl_globals_x86_sse.asm"

bits 32

global _hl_codec_264_quant_frw4x4_scale_ac_asm_sse41


section .data
	extern _HL_CODEC_264_QUANT_MF
	extern _HL_CODEC_264_QUANT_QBITS
	extern _HL_CODEC_264_QUANT_F

section .text

;;; static void hl_codec_264_quant_frw4x4_scale_ac_asm_sse41(
;;;		int32_t QP, 
;;;		hl_bool_t isIntraBlk, 
;;;		HL_ALIGNED(16) const int32_t in4x4[4][4], 
;;;		HL_ALIGNED(16) int32_t out4x4[4][4]
;;;		)
_hl_codec_264_quant_frw4x4_scale_ac_asm_sse41:
	mov eax, [esp + 4] ; QP
	mov edx, [esp + 8]; isIntraBlk
	and edx, 1
	imul edx, 52*4
	mov ecx, eax
	shl ecx, 2
	add edx, ecx
	mov ecx, [_HL_CODEC_264_QUANT_F + edx]
	
	; xmm0 = xmm_zeros
	pxor xmm0, xmm0
	
	; xmm1 = xmm_ones
	movdqa xmm1, [___x86_globals_array4_ones]
	
	; xmm2 = xmm_f
	movd xmm2, ecx
	pshufb xmm2, [___x86_globals_array4_shuffle_mask_0_0_0_0]

	; edx = mf_index = QP%6
	mov dx, 0
	mov cx, 6
	div cx ; ax/cx = (QP/6), quotient in AX, remainder in DX
	shl edx, 6; imul edx, 4*(4*4)
	lea ecx, [_HL_CODEC_264_QUANT_MF + edx]; ecx = HL_CODEC_264_QUANT_MF[mf_index]

	; xmm7 = xmm_qBits (because "psrad" accepts immediate values only)
	mov eax, [esp + 4] ; QP
	mov edx, [_HL_CODEC_264_QUANT_QBITS + eax*4]
	movd xmm7, edx

	mov eax, [esp + 12] ; eax = in4x4
	mov edx, [esp + 16] ; edx = out4x4
	
	%rep 4
		movdqa xmm3, [ecx]; xmm3 = xmm_quant_mf
		movdqa xmm4, [eax]; xmm4 = xmm_in
		
		; SIGN(xmm_in) -> ((xmm_in)>=0 ? 1 : -1)
		pxor xmm5, xmm5
		movdqa xmm6, xmm4
		pcmpgtd xmm5, xmm4
		pand xmm5, xmm1
		pcmpgtd xmm6, xmm0
		pand xmm6, xmm1
		psubd xmm6, xmm5 ; xmm6 = xmm_sign
		
		; xmm_z = xmm4 = (ABS(xmm_in[i]) * xmm_quant_mf[i] + xmm_f) >> xmm_qBits;
		pabsd xmm4, xmm4
		pmulld xmm4, xmm3
		paddd xmm4, xmm2
		psrad xmm4, xmm7 ; xmm4 = xmm_z
		
		; OUT = xmm_z * xmm_sign
		pmulld xmm4, xmm6
		movdqa [edx], xmm4
		
		add eax, 16
		add ecx, 16
		add edx, 16
	%endrep

	ret