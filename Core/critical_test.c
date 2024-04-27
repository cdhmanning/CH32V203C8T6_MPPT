

#include "critical.h"

void critical_test(void)
{
	volatile uint32_t irq_val;
	volatile uint32_t crit_flags;

    irq_val = critical_read();

    crit_flags = critical_lock();

    irq_val = critical_read();

    irq_val = critical_unlock(crit_flags);

    irq_val = critical_read();

    (void) irq_val;
}

