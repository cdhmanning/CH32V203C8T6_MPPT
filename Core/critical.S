/*
 * Critical section control by disabling/enabling interrupts
 *  via the QingKe4 gintenr CSR at 0x800
 * This is a user accessable version of mstatus.
 */

.globl critical_read
.globl critical_lock
.globl critical_unlock

.text

# uint32_t critical_read(void)
critical_read:
	csrrc a0, 0x800, zero
	ret

# uint32_t critical_lock(void)
# Returns pre-locking critical flags.
critical_lock:
	add a0, zero, 0x88		# Select flags to clear
	csrrc a0, 0x800, a0
	ret

# uint32_t critical_unlock(uint32_t crit_flags)
# Applies the crit_flags returned by a previous lock
# Returns whether it was locked or not.
critical_unlock:
	csrrs a0, 0x800, a0
	ret

.end
