; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

%include "hl_math_x86_macros_sse.asm"
%include "hl_globals_x86_sse.asm"

bits 32

global _hl_codec_x86_264_interpol_load_samples_asm_sse2
global _hl_codec_x86_264_tap6filter_vert4_asm_sse41
global _hl_codec_x86_264_tap6filter_horiz4_asm_sse41
global _hl_codec_x86_264_interpol_luma00_horiz4_asm_sse41
global _hl_codec_x86_264_interpol_luma01_vert4_asm_sse41
global _hl_codec_x86_264_interpol_luma02_vert4_asm_sse41
global _hl_codec_x86_264_interpol_luma03_vert4_asm_sse41
global _hl_codec_x86_264_interpol_luma10_horiz4_asm_sse41
global _hl_codec_x86_264_interpol_luma11_diag4_asm_sse41
global _hl_codec_x86_264_interpol_luma12_vert4_asm_sse41
global _hl_codec_x86_264_interpol_luma13_diag4_asm_sse41
global _hl_codec_x86_264_interpol_luma20_horiz4_asm_sse41
global _hl_codec_x86_264_interpol_luma21_diag4_asm_sse41
global _hl_codec_x86_264_interpol_luma22_vert4_asm_sse41
global _hl_codec_x86_264_interpol_luma23_diag4_asm_sse41
global _hl_codec_x86_264_interpol_luma30_horiz4_asm_sse41
global _hl_codec_x86_264_interpol_luma31_diag4_asm_sse41
global _hl_codec_x86_264_interpol_luma32_vert4_asm_sse41
global _hl_codec_x86_264_interpol_luma33_diag4_asm_sse41

extern _hl_math_tap6filter4x1_u32_asm_sse41
extern _hl_math_clip2_4x1_asm_sse41

section .data
	
section .text

;;; HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2(pc_indices, cSL, ret, loadlabel)
;;; "pc_indices", "cSL" and "ret" MUST NOT BE "ebx"
;; Example: HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 eax, ecx, edx, load0
%macro HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 4
%4:
		push ebx
		
		mov ebx, [%1]
		and ebx, 15
		mov ebx, [%1]; restore
		jnz .load_unaligned
		movdqa xmm0, [%2 + ebx*4]; load aligned
		jmp .shufle

.load_unaligned:
		movdqu xmm0, [%2 + ebx*4]; load unaligned
		
.shufle:
		mov ebx, [%1 + 12]
		sub ebx, [%1]
		cmp ebx, 3
		je .bye ; most frequent use case -> nothing else to to
		cmp ebx, 0
		je .shufle_0
		cmp ebx, 1
		je .shufle_1
		jmp .shufle_2

.shufle_0:
		pshufb xmm0, [___x86_globals_array4_shuffle_mask_0_0_0_0]
		jmp .bye

.shufle_1:
		mov ebx, [%1 + 4]
		sub ebx, [%1]
		cmp ebx, 0
		je .shufle_1_0
		pshufb xmm0, [___x86_globals_array4_shuffle_mask_0_1_1_1]; diff == 1
		jmp .bye
.shufle_1_0:
		pshufb xmm0, [___x86_globals_array4_shuffle_mask_0_0_0_1]; diff == 0
		jmp .bye

.shufle_2:
		mov ebx, [%1 + 4]
		sub ebx, [%1]
		cmp ebx, 0
		je .shufle_2_0
		pshufb xmm0, [___x86_globals_array4_shuffle_mask_0_1_2_2]; diff == 1
		jmp .bye
.shufle_2_0:
		pshufb xmm0, [___x86_globals_array4_shuffle_mask_0_0_1_2]; diff == 0

.bye:
		movdqa [%3], xmm0
		pop ebx
%endmacro

;;;
;;; void hl_codec_x86_264_interpol_load_samples_continuous_asm_sse2(
;;;		const uint32_t* pc_indices, // ebp + 8
;;;		const hl_pixel_t* cSL, // ebp + 12
;;;		__m128i* ret // ebp + 16
;;;		)
_hl_codec_x86_264_interpol_load_samples_continuous_asm_sse2:
	push ebp
    mov ebp, esp
    
    mov         eax, [ebp + 8] 
	mov         ecx, [ebp + 8]
	mov         edx, [eax+ 12]
	sub         edx, [ecx]
	cmp         edx, 3 
	jne         .slow_load 
	
	.fast_load
	mov         eax,  [ebp + 8] 
	mov         ecx,  [eax] 
	mov         edx,  [ebp + 12] 
	movdqu      xmm0, [edx+ecx*4] 
	mov         eax,  [ebp + 16] 
	movdqa      [eax], xmm0
	jmp			.end
	
	.slow_load
	push esi
	push edi
	mov         eax, [ebp + 8] 
	mov         ecx, [eax] 
	mov         edx, [ebp + 12] 
	mov         eax, [edx+ecx*4] 
	mov         ecx, [ebp + 8] 
	mov         edx, [ecx + 4] 
	mov         ecx, [ebp + 12] 
	mov         edx, [ecx+edx*4] 
	mov         ecx, [ebp + 8] 
	mov         ecx, [ecx+8] 
	mov         esi, [ebp + 12] 
	mov         ecx, [esi+ecx*4] 
	mov         esi, [ebp + 8] 
	mov         esi, [esi + 12] 
	mov         edi, [ebp + 12] 
	mov         esi, [edi+esi*4] 
	movd        xmm0,esi 
	movd        xmm1,ecx 
	movd        xmm2,edx 
	movd        xmm3,eax 
	punpckldq   xmm3,xmm1 
	punpckldq   xmm2,xmm0 
	punpckldq   xmm3,xmm2 
	mov         edx, [ebp + 16] 
	movdqa      [edx],xmm3
	pop edi
	pop esi
	
	.end
	pop ebp
	ret

;;;
;;; Load samples at index: pc_indices[0], pc_indices[gap], pc_indices[gap * 2] and pc_indices[gap * 3] into "ret"
;;; void hl_codec_x86_264_interpol_load_samples_asm_sse2(
;;;		const uint32_t* pc_indices, // ebp + 8
;;;		int32_t i_gap, // 1 = continuous // ebp + 12
;;;		const hl_pixel_t* cSL, // ebp + 16
;;;		__m128i* ret // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_load_samples_asm_sse2:
	push ebp
    mov ebp, esp
    push esi
    push edi
    push ebx
    
    mov esi, [ebp + 8]; &pc_indices
    mov ecx, [ebp + 12]; &i_gap
    mov edi, [ebp + 16]; &cSL
    mov ebx, [ebp + 20]; &ret
    
    cmp ecx, 1
    jnz .load_slow
    
	mov         edx, [esi + 12]
	sub         edx, [esi]
	cmp         edx, 3 
	jne         .load_slow 
    
    .load_fast
	mov         ecx,  [esi] 
	movdqu      xmm0, [edi+ecx*4]
	movdqa      [ebx], xmm0
	jmp			.end
    
    .load_slow
    shl dword ecx, 2; make i_gap in unit of sizeof("int32_t")
	
    mov eax, [esi]
    shl dword eax, 2
    add edi, eax
    mov edx, [edi]
    mov [ebx], edx
    sub edi, eax
    
    add ebx, 4
    add esi, ecx
    
    mov eax, [esi]
    shl dword eax, 2
    add edi, eax
    mov edx, [edi]
    mov [ebx], edx
    sub edi, eax
    
    add ebx, 4
    add esi, ecx
    
    mov eax, [esi]
    shl dword eax, 2
    add edi, eax
    mov edx, [edi]
    mov [ebx], edx
    sub edi, eax
    
    add ebx, 4
    add esi, ecx
    
    mov eax, [esi]
    shl dword eax, 2
    add edi, eax
    mov edx, [edi]
    mov [ebx], edx
    
    .end
    pop ebx
    pop edi
    pop esi
    pop ebp
	ret
	
	
;;;
;;; void hl_codec_x86_264_tap6filter_vert4_asm_sse41(
;;;		const uint32_t* pc_indices, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL,  // ebp + 16
;;;		__m128i* ret  // ebp + 20
_hl_codec_x86_264_tap6filter_vert4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
    mov esi, [ebp + 8]; &pc_indices
    mov edi, [ebp + 12]; &i_indices_stride
    mov ecx, [ebp + 16]; &cSL
    
    shl dword edi, 2; make i_indices_stride in unit of sizeof("int32_t")
	
	and esp, -16; align on 16 bytes
	
	; Local variables: 16 + 16 + 16 + 16 + 16 + 16 = 96 - > E(@108), F(@92), G(@76), H(@60), I(@44), J(@28)
	; Parameter addresses: 4+4+4+4+4+4+4 = 28
	sub esp, 124
	
	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &E);;;
	lea eax, [esp + 108]; @E
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, ecx, eax, load0
	
	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &F);;;
	add esi, edi; move pc_indices by i_indices_stride
	lea eax, [esp + 92]; @F
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, ecx, eax, load1

	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &G);;;
	add esi, edi; move pc_indices by i_indices_stride
	lea eax, [esp + 76]; @G
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, ecx, eax, load2

	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &H);;;
	add esi, edi; move pc_indices by i_indices_stride
	lea eax, [esp + 60]; @H
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, ecx, eax, load3

	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &I);;;
	add esi, edi; move pc_indices by i_indices_stride
	lea eax, [esp + 44]; @I
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, ecx, eax, load4

	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &J);;;
	add esi, edi; move pc_indices by i_indices_stride
	lea eax, [esp + 28]; @J
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, ecx, eax, load5
	
	;;;void hl_math_tap6filter4x1_u32_asm_sse41(const hl_int128_t* e,const hl_int128_t* f,const hl_int128_t* g,const hl_int128_t* h,const hl_int128_t* i,const hl_int128_t* j,hl_int128_t* ret);;;
	mov eax, [ebp + 20]; @ret
	HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41 esp+108, esp+92, esp+76, esp+60, esp+44, esp+28, eax
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
    pop ebp
	ret
	
;;;
;;; void hl_codec_x86_264_tap6filter_horiz4_asm_sse41(
;;;		const uint32_t* pc_indices, // ebp + 8
;;;		const hl_pixel_t* cSL, // ebp + 12
;;;		__m128i* ret // ebp + 16
;;;		)
_hl_codec_x86_264_tap6filter_horiz4_asm_sse41
	push ebp
    mov ebp, esp	
	push esi
	push edi
	
    mov esi, [ebp + 8]; &pc_indices
    mov edi, [ebp + 12]; &cSL
    
    and esp, -16; align on 16 bytes
	
	; Local variables: 16 + 16 + 16 + 16 + 16 + 16 = 96 -> E(@108), F(@92), G(@76), H(@60), I(@44), J(@28)
	; Parameter addresses: (4 + 4 + 4 + 4 + 4 + 4 + 4) = 28
	sub esp, 124
	
	;;; hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &E);;;
	lea eax, [esp + 108]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load7
	
	add esi, 4
	
	;;; hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &F);;;
	lea eax, [esp + 92]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load8
	
	add esi, 4
	
	;;; hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &G);;;
	lea eax, [esp + 76]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load9
	
	add esi, 4
	
	;;; hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &H);;;
	lea eax, [esp + 60]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load10
	
	add esi, 4
	
	;;; hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &I);;;
	lea eax, [esp + 44]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load11
	
	add esi, 4
	
	;;; hl_codec_x86_264_interpol_load_samples_asm_sse2(pc_indices, 1, cSL, &J);;;
	lea eax, [esp + 28]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load12
	
	;;;hl_math_tap6filter4x1_u32_asm_sse41(&E, &F, &G, &H, &I, &J, ret);;;	
	mov eax, [ebp + 16]; @ret
	HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41 esp+108, esp+92, esp+76, esp+60, esp+44, esp+28, eax
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
    pop ebp
	ret
	
;;;
;;;void hl_codec_x86_264_interpol_luma00_horiz4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz,
;;;		const hl_pixel_t* cSL,
;;;		HL_ALIGNED(16) int32_t* predPartLXL
;;;		)
_hl_codec_x86_264_interpol_luma00_horiz4_asm_sse41:    
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 eax, ecx, edx, load20
	ret
	
;;;
;;; void hl_codec_x86_264_interpol_luma01_vert4_asm_sse41(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL, // ebp + 16
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // // ebp + 24
_hl_codec_x86_264_interpol_luma01_vert4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
    mov esi, [ebp + 8]; &pc_indices_vert
    mov edi, [ebp + 12]; &i_indices_stride
    mov ecx, [ebp + 16]; &cSL

	shl edi, 2; make i_indices_stride in unit of sizeof("int32_t")

	and esp, -16; align on 16 bytes
	
	; Local variables: 16 + 16 + 16 = 48 - > G(@48), h1(@32), h(@16)
	; Parameter addresses: 4+4+4+4 = 16
	sub esp, 64	

	;;; hl_codec_x86_264_interpol_load_samples_asm_sse2(&pc_indices_vert[(i_indices_stride << 1)], 1, cSL, &G);;;
	mov edx, esi
	add edx, edi
	add edx, edi
	lea eax, [esp + 48]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 edx, ecx, eax, load16
	
	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);;;
	mov eax, [ebp + 8]
	mov [esp], eax
	mov eax, [ebp + 12]
	mov [esp + 4], eax
	mov eax, [ebp + 16]
	mov [esp + 8], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;; h1 = ((h1 + 16)>>5);;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 32], xmm0

	;;; h = Clip1Y(h1, BitDepthY);;;
	; Clip1Y(h1, BitDepthY) = HL_MATH_CLIP2_4X1_ASM_SSE41(&MaxPixelValueY4, &h1, &h)
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+32, esp+16

	;;;h += G;;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [esp + 48]
	;;;h += 1;;;
	paddd xmm0, [___x86_globals_array4_ones]
	;;;h >>= 1;;;
	psrad xmm0, 1
	;;;predPartLXL=h;;;
	mov eax, [ebp + 20]
	movdqa [eax], xmm0

	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
    pop ebp
	ret
	
;;;
;;; void _hl_codec_x86_264_interpol_luma02_vert4_asm_sse41(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL, // ebp + 16
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma02_vert4_asm_sse41:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
	
	; Local variables: 16 - > h1(@16)
	; Parameter addresses: 4+4+4+4 = 16
	sub esp, 32
	
	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);;;
	mov eax, [ebp + 8]
	mov [esp], eax
	mov eax, [ebp + 12]
	mov [esp + 4], eax
	mov eax, [ebp + 16]
	mov [esp + 8], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;h1 = (h1 + 16);;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_sixteens]
	;;;h1 >> 5;;;
	psrad xmm0, 5
	movdqa [esp + 16], xmm0
	;;;HL_MATH_CLIP2_4X1_ASM_SSE41((hl_int128_t*)MaxPixelValueY, &h1, (hl_int128_t*)predPartLXL);;;
	mov eax, [ebp + 24]
	mov ecx, [ebp + 20]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+16, ecx
	

	; restore stack pointer
	mov esp, ebp
    
    pop ebp
	ret
	
;;;
;;; void hl_codec_x86_264_interpol_luma03_vert4_asm_sse41(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL, // ebp + 16
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma03_vert4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
    mov esi, [ebp + 8]; &pc_indices_vert
    mov edi, [ebp + 16]; &cSL

	and esp, -16; align on 16 bytes
	
	; Local variables: 16 + 16 + 16 = 48 - > h1(@48), h(@32), G(@16)
	; Parameter addresses: 4+4+4+4 = 16
	sub esp, 64

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);;;
	mov [esp], esi
	mov eax, [ebp + 12]
	mov [esp + 4], eax
	mov [esp + 8], edi
	lea eax, [esp + 48]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;h1 = ((h1 + 16)>>5);;;
	movdqa xmm0, [esp + 48]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 48], xmm0

	;;;HL_MATH_CLIP2_4X1_ASM_SSE41((hl_int128_t*)MaxPixelValueY, &h1, &h);;;
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+48, esp+32

	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(&pc_indices_vert[(i_indices_stride * 3)], 1, cSL, &G);;;
	mov eax, [ebp + 12]
	imul eax, 3
	shl eax, 2
	add esi, eax
	lea eax, [esp + 16]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load17

	;;;h+=G;;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [esp + 16]
	;;;h+=1
	paddd xmm0, [___x86_globals_array4_ones]
	;;;h>>=1
	psrad xmm0, 1
	;;;predPartLXL=h;;;
	mov eax, [ebp + 20]
	movdqa [eax], xmm0

	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret
	
;;;
;;; void hl_codec_x86_264_interpol_luma10_horiz4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const hl_pixel_t* cSL, // ebp + 12
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 16
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma10_horiz4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
	mov esi, [ebp + 8]; &pc_indices_horiz
    mov edi, [ebp + 12]; &cSL
    
    and esp, -16; align stack on 16 bytes

	; Local variables: 16 + 16 + 16 = 48 - > G(@48), b1(@32), b(@16)
	; Parameter addresses: 4+4+4+4 = 16
	sub esp, 64; 64 = (48 + 16)

	;;;hl_codec_x86_264_interpol_load_samples_asm_sse2(&pc_indices_horiz[2], 1, cSL, &G);;;
	mov ecx, esi
	add ecx, 8
	lea eax, [esp + 48]
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 ecx, edi, eax, load18

	;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &b1);;;
	mov [esp], esi
	mov [esp + 4], edi
	lea eax, [esp + 32]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41
	
	;;;b1 = ((b1 + 16) >> 5);;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 32], xmm0
	
	;;;HL_MATH_CLIP2_4X1_ASM_SSE41((hl_int128_t*)MaxPixelValueY, &b1, &b);;;
	mov eax, [ebp + 20]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+32, esp+16

	;;;b = (b + 1 + G) >> 1;;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_ones]
	paddd xmm0, [esp + 48]
	psrad xmm0, 1

	;;;predPartLXL=b;;;
	mov eax, [ebp + 16]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret
	
;;;
;;; void hl_codec_x86_264_interpol_luma11_diag4_asm_sse41(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		const uint32_t* pc_indices_horiz, // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL, // ebp + 20
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 24
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4]  // ebp + 28
;;;		)
_hl_codec_x86_264_interpol_luma11_diag4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
	mov esi, [ebp + 16]; &i_indices_stride
    mov edi, [ebp + 20]; &cSL

	and esp, -16; align stack pointer on 16 bytes

	; Local variables: 16 + 16 + 16 + 16 = 64 - > h1(@64), h(@48), b1(@32), b(@16)
	; Parameter addresses: 4+4+4+4 = 16
	sub esp, 80

	;;; hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);
	mov eax, [ebp + 8]
	mov [esp], eax
	mov [esp + 4], esi
	mov [esp + 8], edi
	lea eax, [esp + 64]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 64]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 64], xmm0
	mov eax, [ebp + 28]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+64, esp+48

	;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &b1);;;
	mov eax, [ebp + 12]
	mov [esp], eax
	mov [esp + 4], edi
	lea eax, [esp + 32]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41

	;;;b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 32], xmm0
	mov eax, [ebp + 28]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 4], eax
	lea eax, [esp + 16]
	mov [esp + 8], eax
	call _hl_math_clip2_4x1_asm_sse41	
	;call _demba

	;;;predPartLXL = (b + h + 1) >> 1;;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [esp + 48]
	paddd xmm0, [___x86_globals_array4_ones]
	psrad xmm0, 1
	mov eax, [ebp + 24]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret
	
;;; 
;;; void hl_codec_x86_264_interpol_luma12_vert4_asm_sse41(
;;;		const uint32_t* pc_indices_vert[6], // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL, // ebp + 16
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4]  // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma12_vert4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	push ebx
	
	mov ebx, [ebp + 8]; &pc_indices_vert
    mov esi, [ebp + 12]; &i_indices_stride
    mov edi, [ebp + 16]; &cSL
    
    and esp, -16; align stack pointer on 16 bytes

	; Local variables: 9x16 = 144 - > h1(@152), h(@136), cc(@120), dd(@104), m1(@88), ee(@72), ff(@56), j1(@40), j(@24)
	; Parameter addresses: 6x4 = 24
	sub esp, 168

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);;;
	mov eax, [ebx]
	mov [esp], eax
	mov [esp + 4], esi
	mov [esp + 8], edi
	lea eax, [esp + 152]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 152]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 136], xmm0
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+136, esp+136
	
	mov [esp + 4], esi
	mov [esp + 8], edi

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 120]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 104]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 88]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
	
	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 72]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 56]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
	
	;;;j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);;;
	lea eax, [esp + 120]
	mov [esp], eax
	lea eax, [esp + 104]
	mov [esp + 4], eax
	lea eax, [esp + 152]
	mov [esp + 8], eax
	lea eax, [esp + 88]
	mov [esp + 12], eax
	lea eax, [esp + 72]
	mov [esp + 16], eax
	lea eax, [esp + 56]
	mov [esp + 20], eax
	lea eax, [esp + 40]
	mov [esp + 24], eax
	call _hl_math_tap6filter4x1_u32_asm_sse41

	;;;j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);;;
	movdqa xmm0, [esp + 40]
	paddd xmm0, [___x86_globals_array4_five_hundred_and_twelves]
	psrad xmm0, 10
	movdqa [esp + 24], xmm0	
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+24, esp+24

	;;;predPartLXL[i] = (h + j + 1) >> 1;;;
	movdqa xmm0, [esp + 136]
	paddd xmm0, [esp + 24]
	paddd xmm0, [___x86_globals_array4_ones]
	psrad xmm0, 1
	mov eax, [ebp + 20]
	movdqa [eax], xmm0
    
    ; restore stack pointer
	mov esp, ebp ; before push esi, edi, ebx
	sub esp, 12; after push esi, edi, ebx
    
    pop ebx
    pop edi
	pop esi
	pop ebp
	ret
	
;;; 
;;; void hl_codec_x86_264_interpol_luma13_diag4_asm_sse41(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		const uint32_t* pc_indices_horiz, // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL, // ebp + 20
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 24
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 28
;;;		)
_hl_codec_x86_264_interpol_luma13_diag4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
	mov esi, [ebp + 16]; &i_indices_stride
    mov edi, [ebp + 20]; &cSL

	and esp, -16; align stack pointer on 16 bytes

	; Local variables: 2x16 = 32 - > h(@32), s(@16)
	; Parameter addresses: 4x4 = 16
	sub esp, 48

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);;;
	mov eax, [ebp + 8]
	mov [esp], eax
	mov [esp + 4], esi
	mov [esp + 8], edi
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 32], xmm0
	mov eax, [ebp + 28]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+32, esp+32

	;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &s1);;;
	mov eax, [ebp + 12]
	mov [esp], eax
	mov [esp + 4], edi
	lea eax, [esp + 16]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41

	;;;s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 16], xmm0	
	mov eax, [ebp + 28]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+16, esp+16

	;;;predPartLXL = (h + s + 1) >> 1;;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_ones]
	psrad xmm0, 1
	mov eax, [ebp + 24]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret
	
;;; 	
;;; void hl_codec_x86_264_interpol_luma20_horiz4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const hl_pixel_t* cSL, // ebp + 12
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 16
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma20_horiz4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
	mov esi, [ebp + 8]; &pc_indices_horiz
    mov edi, [ebp + 12]; &cSL

	and esp, -16; align stack pointer on 16 bytes

	; Local variables: 1x16 = 16 - > b(@12)
	; Parameter addresses: 3x4 = 12
	sub esp, 28
    
    ;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &b);;;
	mov [esp], esi
	mov [esp + 4], edi
	lea eax, [esp + 12]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41
    ;;;predPartLXL[i] = HL_MATH_CLIP1Y(((b + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 12]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 12], xmm0	
	mov eax, [ebp + 20]
	mov ecx, [ebp + 16]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+12, ecx
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret
	
;;; 
;;; void hl_codec_x86_264_interpol_luma21_diag4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert[6], // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL, // ebp + 20
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 24
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4]  // ebp + 28
;;;		)
_hl_codec_x86_264_interpol_luma21_diag4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	push ebx
	
	mov ebx, [ebp + 12]; &pc_indices_vert
    mov esi, [ebp + 8]; &pc_indices_horiz
    mov edi, [ebp + 20]; &cSL

	and esp, -16; align stack pointer on 16 bytes

	; Local variables: 8x16 = 128 - > h1(@140), b(@124), cc(@108), dd(@92), m1(@76), ee(@60), ff(@44), j(@28)
	; Parameter addresses: 7x4 = 28
	sub esp, 156
    
    ;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &b);;;
	mov [esp], esi
	mov [esp + 4], edi
	lea eax, [esp + 124]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41
    
	;;; b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [___x86_globals_array4_sixteens]
	paddd xmm0, [esp + 124]
	psrad xmm0, 5
	movdqa [esp + 124], xmm0	
	mov eax, [ebp + 28]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+124, esp+124

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);;;
	mov eax, [ebx]
	mov [esp], eax
	mov eax, [ebp + 16]
	mov [esp + 4], eax
	mov [esp + 8], edi
	lea eax, [esp + 140]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 108]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 92]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 76]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 60]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
    
    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 44]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_math_tap6filter4x1_u32_asm_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j);;;
	lea eax, [esp + 108]
	mov [esp], eax
	lea eax, [esp + 92]
	mov [esp + 4], eax
	lea eax, [esp + 140]
	mov [esp + 8], eax
	lea eax, [esp + 76]
	mov [esp + 12], eax
	lea eax, [esp + 60]
	mov [esp + 16], eax
	lea eax, [esp + 44]
	mov [esp + 20], eax
	lea eax, [esp + 28]
	mov [esp + 24], eax
	call _hl_math_tap6filter4x1_u32_asm_sse41

    ;;; j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);;;
	movdqa xmm0, [esp + 28]
	paddd xmm0, [___x86_globals_array4_five_hundred_and_twelves]
	psrad xmm0, 10
	movdqa [esp + 28], xmm0	
	mov eax, [ebp + 28]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+28, esp+28

    ;;; predPartLXL = (b + j + 1) >> 1;;;
	movdqa xmm0, [esp + 28]
	paddd xmm0, [___x86_globals_array4_ones]
	paddd xmm0, [esp + 124]
	psrad xmm0, 1
	mov eax, [ebp + 24]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi, ebx
	sub esp, 12; after push esi, edi, ebx
	
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
	
;;; 
;;; void hl_codec_x86_264_interpol_luma22_vert4_asm_sse41(
;;;		const uint32_t* pc_indices_vert[6], // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL, // ebp + 16
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 24 
;;;		)
_hl_codec_x86_264_interpol_luma22_vert4_asm_sse41
	push ebp
    mov ebp, esp
	push esi
	push edi
	push ebx
	
	mov ebx, [ebp + 8]; &pc_indices_vert
    mov esi, [ebp + 12]; &i_indices_stride
    mov edi, [ebp + 16]; &cSL

	and esp, -16; align stack pointer on 16 bytes
	
	; Local variables: 7x16 = 112 - > h1(@124), cc(@108), dd(@92), m1(@76), ee(@60), ff(@44), j1(@28)
	; Parameter addresses: 7x4 = 28
	sub esp, 140

	mov [esp + 4], esi
	mov [esp + 8], edi    

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);;;
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 124]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 108]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 92]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 76]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 60]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 44]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
	
    ;;;hl_math_tap6filter4x1_u32_asm_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j1);;;
	HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41 esp+108,esp+92,esp+124,esp+76,esp+60,esp+44,esp+28

	;;; predPartLXL[i] = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);;;
	movdqa xmm0, [esp + 28]
	paddd xmm0, [___x86_globals_array4_five_hundred_and_twelves]
	psrad xmm0, 10
	movdqa [esp + 28], xmm0	
	mov eax, [ebp + 24]
	mov ecx, [ebp + 20]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+28, ecx
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi, ebx
	sub esp, 12; after push esi, edi, ebx
	
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
	
	
;;; 	
;;; void hl_codec_x86_264_interpol_luma23_diag4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert[6], // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL, // ebp + 20
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 24
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 28
;;;		)
_hl_codec_x86_264_interpol_luma23_diag4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	push ebx
	
	mov ebx, [ebp + 12]; &pc_indices_vert
    mov esi, [ebp + 16]; &i_indices_stride
    mov edi, [ebp + 20]; &cSL

	and esp, -16; align stack pointer on 16 bytes

	; Local variables: 8x16 = 128 - > s(@140), h1(@124), cc(@108), dd(@92), m1(@76), ee(@60), ff(@44), j(@28)
	; Parameter addresses: 7x4 = 28
	sub esp, 156

    ;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &s);;;
	mov eax, [ebp + 8]
	mov [esp], eax
	mov [esp + 4], edi
	lea eax, [esp + 140]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41
    
	;;; s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 140]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 140], xmm0
	;;;hl_math_clip2_4x1_asm_sse41;;;	
	mov eax, [ebp + 28]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+140, esp+140

	mov [esp + 4], esi
	mov [esp + 8], edi

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);;;
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 124]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 108]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 92]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 76]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 60]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 44]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;;hl_math_tap6filter4x1_u32_asm_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j);;;
	HL_MATH_TAP6FILTER_4X1_U32_ASM_SSE41 esp+108,esp+92,esp+124,esp+76,esp+60,esp+44,esp+28

	;;;j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);;;
	movdqa xmm0, [esp + 28]
	paddd xmm0, [___x86_globals_array4_five_hundred_and_twelves]
	psrad xmm0, 10
	movdqa [esp + 28], xmm0
	mov eax, [ebp + 28]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+28, esp+28
    
	;;;predPartLXL = (j + s + 1) >> 1;;;
	movdqa xmm0, [esp + 28]
	paddd xmm0, [___x86_globals_array4_ones]
	paddd xmm0, [esp + 140]
	psrad xmm0, 1
	mov eax, [ebp + 24]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi, ebx
	sub esp, 12; after push esi, edi, ebx
	
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
	
	
;;; 	
;;; void hl_codec_x86_264_interpol_luma30_horiz4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const hl_pixel_t* cSL, // ebp + 12
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 16
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma30_horiz4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
    mov esi, [ebp + 8]; &pc_indices_horiz
    mov edi, [ebp + 12]; &cSL

	and esp, -16; align stack pointer on 16 bytes

	; Local variables: 2x16 = 32 - > b(@32), G(@16)
	; Parameter addresses: 4x4 = 16
	sub esp, 48
	
    ;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &b);;;
	mov [esp], esi
	mov [esp + 4], edi
	lea eax, [esp + 32]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41

    ;;;b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 32], xmm0
	mov eax, [ebp + 20]
	HL_MATH_CLIP2_4X1_ASM_SSE41 eax, esp+32, esp+32
	
	;;;predPartLXL = (cSL[pc_indices_horiz[3]] + b + 1) >> 1;;;
	add esi, 12
	lea eax, [esp + 16]	
	HL_CODEC_X86_264_INTERPOL_LOAD_SAMPLES_CONTINUOUS_ASM_SSE2 esi, edi, eax, load19
	
	movdqa xmm0, [esp + 16]
	paddd xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_ones]
	psrad xmm0, 1
	mov eax, [ebp + 16]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret
	
;;; 
;;; void hl_codec_x86_264_interpol_luma31_diag4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert, // ebp + 12
;;;		int32_t i_indices_stride,  // ebp + 16
;;;		const hl_pixel_t* cSL,  // ebp + 20
;;;		HL_ALIGNED(16) int32_t* predPartLXL,  // ebp + 24
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4]  // ebp + 28
;;;		)
_hl_codec_x86_264_interpol_luma31_diag4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
	mov esi, [ebp + 28]; &MaxPixelValueY
    mov edi, [ebp + 20]; &cSL

	and esp, -16; align stack pointer on 16 bytes

	; Local variables: 2x16 = 32 - > b(@32), m(@16)
	; Parameter addresses: 4x4 = 16
	sub esp, 48    

    ;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &b);;;
	mov eax, [ebp + 8]
	mov [esp], eax
	mov [esp + 4], edi
	lea eax, [esp + 32]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41

	;;; b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 32], xmm0
	HL_MATH_CLIP2_4X1_ASM_SSE41 esi, esp+32, esp+32
	
	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert, i_indices_stride, cSL, &m);;;
	mov eax, [ebp + 12]
	mov [esp], eax
	mov eax, [ebp + 16]
	mov [esp + 4], eax
	mov [esp + 8], edi
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41

	;;; m = HL_MATH_CLIP1Y(((m1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 16], xmm0
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 esi, esp+16, esp+16

	;;; predPartLXL[i] = (b + m + 1) >> 1;;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_ones]
	paddd xmm0, [esp + 32]
	psrad xmm0, 1
	mov eax, [ebp + 24]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret
	
;;; 
;;; void hl_codec_x86_264_interpol_luma32_vert4_asm_sse41(
;;;		const uint32_t* pc_indices_vert[7], // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL, // ebp + 16
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 20
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma32_vert4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	push ebx
	
	mov ebx, [ebp + 8]; &pc_indices_vert
    mov esi, [ebp + 24]; &MaxPixelValueY
    mov edi, [ebp + 16]; &cSL

	and esp, -16; align stack pointer on 16 bytes
	
	; Local variables: 7x16 = 112 - > h1(@124), cc(@108), dd(@92), m1(@76), ee(@60), ff(@44), j(@28)
	; Parameter addresses: 7x4 = 28
	sub esp, 140    

	mov eax, [ebp + 12]
	mov [esp + 4], eax
	mov [esp + 8], edi
	
    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);;;
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 124]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
   
    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 108]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
    
    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 92]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
    
    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 76]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
    
    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 60]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
   
    ;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	lea eax, [esp + 44]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
    
    ;;;hl_math_tap6filter4x1_u32_asm_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j);;;
	lea eax, [esp + 108]
	mov [esp], eax
	lea eax, [esp + 92]
	mov [esp + 4], eax
	lea eax, [esp + 124]
	mov [esp + 8], eax
	lea eax, [esp + 76]
	mov [esp + 12], eax
	lea eax, [esp + 60]
	mov [esp + 16], eax
	lea eax, [esp + 44]
	mov [esp + 20], eax
	lea eax, [esp + 28]
	mov [esp + 24], eax
	call _hl_math_tap6filter4x1_u32_asm_sse41

	;;;j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);;;
	movdqa xmm0, [esp + 28]
	paddd xmm0, [___x86_globals_array4_five_hundred_and_twelves]
	psrad xmm0, 10
	movdqa [esp + 28], xmm0
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 esi, esp+28, esp+28
	
	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert[6], i_indices_stride, cSL, &m1);;;
	add ebx, 4
	mov eax, [ebx]
	mov [esp], eax
	mov eax, [ebp + 12]
	mov [esp + 4], eax
	mov [esp + 8], edi
	lea eax, [esp + 76]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
	
	;;; m = HL_MATH_CLIP1Y(((m1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 76]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 76], xmm0
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 esi, esp+76, esp+76

	;;;predPartLXL[i] = (j + m + 1) >> 1;;;
	movdqa xmm0, [esp + 76]
	paddd xmm0, [esp + 28]
	paddd xmm0, [___x86_globals_array4_ones]
	psrad xmm0, 1
	mov eax, [ebp + 20]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi, ebx
	sub esp, 12; after push esi, edi, ebx
	
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
	
;;; 
;;; void hl_codec_x86_264_interpol_luma33_diag4_asm_sse41(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert, // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL, // ebp + 20
;;;		HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 24
;;;		HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 28
;;;		)
_hl_codec_x86_264_interpol_luma33_diag4_asm_sse41:
	push ebp
    mov ebp, esp
	push esi
	push edi
	
	mov esi, [ebp + 28]; &MaxPixelValueY
    mov edi, [ebp + 20]; &cSL

	and esp, -16; align stack pointer on 16 bytes
	
	; Local variables: 2x16 = 32 - > s(@32), m(@16)
	; Parameter addresses: 4x4 = 16
	sub esp, 48
	
	;;;hl_codec_x86_264_tap6filter_horiz4_asm_sse41(pc_indices_horiz, cSL, &s);;;
	mov eax, [ebp + 8]
	mov [esp], eax
	mov [esp + 4], edi
	lea eax, [esp + 32]
	mov [esp + 8], eax
	call _hl_codec_x86_264_tap6filter_horiz4_asm_sse41
    
	;;; s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 32]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 32], xmm0
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 esi, esp+32, esp+32
	
	;;;hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices_vert, i_indices_stride, cSL, &m);;;
	mov eax, [ebp + 12]
	mov [esp], eax
	mov eax, [ebp + 16]
	mov [esp + 4], eax
	mov [esp + 8], edi
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4_asm_sse41
	
	;;;m = HL_MATH_CLIP1Y(((m1 + 16) >> 5), BitDepthY);;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_sixteens]
	psrad xmm0, 5
	movdqa [esp + 16], xmm0
	mov eax, [ebp + 24]
	HL_MATH_CLIP2_4X1_ASM_SSE41 esi, esp+16, esp+16

	;;;predPartLXL = (m + s + 1) >> 1;;;
	movdqa xmm0, [esp + 16]
	paddd xmm0, [___x86_globals_array4_ones]
	paddd xmm0, [esp + 32]
	psrad xmm0, 1
	mov eax, [ebp + 24]
	movdqa [eax], xmm0
	
	; restore stack pointer
	mov esp, ebp ; before push esi, edi
	sub esp, 8; after push esi, edi
	
	pop edi
	pop esi
	pop ebp
	ret