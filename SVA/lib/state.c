/*===- state.c - SVA Execution Engine  ------------------------------------===
 * 
 *                        Secure Virtual Architecture
 *
 * This file was developed by the LLVM research group and is distributed under
 * the University of Illinois Open Source License. See LICENSE.TXT for details.
 * 
 *===----------------------------------------------------------------------===
 *
 * Provide intrinsics for manipulating the LLVA machine state.
 *
 *===----------------------------------------------------------------------===
 */

#if 0
#include <sva/config.h>
#endif
#include <sva/util.h>
#include <sva/state.h>
#include <sva/interrupt.h>

/*****************************************************************************
 * Interrupt Context Intrinsics
 ****************************************************************************/

#if 0
/*
 * Intrinsic: sva_init_icontext()
 *
 * Description:
 *  Take a new kernel stack and duplicate a given interrupt context on to
 *  the new stack.  This is primarily used for new thread creation.
 */
sva_sp_t
sva_init_icontext (sva_icontext_t * icontext, void * stackp)
{
  /* Working memory pointer */
  unsigned char * p;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  /*
   * Find a nice place on the stack.
   */
  p = (((unsigned char *)(stackp)) - sizeof (sva_icontext_t));

  /*
   * Verify that the memory has the proper access.
   */
  sva_check_memory_read  (icontext, sizeof (sva_icontext_t));
  sva_check_memory_write (p,        sizeof (sva_icontext_t));

  /*
   * Copy the interrupt context on to the new stack.
   */
  __builtin_memcpy (p, icontext, sizeof (sva_icontext_t));

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return ((sva_sp_t)p);
}

void
sva_clear_icontext (sva_icontext_t * icontext)
{
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  /*
   * Verify that the memory has the proper access.
   */
  sva_check_memory_read  (icontext, sizeof (sva_icontext_t));

  /*
   * Clear all of the general purpose registers.
   */
  icontext->eax = 0;
  icontext->ebx = 0;
  icontext->ecx = 0;
  icontext->edx = 0;
  icontext->esi = 0;
  icontext->edi = 0;
  icontext->ebp = 0;

  /*
   * Initialize the interrupt state.
   */
  icontext->enable_shim = 0;
#if LLVA_SCLIMIT
  icontext->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_was_privileged()
 *
 * Description:
 *  This intrinsic flags whether the specified context was running in a
 *  privileged state before the interrupt/exception occurred.
 */
unsigned char
sva_was_privileged (sva_icontext_t * icontext)
{
  return (((icontext->cs) & 0x10) == 0x10);
}
#endif

/*****************************************************************************
 * Miscellaneous State Manipulation Functions
 ****************************************************************************/

#if 0
/*
 * Intrinsic: sva_ipop_function0 ()
 *
 * Description:
 *  This intrinsic modifies the user space process code so that the
 *  function currently being executed is removed, with it's preceding function
 *  being executed instead.
 *
 * Inputs:
 *  exceptp  - A pointer to the exception handler saved state.
 *
 * TODO:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 *
 *  o This intrinsic is assuming that I don't have anything on the stack.
 *    That's, um, not necessairly true all the time.
 */
void
sva_ipop_function0 (void * exceptp)
{
  /* User Context Pointer */
  sva_icontext_t * ep = exceptp;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  do
  {
    /*
     * Check the memory.
     */
    sva_check_memory_write (ep, sizeof (sva_icontext_t));
    sva_check_memory_write (ep->rsp, sizeof (unsigned int));

    /*
     * Verify that this interrupt context has a stack pointer.
     */
    if (sva_is_privileged () && sva_was_privileged(ep))
    {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }
    break;
  } while (1);

  /*
   * Pop the return PC pointer from the stack.
   */
  ep->rip = *((ep->rsp)++);

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_ipush_function0 ()
 *
 * Description:
 *  This intrinsic modifies the user space process code so that the
 *  specified function was called with the given arguments.
 *
 * Inputs:
 *  icontext - A pointer to the exception handler saved state.
 *  newf     - The function to call.
 *
 * NOTES:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 */
void
sva_ipush_function0 (void * icontext, void (*newf)(void))
{
  /* User Context Pointer */
  sva_icontext_t * ip = icontext;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  /*
   * Check the memory.
   */
  sva_check_memory_write (ip,      sizeof (sva_icontext_t));
  sva_check_memory_write (ip->rsp, sizeof (unsigned int) * 1);

  /*
   * Verify that this interrupt context has a stack pointer.
   */
#if 0
  if (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }
#endif

  /* Performance counters */
#if LLVA_COUNTERS
  ++sva_counters.sva_ipush_func;
  if (sva_debug) ++sva_local_counters.sva_ipush_func;
  sc_intrinsics[current_sysnum] |= MASK_LLVA_IPUSH_FUNCTION;
#endif

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ip->rsp)) = ip->rip;

  /*
   * Set the return function to be the specificed function.
   */
  ip->rip = (unsigned int)newf;

  /*
   * Disable restrictions on system calls since we don't know where this
   * function pointer came from.
   */
#if LLVA_SCLIMIT
  ip->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_ipush_function1 ()
 *
 * Description:
 *  This intrinsic modifies the user space process code so that the
 *  specified function was called with the given arguments.
 *
 * Inputs:
 *  icontext - A pointer to the exception handler saved state.
 *  newf     - The function to call.
 *  param    - The parameter to send to the function.
 *
 * TODO:
 *  This currently only takes a function that takes a single integer
 *  argument.  Eventually, this should take any function.
 *
 * NOTES:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 */
void
sva_ipush_function1 (void * icontext, void (*newf)(int), int param)
{
  /* User Context Pointer */
  sva_icontext_t * ep = icontext;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  do
  {
    /*
     * Check the memory.
     */
    sva_check_memory_write (ep, sizeof (sva_icontext_t));
    sva_check_memory_write (ep->rsp, sizeof (unsigned int) * 2);

    /*
     * Verify that this interrupt context has a stack pointer.
     */
    if (sva_is_privileged () && sva_was_privileged(icontext))
    {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }
    break;
  } while (1);

  /* Performance counters */
#if LLVA_COUNTERS
  ++sva_counters.sva_ipush_func;
  if (sva_debug) ++sva_local_counters.sva_ipush_func;
  sc_intrinsics[current_sysnum] |= MASK_LLVA_IPUSH_FUNCTION;
#endif

  /*
   * Push the one argument on to the user space stack.
   */
  *(--(ep->rsp)) = param;

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ep->rsp)) = ep->rip;

  /*
   * Set the return function to be the specificed function.
   */
  ep->rip = (unsigned int)newf;

  /*
   * Disable restrictions on system calls since we don't know where this
   * function pointer came from.
   */
#if LLVA_SCLIMIT
  ep->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}
#endif


/*
 * Intrinsic: sva_ipush_function3 ()
 *
 * Description:
 *  This intrinsic modifies the user space process code so that the
 *  specified function was called with the given arguments.
 *
 * Inputs:
 *  newf     - The function to call.
 *  p[1|2|3] - The parameters to pass to the function.
 *
 * NOTES:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 */
void
sva_ipush_function3 (void (*newf)(int, int, int), int p1, int p2, int p3) {
  /* Old interrupt flags */
  unsigned int rflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (rflags));

  /*
   * Get the user-space interrupt context.
   */
  /* User Context Pointer */
  sva_icontext_t * ep = get_uicontext();

  do {
    /*
     * Check the memory.
     */
    sva_check_memory_write (ep, sizeof (sva_icontext_t));
    sva_check_memory_write (ep->rsp, sizeof (uintptr_t));

    /*
     * Verify that this interrupt context has a stack pointer.
     */
#if 0
    if (sva_is_privileged () && sva_was_privileged(icontext)) {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }
#endif
    break;
  } while (1);

  /*
   * Place the arguments into the proper registers.
   */
  ep->rdi = p1;
  ep->rsi = p2;
  ep->rdx = p3;

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ep->rsp)) = ep->rip;

  /*
   * Set the return function to be the specificed function.
   */
  ep->rip = (unsigned int)newf;

  /*
   * Re-enable interrupts.
   */
  if (rflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*****************************************************************************
 * Integer State
 ****************************************************************************/

#if 0
/*
 * Intrinsic: sva_push_function1 ()
 *
 * Description:
 *  This intrinsic modifies the saved integer state so that the specified
 *  function was called with the given arguments.
 *
 * Inputs:
 *  integer - A pointer to the exception handler saved state.
 *  newf    - The function to call.
 *  param   - The parameter to send to the function.
 *
 * TODO:
 *  This currently only takes a function that takes a single integer
 *  argument.  Eventually, this should take any function.
 *
 * NOTES:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 */
void
sva_push_function1 (void * integer, void (*newf)(int), int param)
{
  /* User Context Pointer */
  sva_integer_state_t * ep = integer;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

#ifdef SC_INTRINCHECKS
  extern MetaPoolTy IntegerStatePool;
  struct node {
    void* left;
    void* right;
    char* key;
    char* end;
    void* tag;
  };
  struct node * np;
  unsigned long start;

  /*
   * Verify that the memory was part of a previous integer state.
   */
  np = getBounds (&IntegerStatePool, integer);
  start = np->key;
  pchk_drop_obj (&IntegerStatePool, integer);
  if (start != integer)
    poolcheckfail ("Integer Check Failure", (unsigned)integer, (void*)__builtin_return_address(0));
#endif

  /*
   * Check the memory.
   */
  sva_check_memory_write (ep, sizeof (sva_integer_state_t));
  sva_check_memory_write (ep->rsp, sizeof (unsigned int) * 2);

  /* Performance counters */
#if LLVA_COUNTERS
  ++sva_counters.sva_push_func;
  if (sva_debug) ++sva_local_counters.sva_push_func;
  sc_intrinsics[current_sysnum] |= MASK_LLVA_PUSH_FUNCTION;
#endif

  /*
   * Push the one argument on to the user space stack.
   */
  *(--(ep->rsp)) = param;

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ep->rsp)) = ep->rip;

  /*
   * Set the return function to be the specificed function.
   */
  ep->rip = (unsigned int)newf;

  /*
   * Disable restrictions on system calls since we don't know where this
   * function pointer came from.
   */
#if LLVA_SCLIMIT
  ep->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_push_syscall ()
 *
 * Description:
 *  Modify the given LLVA state so that it appears that the specified system
 *  call has been invoked.
 */
extern void sc_ret(void);

void
sva_push_syscall (unsigned int sysnum, void * integerp, void * func)
{
  /* User Context Pointer */
  sva_integer_state_t * ep = integerp;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

#ifdef SC_INTRINCHECKS
  extern MetaPoolTy IntegerStatePool;
  struct node {
    void* left;
    void* right;
    char* key;
    char* end;
    void* tag;
  };
  struct node * np;
  unsigned long start;

  /*
   * Verify that the memory was part of a previous integer state.
   */
  np = getBounds (&IntegerStatePool, integerp);
  start = np->key;
  pchk_drop_obj (&IntegerStatePool, integerp);
  if (start != integerp)
    poolcheckfail ("Integer Check Failure", (unsigned)integerp, (void*)__builtin_return_address(0));
#endif

  /*
   * Check the memory.
   */
  sva_check_memory_write (ep,          sizeof (sva_integer_state_t));
  sva_check_memory_write (ep->rsp - 8, sizeof (unsigned int) * 8);

  /* Performance counters */
#if LLVA_COUNTERS
  ++sva_counters.sva_push_syscall;
  if (sva_debug) ++sva_local_counters.sva_push_syscall;
  sc_intrinsics[current_sysnum] |= MASK_LLVA_PUSH_SYSCALL;
#endif

  /*
   * Adjust the stack to hold the six parameters and a context pointer.
   */
  *(--(ep->rsp)) = (ep->rsp) + 1;
  *(--(ep->rsp)) = 0x00000006;
  *(--(ep->rsp)) = 0x00000005;
  *(--(ep->rsp)) = 0x00000004;
  *(--(ep->rsp)) = 0x00000003;
  *(--(ep->rsp)) = 0x00000002;
  *(--(ep->rsp)) = 0x00000001;

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ep->rsp)) = sc_ret;

  /*
   * Set the return function to be the specified function.
   */
  ep->rip = (unsigned int) (func);

  /*
   * Disable restrictions on system calls since we don't know where this
   * function pointer came from.
   */
#if LLVA_SCLIMIT
  ep->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_get_integer_stackp ()
 *
 * Description:
 *  Return the stack pointer that is saved within the current integer state.
 */
unsigned char *
sva_get_integer_stackp (void * except_state)
{
#ifdef SC_INTRINCHECKS
  extern MetaPoolTy IntegerStatePool;
  struct node {
    void* left;
    void* right;
    char* key;
    char* end;
    void* tag;
  };
  struct node * np;
  unsigned long start;

  /*
   * Verify that the memory was part of a previous integer state.
   */
  np = getBounds (&IntegerStatePool, except_state);
  start = np->key;
  pchk_drop_obj (&IntegerStatePool, except_state);
  if (start != except_state)
    poolcheckfail ("Integer Check Failure", (unsigned)except_state, (void*)__builtin_return_address(0));
#endif

  return (((sva_integer_state_t *)except_state)->rsp);
}

/*
 * Intrinsic: sva_set_integer_stackp ()
 *
 * Description:
 *  Take the specified pointer and make it the stack pointer used in the
 *  specified integer state.
 */
void
sva_set_integer_stackp (sva_integer_state_t * intp, sva_sp_t p)
{
#ifdef SC_INTRINCHECKS
  extern MetaPoolTy IntegerStatePool;
  struct node {
    void* left;
    void* right;
    char* key;
    char* end;
    void* tag;
  };
  struct node * np;
  unsigned long start;

  /*
   * Verify that the memory was part of a previous integer state.
   */
  np = getBounds (&IntegerStatePool, intp);
  start = np->key;
  pchk_drop_obj (&IntegerStatePool, intp);
  if (start != intp)
    poolcheckfail ("Integer Check Failure", (unsigned)intp, (void*)__builtin_return_address(0));
#endif

  intp->rsp = (void *) (p);
  return;
}

/*
 * Intrinsic: sva_load_integer ()
 *
 * Description:
 *  Load the integer processor state from the specified buffer.
 *
 * TODO:
 *  The checking code must also verify that there is enough stack space before
 *  proceeding.  Otherwise, state could get really messy.
 */
void
sva_load_integer (void * buffer)
{
  /* Integer State Pointer */
  sva_integer_state_t * p = buffer;

  /* Current code segment */
  unsigned int cs;

  /* Data segment to use for this privilege level */
  unsigned int ds = 0x18;

  /* Flags whether the input has been validated */
  unsigned int validated = 0;

  /* System call disable mask */
  extern unsigned int sva_sys_disabled;

  /* Disable interrupts */
  __asm__ __volatile__ ("cli");

  do
  {
#ifdef SC_INTRINCHECKS
    extern MetaPoolTy IntegerStatePool;
    struct node {
      void* left;
      void* right;
      char* key;
      char* end;
      void* tag;
    };
    struct node * np;
    unsigned long start;

    /*
     * Verify that the memory was part of a previous integer state.
     */
    np = getBounds (&IntegerStatePool, buffer);
    start = np->key;
    pchk_drop_obj (&IntegerStatePool, buffer);
    if (start != buffer)
      poolcheckfail ("Integer Check Failure", (unsigned)buffer, (void*)__builtin_return_address(0));
#endif

    /*
     * Verify that we won't fault if we read from the buffer.
     */
    sva_check_memory_read (buffer, sizeof (sva_integer_state_t));

    /*
     * Verify that we can access the stack pointed to inside the buffer.
     */
    sva_check_memory_write ((p->rsp) - 2, 8);

    /*
     * Grab the current code segment.
     */
    __asm__ __volatile__ ("movl %%cs, %0\n" : "=r" (cs));
    cs &= 0xffff;

    /*
     * If we're not operating at the same privilege level as when this state
     * buffer was saved, then generate an exception.
     */
    if (cs != (p->cs))
    {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }

    /*
     * Configure the data segment to match the code segment, in case it somehow
     * became corrupted.
     */
    ds = ((cs == 0x10) ? (0x18) : (0x2B));

    /*
     * Validation is finished.  Continue.
     */
    validated = 1;
  } while (!validated);

#if LLVA_COUNTERS
  ++sva_counters.sva_load_int;
  if (sva_debug) ++sva_local_counters.sva_load_int;
  sc_intrinsics[current_sysnum] |= MASK_LLVA_LOAD_INT;
#endif

  /*
   * Restore the system call disable mask.
   */
#if LLVA_SCLIMIT
  sva_sys_disabled = p->sc_disabled;
#endif

  /*
   * Restore the state to the processor.
                        "movl 44(%%eax), %%gs\n" 
   */
  __asm__ __volatile__ (
                        /* Get the pointer to the buffer */
                        "movl %0, %%eax\n"

                        /* Conditionally restore the integer state */
                        "movl   4(%%eax), %%ebx\n"
                        "movl   8(%%eax), %%edi\n"
                        "movl  12(%%eax), %%esi\n"

                        /* Restore the extra segment registers */
                        "movw %1, %%es\n" 
                        "movw %1, %%ds\n" 

                        /* Restore ecx and edx */
                        "movl 24(%%eax), %%edx\n" 
                        "movl 28(%%eax), %%ecx\n" 

#if 0
                        /* Restore the code and stack segment registers */
                        "movl 60(%%eax), %%cs\n" 
                        "movl 72(%%eax), %%ss\n" 
#endif

                        /* Restore the ebp register */
                        "movl 36(%%eax), %%ebp\n"

                        /* Switch to the new stack pointer */
                        "movl 68(%%eax), %%rsp\n"

                        /* Put the new return address on the new stack */
                        "pushl 56(%%eax)\n"

                        /* Put the EFLAGS register  on the new stack */
                        "pushl 64(%%eax)\n"

#if 0
                        /* Restore eax */
                        "movl 32(%%eax), %%eax\n"
#else
                        /* Set the return value to 1 */
                        "movl $1, %%eax\n"
#endif

                        /* Restore EFLAGS, potentially re-enabling interrupts */
                        "popfl\n"

                        /* Return */
                        "ret\n" :: "m" (buffer), "m" (ds));

  return;
}

/*
 * Intrinsic: sva_swap_integer()
 *
 * Description:
 *  This intrinsic saves the current integer state and swaps in a new one.
 *
 * Inputs:
 *  newint - The new integer state to load on to the processor.
 *  statep - A pointer to a memory location in which to store the ID of the
 *           state that this invocation of sva_swap_integer() will save.
 *
 * Return value:
 *  0 - State swapping failed.
 *  1 - State swapping succeeded.
 */
unsigned
sva_swap_integer (unsigned int newint, unsigned int * statep) {
  /*
   * Current state held on CPU: We allocate it here so that the caller cannot
   * free it and cause a dangling pointer to an integer state.
   */
  unsigned int old[24];

  /* Get a pointer to the saved state (the ID is the pointer) */
  void * new = (void *) (newint);

  /*
   * Determine whether the integer state is valid.
   */
#if SVA_CHECK_INTEGER
  if ((pchk_check_int (new)) == 0) {
    poolcheckfail ("sva_swap_integer: Bad integer state", (unsigned)old, (void*)__builtin_return_address(0));
    return 0;
  }
#endif

  /*
   * Save the current integer state.
   */
  if (sva_save_integer (old)) {
    /*
     * We've awakened.
     */
#if SVA_CHECK_INTEGER
    /*
     * Mark the integer state invalid and return to the caller.
     */
    pchk_drop_int (old);

    /*
     * Determine what stack we're running on now.
     */
    pchk_update_stack ();
#endif
    return 1;
  }

  /*
   * Register the saved integer state in the splay tree.
   */
#if SVA_CHECK_INTEGER
  pchk_reg_int (old);
#endif

  /*
   * Inform the caller of the location of the last state saved.
   */
  *statep = (unsigned int) old;

  /*
   * Now, reload the integer state pointed to by new.
   */
  sva_load_integer (new);

  /*
   * The loading of integer state failed.
   */
#if SVA_CHECK_INTEGER
  pchk_drop_int (old);
#endif
  return 0;
}

unsigned char
sva_is_privileged  (void)
{
  unsigned int cs;
  __asm__ __volatile__ ("movl %%cs, %0\n" : "=r" (cs));
  return ((cs & 0x10) == 0x10);
}

/*
 * Intrinsic: sva_ialloca()
 *
 * Description:
 *  Allocate space on the current stack frame for an object of the specified
 *  size.
 */
void *
sva_ialloca (void * icontext, unsigned int size)
{
  sva_icontext_t * p = icontext;

  /*
   * Verify that this interrupt context has a stack pointer.
   */
  while (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }

  /*
   * Perform the alloca.
   */
  return (p->rsp -= ((size / 4) + 1));
}

/*
 * Intrinsic: sva_load_icontext()
 *
 * Description:
 *  This intrinsic takes state saved by the Execution Engine during an
 *  interrupt and loads it into the specified interrupt context buffer.
 */
void
sva_load_icontext (sva_icontext_t * icontextp, sva_integer_state_t * statep)
{
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  do
  {
#ifdef SC_INTRINCHECKS
    extern MetaPoolTy IntegerStatePool;
    struct node {
      void* left;
      void* right;
      char* key;
      char* end;
      void* tag;
    };
    struct node * np;
    unsigned long start;

    /*
     * Verify that the memory was part of a previous integer state.
     */
    np = getBounds (&IntegerStatePool, statep);
    start = np->key;
    pchk_drop_obj (&IntegerStatePool, statep);
    if (start != statep)
      poolcheckfail ("Integer Check Failure", (unsigned)statep, (void*)__builtin_return_address(0));
#endif

    sva_check_memory_read (statep, sizeof (sva_icontext_t));
    sva_check_memory_write (icontextp, sizeof (sva_icontext_t));

    /*
     * Verify that this interrupt context has a stack pointer.
     */
    if (sva_is_privileged () && sva_was_privileged(icontextp))
    {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }
    break;
  } while (1);

  /*
   * Currently, the interrupt context and integer state are one to one
   * identical.  This means that they can just be copied over.
   */
  __builtin_memcpy (icontextp, statep, sizeof (sva_icontext_t));

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_save_icontext()
 *
 * Description:
 *  This intrinsic takes state saved by the Execution Engine during an
 *  interrupt and saves it as an integer state structure.
 */
void
sva_save_icontext (sva_icontext_t * icontextp, sva_integer_state_t * statep)
{
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  do
  {

    sva_check_memory_read  (icontextp, sizeof (sva_icontext_t));
    sva_check_memory_write (statep,    sizeof (sva_icontext_t));

    /*
     * Verify that this interrupt context has a stack pointer.
     */
    if (sva_is_privileged () && sva_was_privileged(icontextp))
    {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }
    break;
  } while (1);

  /*
   * Currently, the interrupt context and integer state are one to one
   * identical.  This means that they can just be copied over.
   */
  __builtin_memcpy (statep, icontextp, sizeof (sva_icontext_t));

#ifdef SC_INTRINCHECKS
  extern MetaPoolTy IntegerStatePool;
  struct node {
    void* left;
    void* right;
    char* key;
    char* end;
    void* tag;
  };
  struct node * np;
  unsigned long start;

  /*
   * Register the integer state with the Execution Engine's memory safety code.
   */
  pchk_reg_obj (&IntegerStatePool, statep, 72);
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_load_fp()
 *
 * Description:
 *  This intrinsic loads floating point state back on to the processor.
 */
void
sva_load_fp (void * buffer)
{
  const int ts = 0x00000008;
  unsigned int cr0;
  sva_fp_state_t * p = buffer;
  extern unsigned char sva_fp_used;
 
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

#if LLVA_COUNTERS
  ++sva_counters.sva_load_fp;
  if (sva_debug) ++sva_local_counters.sva_load_fp;
  sc_intrinsics[current_sysnum] |= MASK_LLVA_LOAD_FP;
#endif

  /*
   * Save the state of the floating point unit.
   */
  __asm__ __volatile__ ("frstor %0" : "=m" (p->state));

  /*
   * Mark the FPU has having been unused.  The first FP operation will cause
   * an exception into the Execution Engine.
   */
  __asm__ __volatile__ ("movl %%cr0, %0\n"
                        "orl  %1,    %0\n"
                        "movl %0,    %%cr0\n" : "=&r" (cr0) : "r" ((ts)));
  sva_fp_used = 0;

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_save_fp()
 *
 * Description:
 *  Save the processor's current floating point state into the specified
 *  buffer.
 *
 * Inputs:
 *  buffer - A pointer to the buffer in which to save the data.
 *  always - Only save state if it was modified since the last load FP state.
 */
int
sva_save_fp (void * buffer, int always)
{
  sva_fp_state_t * p = buffer;
  extern unsigned char sva_fp_used;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  if (always || sva_fp_used)
  {
#if LLVA_COUNTERS
    ++sva_counters.sva_save_fp;
    if (sva_debug) ++sva_local_counters.sva_save_fp;
    sc_intrinsics[current_sysnum] |= MASK_LLVA_SAVE_FP;
#endif
    __asm__ __volatile__ ("fnsave %0" : "=m" (p->state) :: "memory");

    /*
     * Re-enable interrupts.
     */
    if (eflags & 0x00000200)
      __asm__ __volatile__ ("sti":::"memory");
    return 1;
  }

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return 0;
}

/*
 * Intrinsic: sva_load_stackp ()
 *
 * Description:
 *  Load on to the processor the stack pointer specified.
 */
void
sva_load_stackp  (sva_sp_t p)
{
  __asm__ __volatile__ ("movl %0, %%rsp\n" :: "r" (p));
  return;
}

/*
 * Intrinsic: sva_load_invoke()
 *
 * Description:
 *  Set the top of the invoke stack.
 */
void
sva_load_invoke (void * p)
{
  extern struct invoke_frame * gip;
  gip = p;
  return;
}

/*
 * Intrinsic: sva_save_invoke()
 *
 * Description:
 *  Save the current value of the top of the invoke stack.
 */
void *
sva_save_invoke (void)
{
  extern struct invoke_frame * gip;
  return gip;
}

unsigned int
sva_icontext_load_retvalue (void * icontext)
{
  return (((sva_icontext_t *)(icontext))->eax);
}

void
sva_icontext_save_retvalue (void * icontext, unsigned int value)
{
  (((sva_icontext_t *)(icontext))->eax) = value;
}

/*
 * Intrinsic: sva_get_icontext_stackp ()
 *
 * Description:
 *  Return the stack pointer that is saved within the specified interrupt
 *  context structure.
 */
unsigned char *
sva_get_icontext_stackp (void * icontext)
{
  /*
   * Verify that this interrupt context has a stack pointer.
   */
  while (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }

  return (((sva_icontext_t *)icontext)->rsp);
}

/*
 * Intrinsic: sva_set_icontext_stackp ()
 *
 * Description:
 *  Sets the stack pointer that is saved within the specified interrupt
 *  context structure.
 */
void
sva_set_icontext_stackp (void * icontext, void * stackp)
{
  /*
   * Verify that this interrupt context has a stack pointer.
   */
#if 0
  if (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }
#endif

  (((sva_icontext_t *)icontext)->rsp) = stackp;
  return;
}

/*
 * Intrinsic: sva_iset_privileged ()
 *
 * Description:
 *  Change the state of the interrupt context to have come from an unprivileged
 *  state.
 */
void
sva_iset_privileged (void * icontext, unsigned char privileged)
{
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  sva_icontext_t * p = icontext;
  if (privileged)
  {
    p->cs = 0x10;
    p->ds = 0x18;
    p->es = 0x18;
  }
  else
  {
    p->cs = 0x23;
    p->ds = 0x2B;
    p->es = 0x2B;
    p->ss = 0x2B;
  }

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

long long
sva_save_tsc (void)
{
  long long tsc;

  __asm__ __volatile__ ("rdtsc\n" : "=A" (tsc));
  return tsc;
}

void
sva_load_tsc (long long tsc)
{
  __asm__ __volatile__ ("wrmsr\n" :: "A" (tsc), "c" (0x10));
}

/*
 * Intrinsic: sva_init_stack()
 *
 * Description:
 *  Pointer to the integer state identifier used for context switching.
 *
 * Inputs:
 *  stackp - A pointer to the *beginning* of the kernel stack.
 *
 * Return value:
 *  An identifier that can be passed to sva_swap_integer() to begin execution
 *  of the thread.
 */
unsigned int
sva_init_stack (unsigned char * start_stackp,
                unsigned length,
                void * orig_icontextp,
                void * func,
                unsigned int arg) {
  /* Working memory pointer */
  sva_icontext_t * icontextp;

  /* Working integer state */
  sva_integer_state_t * integerp;

  /* Arguments allocated on the new stack */
  struct frame {
    /* Return pointer for the function frame */
    void * return_rip;

    /* The arguments to the function */
    unsigned long args[6];

    /* The pointer to the interrupt context */
    void * icontextp;
  } * args;

  /* Old interrupt flags */
  unsigned int eflags;

  /* End of stack */
  unsigned char * stackp = 0;

  /* Length of Stack */
  unsigned int stacklen = 0;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  /*
   * Verify that the given stack is, in fact, a declared stack.
   */
#ifdef SVA_KSTACKS
  if (start_stackp = pchk_checkstack (start_stackp, &stacklen)) {
      stackp = start_stackp + stacklen;
  } else {
    return 0;
  }
#else
  stacklen = length;
  stackp = start_stackp + stacklen;
#endif

  /*
   * Verify that the stack is big enough.
   */
  if (stacklen < sizeof (sva_integer_state_t) +
                 sizeof (sva_icontext_t) + 
                 sizeof (struct frame)) {
    return 0;
  }

  /*
   * Allocate an integer state at the top of the stack before anything else.
   */
  stackp -= sizeof (sva_integer_state_t);
  integerp = stackp;

  /*
   * Allocate an interrupt context just after the integer state on the stack.
   */
  stackp -= sizeof (sva_icontext_t);
  icontextp = stackp;

  /*
   * Allocate the call frame for the call to the system call.
   */
  stackp -= sizeof (struct frame);
  args = stackp;

  /*
   * FIXME: Need to redo for this intrinsic.
   *
   * Verify that the memory has the proper access.
   */
  sva_check_memory_read  (orig_icontextp, sizeof (sva_icontext_t));
  sva_check_memory_write (icontextp,      sizeof (sva_icontext_t));

  /*
   * Initialize the arguments to the system call.  Also setup the interrupt
   * context and return function pointer.
   */
  args->icontextp = icontextp;
  args->return_rip = sc_ret;
  args->args[0] = arg;
  args->args[1] = 0;
  args->args[2] = 0;
  args->args[3] = 0;
  args->args[4] = 0;
  args->args[5] = 0;

  /*
   * Initialize the interrupt context by copying the old interrupt context into
   * the new one on the stack.
   */
  __builtin_memcpy (icontextp, orig_icontextp, sizeof (sva_icontext_t));

  /*
   * Disable restrictions on system calls since we don't know from where this
   * function pointer came.
   */
#if LLVA_SCLIMIT
  icontextp->sc_disabled = 0;
#endif

  /*
   * Initialze the integer state of the new thread of control.
   */
  bzero (integerp, sizeof (sva_integer_state_t));
  integerp->rip = func;
  integerp->rsp = stackp;
  integerp->cs  = 0x10;

#ifdef SVA_CHECK_INTEGER
  /*
   * Mark the integer state as valid.
   */
  pchk_reg_int (integerp);
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return (unsigned int) integerp;
}
#endif