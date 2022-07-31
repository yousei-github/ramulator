#ifndef __PROJECTCONFIGURATION_H
#define __PROJECTCONFIGURATION_H

#define ENABLE  (1)
#define DISABLE (0)

#define DEBUG_PRINTF (ENABLE)
#define USER_CODES   (ENABLE)

/* Functionality options */
#if USER_CODES == ENABLE
#define RAMULATOR
#define MEMORY_USE_HYBRID (ENABLE) // whether use hybrid memory system

#define KB (1024ul) // unit is byte
#define MB (KB*KB)
#define GB (MB*KB)

// Standard libraries

/* Includes */
#include <stdint.h>

/* Defines */


/* Declaration */

#endif

/** @note
 * 631.deepsjeng_s-928B.champsimtrace.xz has 2000000000 PIN traces
 *
 */
#endif
