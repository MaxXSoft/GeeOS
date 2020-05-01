// structure definition of spinlock
typedef struct {
  unsigned locked;
} Spinlock;

int __try_to_acquire(Spinlock *lock) {
  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  //   a5 = 1
  //   s1 = &lock->locked
  //   amoswap.w.aq a5, a5, (s1)
  return __sync_lock_test_and_set(&lock->locked, 1);
}

void __release(Spinlock *lock) {
  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released.
  // On RISC-V, this turns into a fence instruction.
  __sync_synchronize();
  // Release the lock, equivalent to lock->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  // On RISC-V, sync_lock_release turns into an atomic swap:
  //   s1 = &lock->locked
  //   amoswap.w zero, zero, (s1)
  __sync_lock_release(&lock->locked);
}
