; Copyright (C) 2013 Mamadou DIOP
; Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
; License: GPLv3
; This file is part of Open Source Hartallo project <https://code.google.com/p/hartallo/>

bits 32

section .data
	extern ___x86_globals_sizeof_pixel

	extern ___x86_globals_array4_minus_ones
	extern ___x86_globals_array4_zeros
	extern ___x86_globals_array4_ones
	extern ___x86_globals_array4_one_bits
	extern ___x86_globals_array4_fives
	extern ___x86_globals_array4_sixes
	extern ___x86_globals_array4_sixteens
	extern ___x86_globals_array4_twenties
	extern ___x86_globals_array4_thirty_twos
	extern ___x86_globals_array4_five_hundred_and_twelves

	extern ___x86_globals_array16_fives
	extern ___x86_globals_array16_twenties

	extern ___x86_globals_array8_ones
	extern ___x86_globals_array8_twos
	extern ___x86_globals_array8_threes
	extern ___x86_globals_array8_fours
	extern ___x86_globals_array8_fives
	extern ___x86_globals_array8_twenties
	extern ___x86_globals_array8_sixteens
	extern ___x86_globals_array8_two_hundred_fiftyfive
	extern ___x86_globals_array8_five_hundred_and_twelves

	extern ___x86_globals_array4_shuffle_mask_0_0_0_0
	extern ___x86_globals_array4_shuffle_mask_0_0_0_1
	extern ___x86_globals_array4_shuffle_mask_0_0_1_1
	extern ___x86_globals_array4_shuffle_mask_0_0_1_2
	extern ___x86_globals_array4_shuffle_mask_0_1_1_1
	extern ___x86_globals_array4_shuffle_mask_0_1_2_2
	extern ___x86_globals_array4_shuffle_mask_0_1_1_0
	extern ___x86_globals_array4_shuffle_mask_1_1_1_0
	extern ___x86_globals_array4_shuffle_mask_2_2_3_3
	extern ___x86_globals_array4_shuffle_mask_2_2_3_Z
	extern ___x86_globals_array4_shuffle_mask_3_2_2_3
	extern ___x86_globals_array4_shuffle_mask_Z_Z_Z_3
	extern ___x86_globals_array4_shuffle_mask_Z_Z_1_Z
	
	extern ___x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0
	extern ___x86_globals_array4_shuffle_mask_0_0_0_0_1_1_1_1
	extern ___x86_globals_array4_shuffle_mask_0_0_1_1_2_2_3_3
	extern ___x86_globals_array4_shuffle_mask_1_1_1_1_0_0_0_0

	extern ___x86_globals_array4_shuffle_mask_cvt_8to16_lo
	extern ___x86_globals_array4_shuffle_mask_cvt_8to16_hi
	extern ___x86_globals_array4_shuffle_mask_cvt_8to32_0
	extern ___x86_globals_array4_shuffle_mask_cvt_8to32_1
	extern ___x86_globals_array4_shuffle_mask_cvt_8to32_2
	extern ___x86_globals_array4_shuffle_mask_cvt_8to32_3
	extern ___x86_globals_array4_shuffle_mask_cvt_32to8_0
	extern ___x86_globals_array4_shuffle_mask_cvt_16to32_lo
	extern ___x86_globals_array4_shuffle_mask_cvt_16to32_hi
	extern ___x86_globals_array4_shuffle_mask_cvt_16to8_lo

	extern ___x86_globals_array4_sign_P_M_M_P