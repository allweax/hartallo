#ifndef _HARTALLO_PPFCS16_H_
#define _HARTALLO_PPFCS16_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_PPPINITFCS16    0xffff  /* Initial FCS value */
#define HL_PPPGOODFCS16    0xf0b8  /* Good final FCS value */

uint16_t hl_pppfcs16(register uint16_t fcs, register const uint8_t* cp, register int32_t len);

HL_END_DECLS

#endif /* _HARTALLO_PPFCS16_H_ */
