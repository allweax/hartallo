; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

%include "hl_math_x86_macros_sse.asm"
%include "hl_globals_x86_sse.asm"

bits 32

global _hl_codec_x86_264_interpol_load_samples4x4_u8_asm_sse41
global _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3
global _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
global _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
global _hl_codec_x86_264_interpol_luma00_horiz4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma01_vert4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma02_vert4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma03_vert4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma10_horiz4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma11_diag4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma12_vert4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma13_diag4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma20_horiz4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma21_diag4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma22_vert4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma23_diag4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma30_horiz4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma31_diag4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma32_vert4x4_u8_asm_sse3
global _hl_codec_x86_264_interpol_luma33_diag4x4_u8_asm_sse3

extern _hl_codec_264_interpol_load_samples4x4_u8;  "func_ptr"
%define _hl_codec_264_interpol_load_samples4x4_u8_yasm [_hl_codec_264_interpol_load_samples4x4_u8]; "*func_ptr"
extern _hl_math_tap6filter4x4_u8_full_asm_sse3
extern _hl_math_tap6filter4x4_u8_partial_asm_sse3
; extern _hl_math_tap6filter4x2_u16_partial_asm_sse3 ; use func_ptr "_hl_math_tap6filter4x2_u16_partial_asm" which maps to SSE3 or SSE4.1

section .data

section .text


;;; Signature LOAD_RESULT_SSE2(dst_u16x16, src_u8x16)
;;; Loads 16x8b into "predPartLXL16x16" which contains 4x4x32b
;;; This macro changes "xmm0", "xmm1", "xmm2", "xmm3", "xmm4" and "eax"
;;; The parameters must not contains "eax"
;;; Example LOAD_RESULT_SSE2 ebp+20, esp+16
%macro LOAD_RESULT_SSE2 2
	pxor xmm0, xmm0
	mov eax, [%1]
    movdqa xmm1, [%2]
    movdqa xmm2, [%2]
    punpcklbw xmm1, xmm0
    punpckhbw xmm2, xmm0
    movdqa xmm3, xmm1
    movdqa xmm4, xmm2
    punpcklwd xmm1, xmm0
    punpckhwd xmm3, xmm0
    punpcklwd xmm2, xmm0
    punpckhwd xmm4, xmm0
    movdqa [eax], xmm1
    movdqa [eax + 64], xmm3
    movdqa [eax + 128], xmm2
    movdqa [eax + 192], xmm4
%endmacro

;;; void hl_codec_x86_264_interpol_load_samples4x4_u8_asm_sse41(
;;;		const uint32_t* pc_indices, // esp + 8
;;;		int32_t i_indices_stride,  // esp + 12
;;;		const hl_pixel_t* cSL_u8, // esp + 16
;;;		HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/ // esp + 20
;;;		)
_hl_codec_x86_264_interpol_load_samples4x4_u8_asm_sse41:
	push esi
    
	mov eax, [esp + 8]; pc_indices
	mov ecx, [esp + 16] ; cSL_u8
	mov edx, [eax + 3*4]
	sub edx, [eax]
	cmp edx, 3
	jne insert_hardway
	
	;;; EASY_WAY ;;;
	mov edx, [esp + 12] ; i_indices_stride
	
	mov esi, [eax + 0]
	pinsrd xmm0, [ecx + esi], 0
	
	mov esi, [eax + edx*4]
	pinsrd xmm0, [ecx + esi], 1
	
	mov esi, [eax + edx*8]
	pinsrd xmm0, [ecx + esi], 2
	
	lea eax, [eax + edx*8]
	mov esi, [eax + edx*4]
	pinsrd xmm0, [ecx + esi], 3
	
	mov eax, [esp + 20] ; ret_u8
	movdqa [eax], xmm0
	
	pop esi
	ret
insert_hardway:
	;;; HARD_WAY ;;;
	mov edx, [esp + 12] ; i_indices_stride
	
	mov esi, [eax]
	pinsrb xmm0, [ecx + esi], 0
	mov esi, [eax + 4]
	pinsrb xmm0, [ecx + esi], 1
	mov esi, [eax + 8]
	pinsrb xmm0, [ecx + esi], 2
	mov esi, [eax + 12]
	pinsrb xmm0, [ecx + esi], 3
	
	mov esi, [eax + edx*4]
	pinsrb xmm0, [ecx + esi], 4
	mov esi, [eax + edx*4 + 4]
	pinsrb xmm0, [ecx + esi], 5
	mov esi, [eax + edx*4 + 8]
	pinsrb xmm0, [ecx + esi], 6
	mov esi, [eax + edx*4 + 12]
	pinsrb xmm0, [ecx + esi], 7
	
	mov esi, [eax + edx*8]
	pinsrb xmm0, [ecx + esi], 8
	mov esi, [eax + edx*8 + 4]
	pinsrb xmm0, [ecx + esi], 9
	mov esi, [eax + edx*8 + 8]
	pinsrb xmm0, [ecx + esi], 10
	mov esi, [eax + edx*8 + 12]
	pinsrb xmm0, [ecx + esi], 11
	
	lea eax, [eax + edx*8]
	mov esi, [eax + edx*4]
	pinsrb xmm0, [ecx + esi], 12
	mov esi, [eax + edx*4 + 4]
	pinsrb xmm0, [ecx + esi], 13
	mov esi, [eax + edx*4 + 8]
	pinsrb xmm0, [ecx + esi], 14
	mov esi, [eax + edx*4 + 12]
	pinsrb xmm0, [ecx + esi], 15
	
	mov eax, [esp + 20] ; ret_u8
	movdqa [eax], xmm0
	
	pop esi
	ret

;;; Full Operation
;;; Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
;;; RET = Clip(((Tap6Filter + 16) >> 5))
;;; Clip operation is done by the math function when the "lo" and "hi" 16bits are packed.
;;; void hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(
;;;		const uint32_t* pc_indices, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		__m128i* ret // 16x8b values // ebp + 20
;;;		)
_hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3:
	push ebp
    mov ebp, esp
    
    push ebx
    push esi
    push edi
    
    mov esi, [ebp + 8]; &pc_indices
    mov edi, [ebp + 12]; &i_indices_stride
    mov ebx, [ebp + 16]; &cSL_u8
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 6x16 = 96 - > E(@108), F(@92), G(@76), H(@60), I(@44), J(@28)
	; Parameter addresses: 7x4 = 28
	sub esp, 124
	
	mov [esp + 4], edi
	mov [esp + 8], ebx
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&E); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 108]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, 4
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&F); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 92]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, 4
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&G); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 76]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, 4
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&H); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 60]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, 4
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&I); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 44]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, 4
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&J) ;;;
	mov [esp], esi
	lea eax, [esp + 28]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	
    ;;; hl_math_tap6filter4x4_u8_full_asm_sse3(&E, &F, &G, &H, &I, &J, ret) ;;;
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
    mov eax, [ebp + 20]
    mov [esp + 24], eax
    call _hl_math_tap6filter4x4_u8_full_asm_sse3
    
    ; restore stack pointer
	mov esp, ebp ; before push ebx, esi, edi
	sub esp, 12; after push ebx, esi, edi
    
    pop edi
    pop esi
    pop ebx
	pop ebp
	ret

;;; Full Operation
;;; Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
;;; RET = Clip(((Tap6Filter + 16) >> 5))
;;; Clip operation is done by the math function when the "lo" and "hi" 16bits are packed.
;;; void hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3(
;;;		const uint32_t* pc_indices, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		__m128i* ret // 16x8b values // ebp + 20
;;;		)
_hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3:
	push ebp
    mov ebp, esp
    
    push ebx
    push esi
    push edi
    
    mov esi, [ebp + 8]; &pc_indices
    mov edi, [ebp + 12]; &i_indices_stride
    mov ebx, [ebp + 16]; &cSL_u8
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 6x16 = 96 - > E(@108), F(@92), G(@76), H(@60), I(@44), J(@28)
	; Parameter addresses: 7x4 = 28
	sub esp, 124
	
	mov [esp + 4], edi
	mov [esp + 8], ebx
	
	shl edi, 2; make i_indices_stride in unit of sizeof("int32_t")
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&E); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 108]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&F); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 92]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&G); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 76]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&H); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 60]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&I); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 44]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&J) ;;;
	mov [esp], esi
	lea eax, [esp + 28]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	
    ;;; hl_math_tap6filter4x4_u8_full_asm_sse3(&E, &F, &G, &H, &I, &J, ret) ;;;
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
    mov eax, [ebp + 20]
    mov [esp + 24], eax
    call _hl_math_tap6filter4x4_u8_full_asm_sse3
    
    ; restore stack pointer
	mov esp, ebp ; before push ebx, esi, edi
	sub esp, 12; after push ebx, esi, edi
    
    pop edi
    pop esi
    pop ebx
	pop ebp
	ret
	

;;; void hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(
;;;		const uint32_t* pc_indices, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		__m128i* ret_lo, // 8x16b values // ebp + 20
;;;		__m128i* ret_hi // 8x16b values // ebp + 24
_hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3:
	push ebp
    mov ebp, esp
    
    push ebx
    push esi
    push edi
    
    mov esi, [ebp + 8]; &pc_indices
    mov edi, [ebp + 12]; &i_indices_stride
    mov ebx, [ebp + 16]; &cSL_u8
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 6x16 = 96 - > E(@112), F(@96), G(@80), H(@64), I(@48), J(@32)
	; Parameter addresses: 8x4 = 32
	sub esp, 128
	
	mov [esp + 4], edi
	mov [esp + 8], ebx
	
	shl edi, 2; make i_indices_stride in unit of sizeof("int32_t")
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&E); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 112]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&F); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 96]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&G); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 80]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&H); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 64]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&I); pc_indices += i_indices_stride ;;;
	mov [esp], esi
	lea eax, [esp + 48]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	add esi, edi
	
	;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&J) ;;;
	mov [esp], esi
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	
    ;;; hl_math_tap6filter4x4_u8_partial_asm_sse3(&E, &F, &G, &H, &I, &J, ret_lo, ret_hi) ;;;
    lea eax, [esp + 112]
    mov [esp], eax
    lea eax, [esp + 96]
    mov [esp + 4], eax
    lea eax, [esp + 80]
    mov [esp + 8], eax
    lea eax, [esp + 64]
    mov [esp + 12], eax
    lea eax, [esp + 48]
    mov [esp + 16], eax
    lea eax, [esp + 32]
    mov [esp + 20], eax
    mov eax, [ebp + 20]
    mov [esp + 24], eax
    mov eax, [ebp + 24]
    mov [esp + 28], eax
    call _hl_math_tap6filter4x4_u8_partial_asm_sse3
    
    ; restore stack pointer
	mov esp, ebp ; before push ebx, esi, edi
	sub esp, 12; after push ebx, esi, edi
    
    pop edi
    pop esi
    pop ebx
	pop ebp
	ret
	

;;; 
;;; void hl_codec_x86_264_interpol_luma00_horiz4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8,  // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1  // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma00_horiz4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
        
    and esp, -16; align on 16 bytes
    
    ; Local variables: 16 = ret_u8(@16)
	; Parameter addresses: (4 + 4 + 4 + 4) = 16
    sub esp, 32
    
    ;;; hl_codec_264_interpol_load_samples4x4_u8(pc_indices_horiz, i_indices_stride, cSL_u8, (uint8_t*)&ret_u8);;;
    mov eax, [ebp + 8]
    mov [esp], eax
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    lea eax, [esp + 16]
    mov [esp + 12], eax
    call _hl_codec_264_interpol_load_samples4x4_u8_yasm
    
    mov eax, [ebp + 20]
    movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0
    
    mov esp,ebp
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma01_vert4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma01_vert4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
        
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = G(@32), h(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov ecx, [ebp + 12]
    mov [esp + 4], ecx
    mov edx, [ebp + 16]
    mov [esp + 8], edx
    
	;;; hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_vert[(i_indices_stride << 1)], i_indices_stride, cSL_u8, (uint8_t*)&G) ;;;
	mov eax, [ebp + 8]
	shl ecx, 3
	add eax, ecx
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	
	;;; h = Clip1Y(((Tap6Filter(indices) + 16) >> 5));;;
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3
	
	;;; h = ((h + G + 1) >> 1);;;
	movdqa xmm0, [esp + 16]
	pavgb xmm0, [esp + 32]
	movdqa [esp + 16], xmm0
    
    mov eax, [ebp + 20]
    movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0
    
    mov esp,ebp
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma02_vert4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma02_vert4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
        
    and esp, -16; align on 16 bytes
    
    ; Local variables: 1x16 = 16 = h1(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 32
    
    ;;; h1 = Clip1Y(((Tap6Filter(indices) + 16) >> 5));;;
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h1) ;;;
    mov eax, [ebp + 8]
    mov [esp], eax
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    lea eax, [esp + 16]
    mov [esp + 12], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3
    
    mov eax, [ebp + 20]
    movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0

	mov esp,ebp
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma03_vert4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma03_vert4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = h(@32), G(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    
    ;;; h = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3
	
	;;; Load G ;;;
	;;; hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_vert[(i_indices_stride * 3)], i_indices_stride, cSL_u8, (uint8_t*)&G) ;;;
	mov eax, [ebp + 8]
	mov edx, [ebp + 12]
	imul edx, 12
	add eax, edx
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	
	;;; h = ((h + G + 1) >> 1) ;;;
	movdqa xmm0, [esp + 32]
	pavgb xmm0, [esp + 16]
	movdqa [esp + 16], xmm0
    
    mov eax, [ebp + 20]
    movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0

	mov esp,ebp
	pop ebp
	ret
    
;;; void hl_codec_x86_264_interpol_luma10_horiz4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma10_horiz4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = b(@32), G(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    
    ;;; load (unaligned) G ;;;
	;;; hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_horiz[2], i_indices_stride, cSL_u8, (uint8_t*)&G) ;;;
	mov eax, [ebp + 8]
	add eax, 8
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	
	;;; b = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3

	;;; b = ((b + G + 1) >> 1) ;;;
	movdqa xmm0, [esp + 16]
	pavgb xmm0, [esp + 32]
	movdqa [esp + 16], xmm0
	
	mov eax, [ebp + 20]
	movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0

	mov esp,ebp
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma11_diag4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		const uint32_t* pc_indices_horiz, // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL_u8, // ebp + 20
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma11_diag4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = h(@32), b(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov eax, [ebp + 16]
    mov [esp + 4], eax
    mov eax, [ebp + 20]
    mov [esp + 8], eax
    
    ;;; h = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3	
	
	;;; b = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b) ;;;
	mov eax, [ebp + 12]
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
	
	;;; b = (b + h + 1) >> 1 ;;;
	movdqa xmm0, [esp + 16]
	pavgb xmm0, [esp + 32]
	movdqa [esp + 16], xmm0
    
    mov eax, [ebp + 24]
    movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0
    
    mov esp,ebp
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma12_vert4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert[6], // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma12_vert4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    push esi
    
    mov esi, [ebp + 8]; &pc_indices_vert
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 17x16 = 272 = h1_lo(@288), h1_hi(@272), h(@256), cc_lo(@240), cc_hi(@224), dd_lo(@208), dd_hi(@192), m1_lo(@176), m1_hi(@160), ee_lo(@144), ee_hi(@128), ff_lo(@112), ff_hi(@96), j1_lo(@80), j1_hi(@64), j_lo(@48), j_hi(@32)
	; Parameter addresses: 8x4 = 32
    sub esp, 304
    
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi);;; // (h1_lo, h1_hi) -> 8x16b
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 288]
    mov [esp + 12], eax
    lea eax, [esp + 272]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY) ;;; // h -> 16x8b    
    movdqa xmm0, [esp + 288]
    pshufd xmm6, [___x86_globals_array8_sixteens], 0xE4
    paddw xmm0, xmm6
    psraw xmm0, 5
    movdqa xmm1, [esp + 272]
    paddw xmm1, xmm6
    psraw xmm1, 5
    packuswb xmm0, xmm1
    movdqa [esp + 256], xmm0
    
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi) ;;; // (cc_lo, cc_hi) -> 8x16b
	add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 240]
    mov [esp + 12], eax
    lea eax, [esp + 224]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi) ;;; // (dd_lo, dd_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 208]
    mov [esp + 12], eax
    lea eax, [esp + 192]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi) ;;; // (m1_lo, m1_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 176]
    mov [esp + 12], eax
    lea eax, [esp + 160]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi) ;;; // (ee_lo, ee_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 144]
    mov [esp + 12], eax
    lea eax, [esp + 128]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi) ;;; // (ff_lo, ff_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 112]
    mov [esp + 12], eax
    lea eax, [esp + 96]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_math_tap6filter4x2_u16_partial_asm(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 240]
	mov [esp], eax
	lea eax, [esp + 208]
	mov [esp + 4], eax
	lea eax, [esp + 288]
	mov [esp + 8], eax
	lea eax, [esp + 176]
	mov [esp + 12], eax
	lea eax, [esp + 144]
	mov [esp + 16], eax
	lea eax, [esp + 112]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_lo = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    paddd xmm0, xmm7
    psrad xmm0, 10
    movdqa xmm1, [esp + 64]
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_lo -> 8x16b
    movdqa [esp + 48], xmm0
    
	;;; _hl_math_tap6filter4x2_u16_partial_asm(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 224]
	mov [esp], eax
	lea eax, [esp + 192]
	mov [esp + 4], eax
	lea eax, [esp + 272]
	mov [esp + 8], eax
	lea eax, [esp + 160]
	mov [esp + 12], eax
	lea eax, [esp + 128]
	mov [esp + 16], eax
	lea eax, [esp + 96]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_hi = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    paddd xmm0, xmm7
    psrad xmm0, 10
    movdqa xmm1, [esp + 64]
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_hi -> 8x16b
    movdqa [esp + 32], xmm0
    
    ;;; h = (h + j + 1) >> 1; with j = packuswb(j_lo, j_hi) -> 16x8b
    movdqa xmm0, [esp + 48]
    packuswb xmm0, [esp + 32]
    pavgb xmm0, [esp + 256]
	movdqa [esp + 256], xmm0
    
    mov eax, [ebp + 20]
    movdqa xmm0, [esp + 256]
    movdqa [eax], xmm0
    
    mov esp, ebp ; before push esi
	sub esp, 4; after push esi
    
    pop esi
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma13_diag4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert, // ebp + 8
;;;		const uint32_t* pc_indices_horiz, // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL_u8, // ebp + 20
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma13_diag4x4_u8_asm_sse3:
    push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = h(@32), s(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov eax, [ebp + 16]
    mov [esp + 4], eax
    mov eax, [ebp + 20]
    mov [esp + 8], eax
	
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3
	
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &s) ;;;
	mov eax, [ebp + 12]
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
	
	;;; s = (s + h + 1) >> 1 ;;;
	movdqa xmm0, [esp + 16]
	pavgb xmm0, [esp + 32]
	movdqa [esp + 16], xmm0
    
    mov eax, [ebp + 24]
    movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0
    
	mov esp,ebp
	pop ebp
	ret
	
;;; void hl_codec_x86_264_interpol_luma20_horiz4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma20_horiz4x4_u8_asm_sse3:
	 push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 1x16 = 16 = b(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 32
	
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	mov eax, [ebp + 12]
	mov [esp + 4], eax
	mov eax, [ebp + 16]
	mov [esp + 8], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
	
	mov eax, [ebp + 20]
	movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0
    
	mov esp,ebp
	pop ebp
	ret

;;; void hl_codec_x86_264_interpol_luma21_diag4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert[6], // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL_u8, // ebp + 20
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma21_diag4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    push esi
    
    mov esi, [ebp + 12]; &pc_indices_vert
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 17x16 = 272 = h1_lo(@288), h1_hi(@272), b(@256), cc_lo(@240), cc_hi(@224), dd_lo(@208), dd_hi(@192), m1_lo(@176), m1_hi(@160), ee_lo(@144), ee_hi(@128), ff_lo(@112), ff_hi(@96), j1_lo(@80), j1_hi(@64), j_lo(@48), j_hi(@32)
	; Parameter addresses: 8x4 = 32
    sub esp, 304
    
    mov eax, [ebp + 16]
    mov [esp + 4], eax
    mov eax, [ebp + 20]
    mov [esp + 8], eax
    
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b);;; // (b) -> 8x16b
	mov eax, [ebp + 8]
    mov [esp], eax
    lea eax, [esp + 256]
    mov [esp + 12], eax
    call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi) ;;; // (h1_lo, h1_hi) -> 8x16b
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 288]
    mov [esp + 12], eax
    lea eax, [esp + 272]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi) ;;; // (cc_lo, cc_hi) -> 8x16b
	add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 240]
    mov [esp + 12], eax
    lea eax, [esp + 224]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi) ;;; // (dd_lo, dd_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 208]
    mov [esp + 12], eax
    lea eax, [esp + 192]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi) ;;; // (m1_lo, m1_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 176]
    mov [esp + 12], eax
    lea eax, [esp + 160]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi) ;;; // (ee_lo, ee_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 144]
    mov [esp + 12], eax
    lea eax, [esp + 128]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi) ;;; // (ff_lo, ff_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 112]
    mov [esp + 12], eax
    lea eax, [esp + 96]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_math_tap6filter4x2_u16_partial_asm(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 240]
	mov [esp], eax
	lea eax, [esp + 208]
	mov [esp + 4], eax
	lea eax, [esp + 288]
	mov [esp + 8], eax
	lea eax, [esp + 176]
	mov [esp + 12], eax
	lea eax, [esp + 144]
	mov [esp + 16], eax
	lea eax, [esp + 112]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_lo = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    paddd xmm0, xmm7
    psrad xmm0, 10
    movdqa xmm1, [esp + 64]
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_lo -> 8x16b
    movdqa [esp + 48], xmm0
    
	;;; _hl_math_tap6filter4x2_u16_partial_asm(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 224]
	mov [esp], eax
	lea eax, [esp + 192]
	mov [esp + 4], eax
	lea eax, [esp + 272]
	mov [esp + 8], eax
	lea eax, [esp + 160]
	mov [esp + 12], eax
	lea eax, [esp + 128]
	mov [esp + 16], eax
	lea eax, [esp + 96]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_hi = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 64]
    paddd xmm0, xmm7
    psrad xmm0, 10
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_hi -> 8x16b
    movdqa [esp + 32], xmm0
    
    ;;; b = (b + j + 1) >> 1; with j = packuswb(j_lo, j_hi) -> 16x8b
    movdqa xmm0, [esp + 48]
    packuswb xmm0, [esp + 32]
    pavgb xmm0, [esp + 256]
	movdqa [esp + 256], xmm0
    
    mov eax, [ebp + 24]
    movdqa xmm0, [esp + 256]
    movdqa [eax], xmm0
    
    mov esp, ebp ; before push esi
	sub esp, 4; after push esi
    
    pop esi
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma22_vert4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert[6], // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma22_vert4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    push esi
    
    mov esi, [ebp + 8]; &pc_indices_vert
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 17x16 = 272 = h1_lo(@288), h1_hi(@272), j(@256), cc_lo(@240), cc_hi(@224), dd_lo(@208), dd_hi(@192), m1_lo(@176), m1_hi(@160), ee_lo(@144), ee_hi(@128), ff_lo(@112), ff_hi(@96), j1_lo(@80), j1_hi(@64), j_lo(@48), j_hi(@32)
	; Parameter addresses: 8x4 = 32
    sub esp, 304
    
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi);;; // (h1_lo, h1_hi) -> 8x16b
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 288]
    mov [esp + 12], eax
    lea eax, [esp + 272]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY) ;;; // h -> 16x8b
    movdqa xmm0, [esp + 288]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 272]
    paddw xmm0, xmm7
    psraw xmm0, 5
    paddw xmm1, xmm7
    psraw xmm1, 5
    packuswb xmm0, xmm1
    movdqa [esp + 256], xmm0
    
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi) ;;; // (cc_lo, cc_hi) -> 8x16b
	add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 240]
    mov [esp + 12], eax
    lea eax, [esp + 224]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi) ;;; // (dd_lo, dd_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 208]
    mov [esp + 12], eax
    lea eax, [esp + 192]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi) ;;; // (m1_lo, m1_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 176]
    mov [esp + 12], eax
    lea eax, [esp + 160]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi) ;;; // (ee_lo, ee_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 144]
    mov [esp + 12], eax
    lea eax, [esp + 128]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi) ;;; // (ff_lo, ff_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 112]
    mov [esp + 12], eax
    lea eax, [esp + 96]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_math_tap6filter4x2_u16_partial_asm(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 240]
	mov [esp], eax
	lea eax, [esp + 208]
	mov [esp + 4], eax
	lea eax, [esp + 288]
	mov [esp + 8], eax
	lea eax, [esp + 176]
	mov [esp + 12], eax
	lea eax, [esp + 144]
	mov [esp + 16], eax
	lea eax, [esp + 112]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_lo = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 64]
    paddd xmm0, xmm7
    psrad xmm0, 10
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_lo -> 8x16b
    movdqa [esp + 48], xmm0
    
	;;; _hl_math_tap6filter4x2_u16_partial_asm(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 224]
	mov [esp], eax
	lea eax, [esp + 192]
	mov [esp + 4], eax
	lea eax, [esp + 272]
	mov [esp + 8], eax
	lea eax, [esp + 160]
	mov [esp + 12], eax
	lea eax, [esp + 128]
	mov [esp + 16], eax
	lea eax, [esp + 96]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_hi = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 64]
    paddd xmm0, xmm7
    psrad xmm0, 10
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_hi -> 8x16b
    movdqa [esp + 32], xmm0
    
    ;;; j = packs(j_lo, j_hi) -> 16x8b ;;;
    movdqa xmm0, [esp + 48]
    packuswb xmm0, [esp + 32]
	movdqa [esp + 256], xmm0
    
    mov eax, [ebp + 20]
    movdqa xmm0, [esp + 256]
    movdqa [eax], xmm0
    
    mov esp, ebp ; before push esi
	sub esp, 4; after push esi
    
    pop esi
	pop ebp
	ret
	
;;; void hl_codec_x86_264_interpol_luma23_diag4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert[6], // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL_u8, // ebp + 20
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma23_diag4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    push esi
    
    mov esi, [ebp + 12]; &pc_indices_vert
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 17x16 = 272 = h1_lo(@288), h1_hi(@272), s(@256), cc_lo(@240), cc_hi(@224), dd_lo(@208), dd_hi(@192), m1_lo(@176), m1_hi(@160), ee_lo(@144), ee_hi(@128), ff_lo(@112), ff_hi(@96), j1_lo(@80), j1_hi(@64), j_lo(@48), j_hi(@32)
	; Parameter addresses: 8x4 = 32
    sub esp, 304
    
    mov eax, [ebp + 16]
    mov [esp + 4], eax
    mov eax, [ebp + 20]
    mov [esp + 8], eax
    
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &s);;; // s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY); (s) -> 8x16b
	mov eax, [ebp + 8]
    mov [esp], eax
    lea eax, [esp + 256]
    mov [esp + 12], eax
    call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi) ;;; // (h1_lo, h1_hi) -> 8x16b
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 288]
    mov [esp + 12], eax
    lea eax, [esp + 272]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi) ;;; // (cc_lo, cc_hi) -> 8x16b
	add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 240]
    mov [esp + 12], eax
    lea eax, [esp + 224]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi) ;;; // (dd_lo, dd_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 208]
    mov [esp + 12], eax
    lea eax, [esp + 192]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi) ;;; // (m1_lo, m1_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 176]
    mov [esp + 12], eax
    lea eax, [esp + 160]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi) ;;; // (ee_lo, ee_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 144]
    mov [esp + 12], eax
    lea eax, [esp + 128]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi) ;;; // (ff_lo, ff_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 112]
    mov [esp + 12], eax
    lea eax, [esp + 96]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_math_tap6filter4x2_u16_partial_asm(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 240]
	mov [esp], eax
	lea eax, [esp + 208]
	mov [esp + 4], eax
	lea eax, [esp + 288]
	mov [esp + 8], eax
	lea eax, [esp + 176]
	mov [esp + 12], eax
	lea eax, [esp + 144]
	mov [esp + 16], eax
	lea eax, [esp + 112]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_lo = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 64]
    paddd xmm0, xmm7
    psrad xmm0, 10
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_lo -> 8x16b
    movdqa [esp + 48], xmm0
    
	;;; _hl_math_tap6filter4x2_u16_partial_asm(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 224]
	mov [esp], eax
	lea eax, [esp + 192]
	mov [esp + 4], eax
	lea eax, [esp + 272]
	mov [esp + 8], eax
	lea eax, [esp + 160]
	mov [esp + 12], eax
	lea eax, [esp + 128]
	mov [esp + 16], eax
	lea eax, [esp + 96]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_hi = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 64]
    paddd xmm0, xmm7
    psrad xmm0, 10
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_hi -> 8x16b
    movdqa [esp + 32], xmm0
    
    ;;; s = (s + j + 1) >> 1; with j = packuswb(j_lo, j_hi) -> 16x8b
    movdqa xmm0, [esp + 48]
    packuswb xmm0, [esp + 32]
    pavgb xmm0, [esp + 256]
	movdqa [esp + 256], xmm0
    
    mov eax, [ebp + 24]
    movdqa xmm0, [esp + 256]
    movdqa [eax], xmm0
    
    mov esp, ebp ; before push esi
	sub esp, 4; after push esi
    
    pop esi
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma30_horiz4x4_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma30_horiz4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = b(@32), G(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    
    ;;; load (unaligned) G ;;;
	;;; hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_horiz[3], i_indices_stride, cSL_u8, (uint8_t*)&G) ;;;
	mov eax, [ebp + 8]
	add eax, 12
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_264_interpol_load_samples4x4_u8_yasm
	
	;;; b = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3

	;;; b = ((b + G + 1) >> 1) ;;;
	movdqa xmm0, [esp + 16]
	pavgb xmm0, [esp + 32]
	movdqa [esp + 16], xmm0
	
	mov eax, [ebp + 20]
    movdqa xmm0, [esp + 16]
    movdqa [eax], xmm0

	mov esp,ebp
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma31_diag4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert, // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL_u8, // ebp + 20
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma31_diag4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = b(@32), m(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov eax, [ebp + 16]
    mov [esp + 4], eax
    mov eax, [ebp + 20]
    mov [esp + 8], eax
    
    ;;; b = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
	
    ;;; m = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &m); ;;;
	mov eax, [ebp + 12]
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3

	;;; b = (b + m + 1) >> 1 ;;;
	movdqa xmm0, [esp + 16]
	pavgb xmm0, [esp + 32]
	movdqa [esp + 32], xmm0
    
    mov eax, [ebp + 24]
    movdqa xmm0, [esp + 32]
    movdqa [eax], xmm0

	mov esp,ebp
	pop ebp
	ret
	
	
;;; void hl_codec_x86_264_interpol_luma32_vert4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_vert[7], // ebp + 8
;;;		int32_t i_indices_stride, // ebp + 12
;;;		const hl_pixel_t* cSL_u8, // ebp + 16
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 20
;;;		)
_hl_codec_x86_264_interpol_luma32_vert4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    push esi
    
    mov esi, [ebp + 8]; &pc_indices_vert
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 17x16 = 272 = h1_lo(@288), h1_hi(@272), s(@256), cc_lo(@240), cc_hi(@224), dd_lo(@208), dd_hi(@192), m1_lo(@176), m1_hi(@160), ee_lo(@144), ee_hi(@128), ff_lo(@112), ff_hi(@96), j1_lo(@80), j1_hi(@64), j_lo(@48), j_hi(@32)
	; Parameter addresses: 8x4 = 32
    sub esp, 304
    
    mov eax, [ebp + 12]
    mov [esp + 4], eax
    mov eax, [ebp + 16]
    mov [esp + 8], eax
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi) ;;; // (h1_lo, h1_hi) -> 8x16b
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 288]
    mov [esp + 12], eax
    lea eax, [esp + 272]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi) ;;; // (cc_lo, cc_hi) -> 8x16b
	add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 240]
    mov [esp + 12], eax
    lea eax, [esp + 224]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi) ;;; // (dd_lo, dd_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 208]
    mov [esp + 12], eax
    lea eax, [esp + 192]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi) ;;; // (m1_lo, m1_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 176]
    mov [esp + 12], eax
    lea eax, [esp + 160]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi) ;;; // (ee_lo, ee_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 144]
    mov [esp + 12], eax
    lea eax, [esp + 128]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi) ;;; // (ff_lo, ff_hi) -> 8x16b
    add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 112]
    mov [esp + 12], eax
    lea eax, [esp + 96]
    mov [esp + 16], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_partial_asm_sse3
    
    ;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert[6], i_indices_stride, cSL_u8, &s);;; // s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY); (s) -> 8x16b
	add esi, 4
	mov eax, [esi]
    mov [esp], eax
    lea eax, [esp + 256]
    mov [esp + 12], eax
    call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3
    
    ;;; hl_math_tap6filter4x2_u16_partial_asm(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 240]
	mov [esp], eax
	lea eax, [esp + 208]
	mov [esp + 4], eax
	lea eax, [esp + 288]
	mov [esp + 8], eax
	lea eax, [esp + 176]
	mov [esp + 12], eax
	lea eax, [esp + 144]
	mov [esp + 16], eax
	lea eax, [esp + 112]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_lo = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 64]
    paddd xmm0, xmm7
    psrad xmm0, 10
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_lo -> 8x16b
    movdqa [esp + 48], xmm0
    
	;;; _hl_math_tap6filter4x2_u16_partial_asm(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi) ;;; // (j1_lo, j1_hi) -> 4x32b
	lea eax, [esp + 224]
	mov [esp], eax
	lea eax, [esp + 192]
	mov [esp + 4], eax
	lea eax, [esp + 272]
	mov [esp + 8], eax
	lea eax, [esp + 160]
	mov [esp + 12], eax
	lea eax, [esp + 128]
	mov [esp + 16], eax
	lea eax, [esp + 96]
	mov [esp + 20], eax
	lea eax, [esp + 80]
	mov [esp + 24], eax
	lea eax, [esp + 64]
	mov [esp + 28], eax
    call _hl_math_tap6filter4x2_u16_partial_asm
    
    ;;; j_hi = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY) ;;;
    movdqa xmm0, [esp + 80]
    pshufd xmm7, [___x86_globals_array4_five_hundred_and_twelves], 0xE4
    movdqa xmm1, [esp + 64]
    paddd xmm0, xmm7
    psrad xmm0, 10
    paddd xmm1, xmm7
    psrad xmm1, 10
    packssdw xmm0, xmm1 ; j_hi -> 8x16b
    movdqa [esp + 32], xmm0
    
    ;;; s = (s + j + 1) >> 1; with j = packuswb(j_lo, j_hi) -> 16x8b
    movdqa xmm0, [esp + 48]
    packuswb xmm0, [esp + 32]
    pavgb xmm0, [esp + 256]
	movdqa [esp + 256], xmm0
    
    mov eax, [ebp + 20]
    movdqa xmm0, [esp + 256]
    movdqa [eax], xmm0
    
    mov esp, ebp ; before push esi
	sub esp, 4; after push esi
    
    pop esi
	pop ebp
	ret
	
;;; void hl_codec_x86_264_interpol_luma33_diag4x4_u8_asm_sse3(
;;;		const uint32_t* pc_indices_horiz, // ebp + 8
;;;		const uint32_t* pc_indices_vert, // ebp + 12
;;;		int32_t i_indices_stride, // ebp + 16
;;;		const hl_pixel_t* cSL_u8, // ebp + 20
;;;		HL_ALIGNED(16) uint8_t* predPartLXL16x1 // ebp + 24
;;;		)
_hl_codec_x86_264_interpol_luma33_diag4x4_u8_asm_sse3:
	push ebp
    mov ebp, esp
    
    and esp, -16; align on 16 bytes
    
    ; Local variables: 2x16 = 32 = s(@32), m(@16)
	; Parameter addresses: 4x4 = 16
    sub esp, 48
    
    mov eax, [ebp + 16]
    mov [esp + 4], eax
    mov eax, [ebp + 20]
    mov [esp + 8], eax
    
    ;;; s = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &s) ;;;
	mov eax, [ebp + 8]
	mov [esp], eax
	lea eax, [esp + 32]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_horiz4x4_u8_full_asm_sse3
	
    ;;; m = Clip1Y(((Tap6Filter(indices) + 16) >> 5)) ;;;
	;;; hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &m); ;;;
	mov eax, [ebp + 12]
	mov [esp], eax
	lea eax, [esp + 16]
	mov [esp + 12], eax
	call _hl_codec_x86_264_tap6filter_vert4x4_u8_full_asm_sse3

	;;; s = (s + m + 1) >> 1 ;;;
	movdqa xmm0, [esp + 16]
	pavgb xmm0, [esp + 32]
	movdqa [esp + 32], xmm0
    
    mov eax, [ebp + 24]
    movdqa xmm0, [esp + 32]
    movdqa [eax], xmm0

	mov esp,ebp
	pop ebp
	ret