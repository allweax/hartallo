; Copyright (C) 2014 Mamadou DIOP
; Copyright (C) 2014 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

%include "hl_math_x86_macros_sse.asm"
%include "hl_globals_x86_sse.asm"

bits 32

global _hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse2
global _hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse3
global _hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_asm_sse2
global _hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_asm_sse2
global _hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_asm_sse2
global _hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_asm_ssse3
global _hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_asm_ssse3
global _hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_asm_ssse3
global _hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_asm_ssse3
global _hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_asm_ssse3
global _hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_asm_sse2

section .data
	extern _HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE

section .text

;;;
;;;static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse2(
;;;    const uint8_t *pc_luma_samples, uint32_t u_luma_stride,
;;;    HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse2:
	mov ecx, [esp + 4] ;pc_luma_samples
	mov edx, [esp + 8] ;u_luma_stride
	
	movdqu xmm0, [ecx]
	movdqu xmm1, [ecx + edx]
	movdqu xmm2, [ecx + edx*2]
	lea ecx, [ecx + edx*2]
	movdqu xmm3, [ecx + edx]
	movdqu xmm4, [ecx + edx*2]
	lea ecx, [ecx + edx*2]
	movdqu xmm5, [ecx + edx]
	movdqu xmm6, [ecx + edx*2]
	lea ecx, [ecx + edx*2]
	movdqu xmm7, [ecx + edx]
	
	mov ecx, [esp + 12] ;ecx = p	
	mov edx, [esp + 16] ;edx = q
	
	movdqa [ecx + 48], xmm0
	movdqa [ecx + 32], xmm1
	movdqa [ecx + 16], xmm2
	movdqa [ecx], xmm3
	
	movdqa [edx], xmm4
	movdqa [edx + 16], xmm5
	movdqa [edx + 32], xmm6
	movdqa [edx + 48], xmm7
	
	ret
	
;;;
;;;static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse3(
;;;    const uint8_t *pc_luma_samples, uint32_t u_luma_stride,
;;;    HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse3:
	mov ecx, [esp + 4] ;pc_luma_samples
	mov edx, [esp + 8] ;u_luma_stride
	
	lddqu xmm0, [ecx]
	lddqu xmm1, [ecx + edx]
	lddqu xmm2, [ecx + edx*2]
	lea ecx, [ecx + edx*2]
	lddqu xmm3, [ecx + edx]
	lddqu xmm4, [ecx + edx*2]
	lea ecx, [ecx + edx*2]
	lddqu xmm5, [ecx + edx]
	lddqu xmm6, [ecx + edx*2]
	lea ecx, [ecx + edx*2]
	lddqu xmm7, [ecx + edx]
	
	mov ecx, [esp + 12] ;ecx = p	
	mov edx, [esp + 16] ;edx = q
	
	movdqa [ecx + 48], xmm0
	movdqa [ecx + 32], xmm1
	movdqa [ecx + 16], xmm2
	movdqa [ecx], xmm3
	
	movdqa [edx], xmm4
	movdqa [edx + 16], xmm5
	movdqa [edx + 32], xmm6
	movdqa [edx + 48], xmm7
	
	ret
	
;;;
;;; static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_asm_sse2(
;;;    const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
;;;    HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_asm_sse2:
	mov edx, [esp + 12] ; edx = u_chroma_stride
	mov ecx, [esp + 4] ; ecx = pc_cb_samples
	mov eax, [esp + 8] ; ecx = pc_cr_samples
	movlps xmm0, [ecx]
	movhps xmm0, [eax]
	movlps xmm1, [ecx + edx]
	movhps xmm1, [eax + edx]
	movlps xmm2, [ecx + edx*2]
	movhps xmm2, [eax + edx*2]
	lea ecx, [ecx + edx*2]
	lea eax, [eax + edx*2]
	movlps xmm3, [ecx + edx]
	movhps xmm3, [eax + edx]
	movlps xmm4, [ecx + edx*2]
	movhps xmm4, [eax + edx*2]
	lea ecx, [ecx + edx*2]
	lea eax, [eax + edx*2]
	movlps xmm5, [ecx + edx]
	movhps xmm5, [eax + edx]
	movlps xmm6, [ecx + edx*2]
	movhps xmm6, [eax + edx*2]
	lea ecx, [ecx + edx*2]
	lea eax, [eax + edx*2]
	movlps xmm7, [ecx + edx]
	movhps xmm7, [eax + edx]
	
	mov eax, [esp + 16] ; eax = p
	movdqa [eax + 48], xmm0
	movdqa [eax + 32], xmm1
	movdqa [eax + 16], xmm2
	movdqa [eax], xmm3
	
	mov eax, [esp + 20] ; eax = q
	movdqa [eax], xmm4
	movdqa [eax + 16], xmm5
	movdqa [eax + 32], xmm6
	movdqa [eax + 48], xmm7
	
	ret
	
;;;
;;; static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_asm_sse2(
;;;    uint8_t *pc_luma_samples, uint32_t u_luma_stride,
;;;    HL_ALIGNED(16) const uint8_t pf[3/*pf0,pf1,pf2*/][16], HL_ALIGNED(16) const uint8_t qf[3/*qf0,qf1,qf2*/][16]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_asm_sse2:	
	mov eax, [esp + 12] ;eax = pf
	movdqa xmm0, [eax + 32]
	movdqa xmm1, [eax + 16]
	movdqa xmm2, [eax]
	mov eax, [esp + 16] ;eax = qf
	movdqa xmm3, [eax]
	movdqa xmm4, [eax + 16]
	movdqa xmm5, [eax + 32]
	
	mov ecx, [esp + 4] ; ecx = pc_luma_samples
	mov edx, [esp + 8] ; edx = u_luma_stride
	add ecx, edx ; skip pf/qf[3] and take p/qf[0,2,2]
	movdqu [ecx], xmm0
	movdqu [ecx + edx], xmm1
	movdqu [ecx + edx*2], xmm2
	lea ecx, [ecx + edx*2]
	movdqu [ecx + edx], xmm3
	movdqu [ecx + edx*2], xmm4
	lea ecx, [ecx + edx*2]
	movdqu [ecx + edx], xmm5
	
	ret
	
;;;
;;; static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_asm_sse2(
;;;    uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
;;;    HL_ALIGNED(16) const uint8_t pf[3/*pf0,pf1,pf2*/][16], HL_ALIGNED(16) const uint8_t qf[3/*qf0,qf1,qf2*/][16]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_asm_sse2:
	mov eax, [esp + 16] ; eax = pf
	mov ecx, [esp + 20] ; ecx = qf
	movdqa xmm0, [eax + 32]
	movdqa xmm1, [eax + 16]
	movdqa xmm2, [eax]
	movdqa xmm3, [ecx]
	movdqa xmm4, [ecx + 16]
	movdqa xmm5, [ecx + 32]
	
	mov edx, [esp + 12] ; edx = u_chroma_stride
	mov ecx, [esp + 4] ; ecx = pc_cb_samples
	mov eax, [esp + 8] ; ecx = pc_cr_samples
	add ecx, edx ;skip p/q[3]
	add eax, edx ;skip p/q[3]
	movlps [ecx], xmm0
	movhps [eax], xmm0
	movlps [ecx + edx], xmm1
	movhps [eax + edx], xmm1
	movlps [ecx + edx*2], xmm2
	movhps [eax + edx*2], xmm2
	lea ecx, [ecx + edx*2]
	lea eax, [eax + edx*2]
	movlps [ecx + edx], xmm3
	movhps [eax + edx], xmm3
	movlps [ecx + edx*2], xmm4
	movhps [eax + edx*2], xmm4
	lea ecx, [ecx + edx*2]
	lea eax, [eax + edx*2]
	movlps [ecx + edx], xmm5
	movhps [eax + edx], xmm5
	
	ret
	
	
;;;
;;; void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_asm_ssse3(
;;;    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
;;;    int16_t bS[2],
;;;    int16_t alpha, int16_t beta,
;;;    HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_asm_ssse3:
	pxor xmm0, xmm0
	
	; xmm4[p0] = _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0])
	mov eax, [esp + 4]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm0
	; xmm5[q0] = _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0])
	mov eax, [esp + 8]
	movlps xmm5, [eax]
	punpcklbw xmm5, xmm0
	; xmm6[p1] = _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0])
	mov eax, [esp + 12]
	movlps xmm6, [eax]
	punpcklbw xmm6, xmm0
	; xmm7[q1] = _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0])
	mov eax, [esp + 16]
	movlps xmm7, [eax]
	punpcklbw xmm7, xmm0
	
	;;; bS[0] != 0 ;;;
	; xmm1[bS] = _mm_set_epi16(bS[1], bS[1], bS[1], bS[1], bS[0], bS[0], bS[0], bS[0]))
	mov eax, [esp + 20]
	movss xmm1, [eax]
	pshufb xmm1, [___x86_globals_array4_shuffle_mask_0_0_0_0_1_1_1_1]
	; xmm1[cmp] = _mm_cmpgt_epi16(xmm1[bS], _mm_setzero_si128())
	pcmpgtw xmm1, xmm0
	
	;;; |p0 - q0| < alpha ;;;
	; xmm2 = |p0 - q0|
	pshufd xmm2, xmm4, 0xE4
	psubw xmm2, xmm5
	pabsw xmm2, xmm2
	; xmm3[alpha] = _mm_set1_epi16(alpha)
	movss xmm3, [esp + 24]
	pshufb xmm3, [___x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0]
	; xmm3 = _mm_cmplt_epi16(xmm2, xmm3[alpha]) = _mm_cmpgt_epi16(xmm3[alpha]), xmm2)
	pcmpgtw xmm3, xmm2
	; xmm1[cmp] = _mm_and_si128(xmm1[cmp], xmm3)
	pand xmm1, xmm3
	
	;;; |p1 - p0| < beta ;;;
	psubw xmm6, xmm4
	pabsw xmm6, xmm6
	; xmm4[beta] = _mm_set1_epi16(beta)
	movss xmm4, [esp + 28]
	pshufb xmm4, [___x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0]
	; xmm0 = _mm_cmplt_epi16(xmm6, xmm4[beta]) = _mm_cmpgt_epi16(xmm4[beta], xmm6)
	pshufd xmm0, xmm4, 0xE4
	pcmpgtw xmm0, xmm6
	; xmm1[cmp] = _mm_and_si128(xmm1[cmp], xmm0)
	pand xmm1, xmm0	
	
	;;; |q1 - q0| < beta ;;;
	psubw xmm7, xmm5
	pabsw xmm7, xmm7
	; xmm4 = _mm_cmplt_epi16(xmm7, xmm4[beta]) = _mm_cmpgt_epi16(xmm4[beta], xmm7)
	pcmpgtw xmm4, xmm7
	; xmm1[cmp] = _mm_and_si128(xmm1[cmp], xmm4)
	pand xmm1, xmm4
	
	;;; filterSamplesFlag = xmm1[cmp] & 1 ;;;
	pand xmm1, [___x86_globals_array8_ones]
	mov eax, [esp + 32]
	movdqa [eax], xmm1
	
	ret
	
;;;
;;; static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_asm_ssse3(
;;;    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
;;;    int16_t bS[4],
;;;    int16_t alpha, int16_t beta,
;;;    HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_asm_ssse3:
	pxor xmm0, xmm0
	
	; xmm4[p0] = _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0])
	mov eax, [esp + 4]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm0
	; xmm5[q0] = _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0])
	mov eax, [esp + 8]
	movlps xmm5, [eax]
	punpcklbw xmm5, xmm0
	; xmm6[p1] = _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0])
	mov eax, [esp + 12]
	movlps xmm6, [eax]
	punpcklbw xmm6, xmm0
	; xmm7[q1] = _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0])
	mov eax, [esp + 16]
	movlps xmm7, [eax]
	punpcklbw xmm7, xmm0
	
	;;; bS[0] != 0 ;;;
	; xmm1[bS] = _mm_set_epi16(bS[3], bS[3], bS[2], bS[2], bS[1], bS[1], bS[0], bS[0]))
	mov eax, [esp + 20]
	movlps xmm1, [eax]
	pshufb xmm1, [___x86_globals_array4_shuffle_mask_0_0_1_1_2_2_3_3]
	; xmm1[cmp] = _mm_cmpgt_epi16(xmm1[bS], _mm_setzero_si128())
	pcmpgtw xmm1, xmm0
	
	;;; |p0 - q0| < alpha ;;;
	; xmm2 = |p0 - q0|
	pshufd xmm2, xmm4, 0xE4
	psubw xmm2, xmm5
	pabsw xmm2, xmm2
	; xmm3[alpha] = _mm_set1_epi16(alpha)
	movss xmm3, [esp + 24]
	pshufb xmm3, [___x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0]
	; xmm3 = _mm_cmplt_epi16(xmm2, xmm3[alpha]) = _mm_cmpgt_epi16(xmm3[alpha]), xmm2)
	pcmpgtw xmm3, xmm2
	; xmm1[cmp] = _mm_and_si128(xmm1[cmp], xmm3)
	pand xmm1, xmm3
	
	;;; |p1 - p0| < beta ;;;
	psubw xmm6, xmm4
	pabsw xmm6, xmm6
	; xmm4[beta] = _mm_set1_epi16(beta)
	movss xmm4, [esp + 28]
	pshufb xmm4, [___x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0]
	; xmm0 = _mm_cmplt_epi16(xmm6, xmm4[beta]) = _mm_cmpgt_epi16(xmm4[beta], xmm6)
	pshufd xmm0, xmm4, 0xE4
	pcmpgtw xmm0, xmm6
	; xmm1[cmp] = _mm_and_si128(xmm1[cmp], xmm0)
	pand xmm1, xmm0	
	
	;;; |q1 - q0| < beta ;;;
	psubw xmm7, xmm5
	pabsw xmm7, xmm7
	; xmm4 = _mm_cmplt_epi16(xmm7, xmm4[beta]) = _mm_cmpgt_epi16(xmm4[beta], xmm7)
	pcmpgtw xmm4, xmm7
	; xmm1[cmp] = _mm_and_si128(xmm1[cmp], xmm4)
	pand xmm1, xmm4
	
	;;; filterSamplesFlag = xmm1[cmp] & 1 ;;;
	pand xmm1, [___x86_globals_array8_ones]
	mov eax, [esp + 32]
	movdqa [eax], xmm1
	
	ret
	
;;; static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_asm_ssse3(
;;;    const uint8_t *p0[ebp + 8], const uint8_t *p1[ebp + 12], const uint8_t *p2[ebp + 16],
;;;    const uint8_t *q0[ebp + 20], const uint8_t *q1[ebp + 24], const uint8_t *q2[ebp + 28],
;;;    int16_t bS[2][ebp + 32],
;;;    int16_t indexA[ebp + 36],
;;;    int16_t beta[ebp + 40],
;;;    HL_OUT uint8_t *pf0[ebp + 44], HL_OUT uint8_t *pf1[ebp + 48], HL_OUT uint8_t *pf2[ebp + 52],
;;;    HL_OUT uint8_t *qf0[ebp + 56], HL_OUT uint8_t *qf1[ebp + 60], HL_OUT uint8_t *qf2[ebp + 64]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_asm_ssse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 9x16 = 144 = xmm_tc(@128), xmm_delta(@112), xmm_p0(@96), xmm_p1(@80), xmm_p2(@64), xmm_q0(@48), xmm_q1(@32), xmm_q2(@16), xmm_beta(@0)
    sub esp, 144
    
    ;;; xmm0[xmm_tc0] = _mm_set_epi16(T[IA][bS[1]], T[IA][bS[1]], T[IA][bS[1]], T[IA][bS[1]], T[IA][bS[0]], T[IA][bS[0]], T[IA][bS[0]], T[IA][bS[0]])) ;;;
    xor edx, edx
	mov dx, [ebp + 36]
	imul edx, 20
	mov ecx, [ebp + 32]
	mov eax, [ecx] ; [bS0,bS1]
	xor ecx, ecx
	mov cx, dx
	shl ax, 2
	add cx, ax
	lea ecx, [_HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE + ecx]
	and eax, 0xffff0000
	shr eax, 14
	add edx, eax
	lea edx, [_HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE + edx]
	xor eax, eax
	add eax, [edx]
	shl eax, 16
	or eax, [ecx]
	mov [esp], eax
	movss xmm0, [esp]
	pshufb xmm0, [___x86_globals_array4_shuffle_mask_0_0_0_0_1_1_1_1]
	
	;;; _mm_store_si128((__m128i*)&xmm_beta[xmm2], _mm_set1_epi16(beta)) ;;;
	movss xmm2, [ebp + 40]
	pshufb xmm2, [___x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0]
	movdqa [esp + 0], xmm2
	
	pxor xmm7, xmm7
	
	;;; _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0])) ;;;
	mov eax, [ebp + 20]
	movlps xmm1, [eax]
	punpcklbw xmm1, xmm7
	movdqa [esp + 48], xmm1
	;;; _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0])) ;;;
	mov eax, [ebp + 24]
	movlps xmm1, [eax]
	punpcklbw xmm1, xmm7
	movdqa [esp + 32], xmm1
	;;; _mm_store_si128((__m128i*)&xmm_q2, _mm_set_epi16(q2[7], q2[6], q2[5], q2[4], q2[3], q2[2], q2[1], q2[0])) ;;;
	mov eax, [ebp + 28]
	movlps xmm1, [eax]
	punpcklbw xmm1, xmm7
	movdqa [esp + 16], xmm1
	
	;;; _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0])) ;;;
	mov eax, [ebp + 8]
	movlps xmm1, [eax]
	punpcklbw xmm1, xmm7
	movdqa [esp + 96], xmm1
	;;; _mm_store_si128((__m128i*)&xmm_p2, _mm_set_epi16(p2[7], p2[6], p2[5], p2[4], p2[3], p2[2], p2[1], p2[0])) ;;;
	mov eax, [ebp + 16]
	movlps xmm1, [eax]
	punpcklbw xmm1, xmm7
	movdqa [esp + 64], xmm1
	;;; _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0])) ;;;
	mov eax, [ebp + 12]
	movlps xmm1, [eax]
	punpcklbw xmm1, xmm7
	movdqa [esp + 80], xmm1

	;;; (8-471), xmm3[ap] = |p2 - p0| ;;;
	;;; (8-472), xmm4[aq] = |q2 - q0| ;;;
	movdqa xmm3, [esp + 64]
	movdqa xmm4, [esp + 16]
	psubw xmm3, [esp + 96]
	pabsw xmm3, xmm3	
	psubw xmm4, [esp + 48]
	pabsw xmm4, xmm4
	 
	;;; (8-473), xmm5[tc] = tc0 + ((xmm2[ap] < xmm2[beta]) ? 1 : 0) + ((xmm3[aq] < xmm2[beta]) ? 1 : 0) ;;;
	movdqa xmm5, xmm2
	movdqa xmm6, xmm2
	movdqa xmm7, [___x86_globals_array8_ones]
	pcmpgtw xmm5, xmm4
	pand xmm5, xmm7
	pcmpgtw xmm6, xmm3
	pand xmm6, xmm7
	paddw xmm5, xmm6
	paddw xmm5, xmm0
	movdqa [esp + 128], xmm5
	
	;;; (8-475), xmm5[delta] = clip3(xmm6[-tc], tc, ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3)) ;;;
	movdqa xmm7, [esp + 80]
	pxor xmm6, xmm6
	psubw xmm6, xmm5
	movdqa xmm5, [esp + 48]
	psubw xmm7, [esp + 32]
	paddw xmm7, [___x86_globals_array8_fours]
	psubw xmm5, [esp + 96]
	psllw xmm5, 2
	paddw xmm5, xmm7
	psraw xmm5, 3
	pmaxsw xmm5, xmm6
	pminsw xmm5, [esp + 128]
	movdqa [esp + 112], xmm5
	
	;;; (8-476), pf0 = clip3(0, 255, (p0 + xmm5[delta])) ;;;
	pshufd xmm7, xmm5, 0xE4
	mov eax, [ebp + 44]
	paddw xmm7, [esp + 96]
	packuswb xmm7, xmm7
	movlps [eax], xmm7
	
	;;; (8-477), qf0 = clip3(xmm6[0], 255, (q0 - xmm5[delta])) ;;;
	pshufd xmm7, [esp + 48], 0xE4
	mov eax, [ebp + 56]
	psubw xmm7, xmm5
	packuswb xmm7, xmm7
	movlps [eax], xmm7
	
	;;; (8-478), if (ap < beta) pf1 = p1 + clip3(-tc0, xmm0[tc0], (p2[0] + ((p0[0] + q0[0] + 1) >> 1) - (p1[0] << 1)) >> 1); else pf1 = p1 ;;;
	movdqa xmm5, [esp + 80]
	movdqa xmm7, [esp + 96]
	mov eax, [ebp + 48]
	pxor xmm6, xmm6
	psubw xmm6, xmm0
	psllw xmm5, 1
	pavgw xmm7, [esp + 48]
	psubw xmm7, xmm5
	movdqa xmm5, xmm2
	paddw xmm7, [esp + 64]
	psraw xmm7, 1
	pmaxsw xmm7, xmm6
	pminsw xmm7, xmm0
	pcmpgtw xmm5, xmm3
	pand xmm5, xmm7
	paddw xmm5, [esp + 80]
	packuswb xmm5, xmm5
	movlps [eax], xmm5
	
	;;; (8-480), if (aq < beta) qf1 = q1 + clip3(xmm6[-tc0], xmm0[tc0], (q2[0] + ((p0[0] + q0[0] + 1) >> 1) - (q1[0] << 1)) >> 1); else qf1 = q1 ;;;
	movdqa xmm5, [esp + 32]
	movdqa xmm7, [esp + 96]
	mov eax, [ebp + 60]
	psllw xmm5, 1
	pavgw xmm7, [esp + 48]
	psubw xmm7, xmm5
	movdqa xmm5, xmm2
	paddw xmm7, [esp + 16]
	psraw xmm7, 1
	pmaxsw xmm7, xmm6
	pminsw xmm7, xmm0
	pcmpgtw xmm5, xmm4
	pand xmm5, xmm7
	paddw xmm5, [esp + 32]
	packuswb xmm5, xmm5
	movlps [eax], xmm5
	
	;;; (8-482), pf2 = p2 ;;;
	;;; (8-483), qf2 = q2 ;;;
	mov eax, [ebp + 16]
	movlps xmm5, [eax]
	mov eax, [ebp + 28]
	movlps xmm6, [eax]
	mov eax, [ebp + 52]
	movlps [eax], xmm5	
	mov eax, [ebp + 64]
	movlps [eax], xmm6
	
	mov esp, ebp
	pop ebp
	ret
	
;;;
;;;static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_asm_ssse3(
;;;    const uint8_t *p0[ebp + 8], const uint8_t *p1[ebp + 12], const uint8_t *p2[ebp + 16],
;;;    const uint8_t *q0[ebp + 20], const uint8_t *q1[ebp + 24], const uint8_t *q2[ebp + 28],
;;;    int16_t bS[4][ebp + 32],
;;;    int16_t indexA[ebp + 36],
;;;    HL_OUT uint8_t *pf0[ebp + 40], HL_OUT uint8_t *pf1[ebp + 44], HL_OUT uint8_t *pf2[ebp + 48],
;;;    HL_OUT uint8_t *qf0[ebp + 52], HL_OUT uint8_t *qf1[ebp + 56], HL_OUT uint8_t *qf2[ebp + 60]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_asm_ssse3:
	push ebp
    mov ebp, esp
    
    sub esp, 8
    ;;; xmm0[xmm_tc0] = _mm_set_epi16(T[IA][bS[3]], T[IA][bS[3]], T[IA][bS[2]], T[IA][bS[2]], T[IA][bS[1]], T[IA][bS[1]], T[IA][bS[0]], T[IA][bS[0]])) ;;;
    xor edx, edx
	mov dx, [ebp + 36]
	imul edx, 20			
	lea edx, [_HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE + edx]
	mov ecx, [ebp + 32]
	; bS[0]
	mov eax, [ecx]
	and eax, 0xffff
	mov eax, [edx + eax*4]
	mov [esp], eax
	; bS[1]
	mov eax, [ecx]
	shr eax, 16
	mov eax, [edx + eax*4]
	shl eax, 16
	or [esp], eax
	; bS[2]
	mov eax, [ecx + 4]
	and eax, 0xffff
	mov eax, [edx + eax*4]
	mov [esp + 4], eax
	; bS[3]
	mov eax, [ecx + 4]
	shr eax, 16
	mov eax, [edx + eax*4]
	shl eax, 16
	or [esp + 4], eax
	movlps xmm0, [esp]
	pshufb xmm0, [___x86_globals_array4_shuffle_mask_0_0_1_1_2_2_3_3]
	add esp, 8
	
	;;; (8-473), xmm1[tc] = xmm0[tc0] + 1 ;;;
	pshufd xmm1, [___x86_globals_array8_ones], 0xE4
	paddw xmm1, xmm0
	
	
	;;; (8-475), "xmm7"[delta] = clip3(-xmm1[tc][0], xmm1[tc][0], ((((q0[0] - p0[0]) << 2) + (p1[0] - q1[0]) + 4) >> 3)) ;;;
	; "xmm2"[p0], "xmm7"[q0], xmm4[p1], xmm5[q1]
	mov eax, [ebp + 8]
	movlps xmm2, [eax]
	mov eax, [ebp + 20]
	movlps xmm3, [eax]
	mov eax, [ebp + 12]
	movlps xmm4, [eax]
	mov eax, [ebp + 24]
	movlps xmm5, [eax]
	pxor xmm6, xmm6
	punpcklbw xmm2, xmm6	
	punpcklbw xmm3, xmm6
	punpcklbw xmm4, xmm6	
	punpcklbw xmm5, xmm6
	movdqa xmm7, xmm3
	psubw xmm6, xmm1
	psubw xmm4, xmm5
	paddw xmm4, [___x86_globals_array8_fours]
	psubw xmm7, xmm2
	psllw xmm7, 2
	paddw xmm7, xmm4
	psraw xmm7, 3
	pmaxsw xmm7, xmm6
	pminsw xmm7, xmm1
	
	;;; (8-476), pf0 = clip3(0, 255, (xmm2[p0] + xmm7[delta])) ;;;
	mov eax, [ebp + 40]
	paddw xmm2, xmm7
	packuswb xmm2, xmm2
	movlps [eax], xmm2
	
	;;; (8-477), qf0 = clip3(0, 255, (xmm3[q0] - delta)) ;;;
	mov eax, [ebp + 52]
	psubw xmm3, xmm7
	packuswb xmm3, xmm3
	movlps [eax], xmm3
	
	;;; (8-479), pf1 = p1 ;;;
	;;; (8-481), qf1 = q1 ;;;
	;;; (8-482), pf2 = p2 ;;;
	;;; (8-483), qf2 = q2 ;;;
	mov eax, [ebp + 12]
	movlps xmm0, [eax]
	mov eax, [ebp + 24]
	movlps xmm1, [eax]
	mov eax, [ebp + 16]
	movlps xmm2, [eax]
	mov eax, [ebp + 28]
	movlps xmm3, [eax]
	mov eax, [ebp + 44]
	movlps [eax], xmm0
	mov eax, [ebp + 56]
	movlps [eax], xmm1
	mov eax, [ebp + 48]
	movlps [eax], xmm2
	mov eax, [ebp + 60]
	movlps [eax], xmm3

	mov esp, ebp
	pop ebp
	ret
	

;;;
;;;static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_asm_ssse3(
;;;    const uint8_t *p0[ebp + 8], const uint8_t *p1[ebp + 12], const uint8_t *p2[ebp + 16], const uint8_t *p3[ebp + 20],
;;;    const uint8_t *q0[ebp + 24], const uint8_t *q1[ebp + 28], const uint8_t *q2[ebp + 32], const uint8_t *q3[ebp + 36],
;;;    int16_t alpha[ebp + 40], int16_t beta[ebp + 44],
;;;    HL_OUT uint8_t *pf0[ebp + 48], HL_OUT uint8_t *pf1[ebp + 52], HL_OUT uint8_t *pf2[ebp + 56],
;;;    HL_OUT uint8_t *qf0[ebp + 60], HL_OUT uint8_t *qf1[ebp + 64], HL_OUT uint8_t *qf2[ebp + 68]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_asm_ssse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 8x16 = 128 = xmm_p0(@112), xmm_p1(@96), xmm_p2(@80), xmm_p3(@64), xmm_q0(@48), xmm_q1(@32), xmm_q2(@16), xmm_q3(@0)
    sub esp, 128
    
    movdqa xmm7, [___x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0]
    
    ;;; xmm0[alpha] = _mm_set1_epi16(alpha);
    movss xmm0, [ebp + 40]
	pshufb xmm0, xmm7
	
	;;; xmm1[beta] = _mm_set1_epi16(beta);
    movss xmm1, [ebp + 44]
	pshufb xmm1, xmm7
	
	pxor xmm2, xmm2
	
	;;; _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0])) ;;;
	mov eax, [ebp + 24]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm2
	movdqa [esp + 48], xmm4
	
    ;;; _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0])) ;;;
    mov eax, [ebp + 28]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm2
	movdqa [esp + 32], xmm4
    
    ;;; _mm_store_si128((__m128i*)&xmm_q2, _mm_set_epi16(q2[7], q2[6], q2[5], q2[4], q2[3], q2[2], q2[1], q2[0])) ;;;
    mov eax, [ebp + 32]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm2
	movdqa [esp + 16], xmm4
    
    ;;; _mm_store_si128((__m128i*)&xmm_q3, _mm_set_epi16(q3[7], q3[6], q3[5], q3[4], q3[3], q3[2], q3[1], q3[0])) ;;;
    mov eax, [ebp + 36]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm2
	movdqa [esp + 0], xmm4

    ;;; _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0])) ;;;
    mov eax, [ebp + 8]
	movlps xmm5, [eax]
	punpcklbw xmm5, xmm2
	movdqa [esp + 112], xmm5
    
    ;;; _mm_store_si128((__m128i*)&xmm_p2, _mm_set_epi16(p2[7], p2[6], p2[5], p2[4], p2[3], p2[2], p2[1], p2[0])) ;;;
    mov eax, [ebp + 16]
	movlps xmm6, [eax]
	punpcklbw xmm6, xmm2
	movdqa [esp + 80], xmm6
    
    ;;; _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0])) ;;;
    mov eax, [ebp + 12]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm2
	movdqa [esp + 96], xmm4
    
    ;;; _mm_store_si128((__m128i*)&xmm_p3, _mm_set_epi16(p3[7], p3[6], p3[5], p3[4], p3[3], p3[2], p3[1], p3[0])) ;;;
    mov eax, [ebp + 20]
	movlps xmm4, [eax]
	punpcklbw xmm4, xmm2
	movdqa [esp + 64], xmm4
	
	;;; /*** PF ***/ ;;;
	
	;;; (8-471), xmm4[ap] = |xmm6[p2] - xmm5[p0]| ;;;
	psubw xmm6, xmm5
	pabsw xmm4, xmm6
	
	;;; xmm7[cmp_if] = (xmm4[ap] < xmm1[beta] && |p0[0] - q0[0]| < ((xmm0[alpha] >> 2) + 2)) ;;;
	movdqa xmm5, xmm0
	movdqa xmm7, xmm1
	movdqa xmm6, [esp + 112]
	psraw xmm5, 2
	paddw xmm5, [___x86_globals_array8_twos]
	psubw xmm6, [esp + 48]
	pabsw xmm6, xmm6
	pcmpgtw xmm5, xmm6
	pcmpgtw xmm7, xmm4
	pand xmm7, xmm5
	;;; xmm6[cmp_else] = !xmm7[cmp_if] ;;;
	pshufd xmm6, xmm7, 0xE4
	pcmpeqw xmm3, xmm3 ; xmm3 = 0xFFFFF...
	pandn xmm6, xmm3
    ;;; (8-485), cmp_if() -> xmm5[pf0_if] = (p2 + (p1 << 1) + (p0 << 1) + (q0 << 1) + q1 + 4) >> 3 ;;;
    movdqa xmm3, [esp + 48]
    movdqa xmm5, [esp + 96]
    movdqa xmm4, [esp + 112]
    psllw xmm3, 1
    psllw xmm5, 1
    psllw xmm4, 1
    paddw xmm5, xmm4
    paddw xmm5, xmm3
    paddw xmm5, [esp + 80]
    paddw xmm5, [esp + 32]
    paddw xmm5, [___x86_globals_array8_fours]
    psraw xmm5, 3
    ;;; (8-488), cmp_else() -> xmm4[pf0_else] = ((p1 << 1) + p0 + q1 + 2) >> 2 ;;;
    ;;; xmm3 = 2
    movdqa xmm4, [esp + 96]
    movdqa xmm3, [___x86_globals_array8_twos]
    psllw xmm4, 1
    paddw xmm4, [esp + 112]
    paddw xmm4, [esp + 32]
    paddw xmm4, xmm3
    psraw xmm4, 2
    ;;; (8-485) + (8-488), pf0 = ((xmm5[pf0_if] & xmm7[cmp_if]) + (xmm4[pf0_else] & xmm6[cmp_else])) ;;;
    pand xmm4, xmm6
    pand xmm5, xmm7
    paddw xmm5, xmm4
    packuswb xmm5, xmm5
    mov eax, [ebp + 48]
    movlps [eax], xmm5
	;;; (8-486), cmp_if() -> xmm5[pf1_if] = (p2 + p1 + p0 + q0 + 2) >> 2 ;;;
	movdqa xmm5, [esp + 80]
	paddw xmm5, [esp + 96]
	paddw xmm5, [esp + 112]
	paddw xmm5, [esp + 48]
	paddw xmm5, xmm3
	psraw xmm5, 2
	;;; (8-489), cmp_else() -> xmm4[pf1_else] = p1
	movdqa xmm4, [esp + 96]
	;;; (8-486) + (8-489), pf1 = ((xmm5[pf1_if] & xmm7[cmp_if]) + (xmm4[pf1_else] & xmm6[cmp_else])) ;;;
	pand xmm4, xmm6
    pand xmm5, xmm7
    paddw xmm5, xmm4
    packuswb xmm5, xmm5
    mov eax, [ebp + 52]
    movlps [eax], xmm5
    ;;; (8-487), cmp_if() -> xmm5[pf2_if] = ((p3 << 1) + 3*p2 + p1 + p0 + q0 + 4) >> 3 ;;;
    ;;; (8-490), cmp_else() -> xmm4[pf2_else] = p2 ;;;
    movdqa xmm3, [esp + 64]
    movdqa xmm5, [esp + 80]
    movdqa xmm4, [esp + 80]
    psllw xmm3, 1
    pmullw xmm5, [___x86_globals_array8_threes]
    paddw xmm5, xmm3
    paddw xmm5, [esp + 96]
    paddw xmm5, [esp + 112]
    paddw xmm5, [esp + 48]
    paddw xmm5, [___x86_globals_array8_fours]
    psraw xmm5, 3    
    ;;; (8-487) + (8-490), pf2 = ((xmm5[pf2_if] & xmm7[cmp_if]) + (xmm4[pf2_else] & xmm6[cmp_else])) ;;;
    pand xmm4, xmm6
    pand xmm5, xmm7
    paddw xmm5, xmm4
    packuswb xmm5, xmm5
    mov eax, [ebp + 56]
    movlps [eax], xmm5
    
    ;;; /*** QF ***/ ;;;
        
    ;;; (8-472), xmm4[aq] = |q2 - q0| ;;;
    pshufd xmm6, [esp + 16], 0xE4
	psubw xmm6, [esp + 48]
	pabsw xmm4, xmm6
	
	;;; xmm7[cmp_if] = (xmm4[aq] < xmm1[beta] && |p0[0] - q0[0]| < ((xmm0[alpha] >> 2) + 2)) ;;;
	movdqa xmm5, xmm0
	movdqa xmm3, [___x86_globals_array8_twos]
	movdqa xmm7, xmm1
	movdqa xmm6, [esp + 112]
	psraw xmm5, 2
	paddw xmm5, xmm3
	psubw xmm6, [esp + 48]
	pabsw xmm6, xmm6
	pcmpgtw xmm5, xmm6
	pcmpgtw xmm7, xmm4
	pand xmm7, xmm5
	;;; xmm6[cmp_else] = !xmm7[cmp_if] ;;;
	pshufd xmm6, xmm7, 0xE4
	pcmpeqw xmm3, xmm3 ; xmm3 = 0xFFFFF...
	pandn xmm6, xmm3
	 ;;; (8-492), cmp_if() -> xmm5[qf0_if] = (p1 + (p0 << 1) + (q0 << 1) + (q1 << 1) + q2 + 4) >> 3 ;;;
	movdqa xmm3, [esp + 48]
	movdqa xmm5, [esp + 32]
	movdqa xmm4, [esp + 112]
    psllw xmm3, 1
    psllw xmm4, 1
    psllw xmm5, 1
    paddw xmm5, xmm4
    movdqa xmm4, [___x86_globals_array8_fours]
    paddw xmm5, xmm3
    paddw xmm5, [esp + 96]
    paddw xmm5, [esp + 16]
    paddw xmm5, xmm4
    psraw xmm5, 3
    ;;; (8-495), cmp_else() -> xmm4[qf0_else] = ((q1 << 1) + q0 + p1 + 2) >> 2 ;;;
    ; xmm3 = 2
    movdqa xmm4, [esp + 32]
    movdqa xmm3, [___x86_globals_array8_twos]
    psllw xmm4, 1
    paddw xmm4, [esp + 48]
    paddw xmm4, [esp + 96]
    paddw xmm4, xmm3
    psraw xmm4, 2
    ;;; (8-492) + (8-495), qf0 = ((xmm5[qf0_if] & xmm7[cmp_if]) + (xmm4[qf0_else] & xmm6[cmp_else])) ;;;
    pand xmm4, xmm6
    pand xmm5, xmm7
    paddw xmm5, xmm4
    packuswb xmm5, xmm5
    mov eax, [ebp + 60]
    movlps [eax], xmm5
    ;;; (8-493), cmp_if() -> xmm5[qf1_if] = (p0 + q0 + q1 + q2 + 2) >> 2 ;;;
    ;;; (8-496), cmp_else() -> xmm4[qf1_else] = q1 ;;;
	movdqa xmm5, [esp + 16]
	movdqa xmm4, [esp + 32]
	paddw xmm5, [esp + 32]
	paddw xmm5, [esp + 48]
	paddw xmm5, [esp + 112]
	paddw xmm5, xmm3
	psraw xmm5, 2	
	;;; (8-493) + (8-496), qf1 = ((xmm5[qf1_if] & xmm7[cmp_if]) + (xmm4[qf1_else] & xmm6[cmp_else])) ;;;
	pand xmm4, xmm6
    pand xmm5, xmm7
    paddw xmm5, xmm4
    packuswb xmm5, xmm5
    mov eax, [ebp + 64]
    movlps [eax], xmm5
    ;;; (8-494), cmp_if() -> xmm5[qf2_if] = ((q3 << 1) + 3*q2 + q1 + q0 + p0 + 4) >> 3;
    ;;; (8-497), cmp_else() -> xmm4[qf2_else] = q2 ;;;
    movdqa xmm3, [esp + 0]
    movdqa xmm5, [esp + 16]
    movdqa xmm4, [esp + 16]
    psllw xmm3, 1
    pmullw xmm5, [___x86_globals_array8_threes]
    paddw xmm5, xmm3
    paddw xmm5, [esp + 32]
    paddw xmm5, [esp + 48]
    paddw xmm5, [esp + 112]
    paddw xmm5, [___x86_globals_array8_fours]
    psraw xmm5, 3   
    ;;; (8-494) + (8-497), qf2 = ((xmm5[qf2_if] & xmm7[cmp_if]) + (xmm4[qf2_else] & xmm6[cmp_else])) ;;;
    pand xmm4, xmm6
    pand xmm5, xmm7
    paddw xmm5, xmm4
    packuswb xmm5, xmm5
    mov eax, [ebp + 68]
    movlps [eax], xmm5

	mov esp, ebp
	pop ebp
	ret
	
;;;	
;;;static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_asm_sse2(
;;;    const uint8_t *p0[esp + 4], const uint8_t *p1[esp + 8], const uint8_t *p2[esp + 12], const uint8_t *p3[esp + 16],
;;;    const uint8_t *q0[esp + 20], const uint8_t *q1[esp + 24], const uint8_t *q2[esp + 28], const uint8_t *q3[esp + 32],
;;;    HL_OUT uint8_t *pf0[esp + 36], HL_OUT uint8_t *pf1[esp + 40], HL_OUT uint8_t *pf2[esp + 44],
;;;    HL_OUT uint8_t *qf0[esp + 48], HL_OUT uint8_t *qf1[esp + 52], HL_OUT uint8_t *qf2[esp + 56]
;;;)
_hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_asm_sse2:
	pxor xmm5, xmm5
	movdqa xmm6, [___x86_globals_array8_twos]
		
	;;; _mm_store_si128((__m128i*)&xmm0[xmm_q0], _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0])) ;;;
	mov eax, [esp + 20]
	movlps xmm0, [eax]
	punpcklbw xmm0, xmm5
	
	;;;  _mm_store_si128((__m128i*)&xmm1[xmm_q1], _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0])) ;;;
	mov eax, [esp + 24]
	movlps xmm1, [eax]
	punpcklbw xmm1, xmm5
	
	;;; _mm_store_si128((__m128i*)&xmm2[xmm_p0], _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0])) ;;;
	mov eax, [esp + 4]
	movlps xmm2, [eax]
	punpcklbw xmm2, xmm5
	
	;;; _mm_store_si128((__m128i*)&xmm3[xmm_p1], _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0])) ;;;
	mov eax, [esp + 8]
	movlps xmm3, [eax]
	punpcklbw xmm3, xmm5
	
	;;; (8-488), pf0 = ((p1 << 1) + p0 + q1 + 2) >> 2 ;;;
	movdqa xmm4, xmm3
	paddw xmm2, xmm1
	paddw xmm2, xmm6
	psllw xmm4, 1
	paddw xmm2, xmm4
	psraw xmm2, 2
	packuswb xmm2, xmm2
	mov eax, [esp + 36]
	movlps [eax], xmm2
	
	;;; (8-489), pf1 = p1 ;;;
	;;; (8-490), pf2 = p2 ;;;
	mov eax, [esp + 8]
	movlps xmm2, [eax]
	mov eax, [esp + 12]
	movlps xmm4, [eax]
	mov eax, [esp + 40]
	movlps [eax], xmm2	
	mov eax, [esp + 44]
	movlps [eax], xmm4
	
	;;; (8-495), qf0 = ((q1 << 1) + q0 + p1 + 2) >> 2 ;;;
	movdqa xmm4, xmm1
	paddw xmm3, xmm0
	paddw xmm3, xmm6
	psllw xmm4, 1
	paddw xmm3, xmm4
	psraw xmm3, 2
	packuswb xmm3, xmm3
	mov eax, [esp + 48]
	movlps [eax], xmm3
	
	;;; (8-496), qf1 = q1 ;;;
	;;; (8-497), qf2 = q2 ;;;
	packuswb xmm1, xmm1
	mov eax, [esp + 28]
	movlps xmm4, [eax]
	mov eax, [esp + 52]
	movlps [eax], xmm1	
	mov eax, [esp + 56]
	movlps [eax], xmm4
		
	ret