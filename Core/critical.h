/*
 * Critical section locking using irq_locking.
 * Interrupt control via the QingKe4 gintenr CSR at 0x800
 * This is a user accessable version of mstatus.
 *
 * Typical usage:
 *
 * uint32_t lock;
 *
 * lock = critical_lock();
 *
 * do critical stuff here...
 *
 * critical_unlock(lock);
 *
 */

#ifndef __CORE_IRQ_CTL_H__
#define __CORE_IRQ_CTL_H__

#include <stdint.h>

uint32_t critical_read(void);
uint32_t critical_lock(void);
uint32_t critical_unlock(uint32_t crit_flags);

#endif
