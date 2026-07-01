/*
 * microcbor public configuration.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MCBOR_CONFIG_H
#define MCBOR_CONFIG_H

#ifndef MCBOR_ENABLE_FLOAT32
#define MCBOR_ENABLE_FLOAT32 1
#endif

#if (MCBOR_ENABLE_FLOAT32 != 0) && (MCBOR_ENABLE_FLOAT32 != 1)
#error "MCBOR_ENABLE_FLOAT32 must be defined as 0 or 1"
#endif

#endif /* MCBOR_CONFIG_H */
