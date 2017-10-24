/*
 * VNPE_reenable.h
 *
 *  Created on: 2015-02-16
 *      Author: wcs
 */

#undef PE_NULL
#undef PE2_NULL
#undef PE_EOF
#undef PE
#undef PE2
#undef PE_NOT
#undef PE_NEG
#undef PE_0
#undef PE_EOK

#define PE_NULL(function) PE_NULL_P(function)
#define PE2_NULL(function, info) PE2_NULL_P(function, info)
#define PE_EOF(function) PE_EOF_P(function)
#define PE(function) PE_P(function)
#define PE2(function, info) PE2_P(function, info)
#define PE_NEG(function) PE_NEG_P(function)
#define PE_NOT(function, desiredRv) PE_NOT_P(function, desiredRv)

#define PE_0(function) PE_0_P(function)
#ifdef EOK
#define PE_EOK(function) PE_EOK_P(function)
#endif
