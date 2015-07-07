#ifndef _HARTALLO_PPFCS32_H_
#define _HARTALLO_PPFCS32_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_PPPINITFCS32  0xffffffff   /* Initial FCS value */
#define HL_PPPGOODFCS32  0xdebb20e3   /* Good final FCS value */

uint32_t hl_pppfcs32(register uint32_t fcs, register const uint8_t* cp, register int32_t len);

HL_END_DECLS

#endif /* _HARTALLO_PPFCS32_H_ */
