/*
    ChibiOS - Copyright (C) 2006..2017 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"
#include "ch_test.h"
#include "test_root.h"

/**
 * @file    test_sequence_013.c
 * @brief   Test Sequence 013 code.
 *
 * @page test_sequence_013 [13] Benchmarks
 *
 * File: @ref test_sequence_013.c
 *
 * <h2>Description</h2>
 * This module implements a series of system benchmarks. The benchmarks
 * are useful as a stress test and as a reference when comparing
 * ChibiOS/RT with similar systems.<br> Objective of the test sequence
 * is to provide a performance index for the most critical system
 * subsystems. The performance numbers allow to discover performance
 * regressions between successive ChibiOS/RT releases.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_013_001
 * - @subpage test_013_002
 * - @subpage test_013_003
 * - @subpage test_013_004
 * - @subpage test_013_005
 * - @subpage test_013_006
 * - @subpage test_013_007
 * - @subpage test_013_008
 * - @subpage test_013_009
 * - @subpage test_013_010
 * - @subpage test_013_011
 * - @subpage test_013_012
 * .
 */

/****************************************************************************
 * Shared code.
 ****************************************************************************/

#if CH_CFG_USE_SEMAPHORES || defined(__DOXYGEN__)
static semaphore_t sem1;
#endif
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
static mutex_t mtx1;
#endif

static void tmo(void *param) {(void)param;}

#if CH_CFG_USE_MESSAGES
static THD_FUNCTION(bmk_thread1, p) {
  thread_t *tp;
  msg_t msg;

  (void)p;
  do {
    tp = chMsgWait();
    msg = chMsgGet(tp);
    chMsgRelease(tp, msg);
  } while (msg);
}

NOINLINE static unsigned int msg_loop_test(thread_t *tp) {
  systime_t start, end;

  uint32_t n = 0;
  start = test_wait_tick();
  end = start + MS2ST(1000);
  do {
    (void)chMsgSend(tp, 1);
    n++;
#if defined(SIMULATOR)
    _sim_check_for_interrupts();
#endif
  } while (chVTIsSystemTimeWithinX(start, end));
  (void)chMsgSend(tp, 0);
  return n;
}
#endif

static THD_FUNCTION(bmk_thread3, p) {

  chThdExit((msg_t)p);
}

static THD_FUNCTION(bmk_thread4, p) {
  msg_t msg;
  thread_t *self = chThdGetSelfX();

  (void)p;
  chSysLock();
  do {
    chSchGoSleepS(CH_STATE_SUSPENDED);
    msg = self->u.rdymsg;
  } while (msg == MSG_OK);
  chSysUnlock();
}

#if CH_CFG_USE_SEMAPHORES
static THD_FUNCTION(bmk_thread7, p) {

  (void)p;
  while (!chThdShouldTerminateX())
    chSemWait(&sem1);
}
#endif

static THD_FUNCTION(bmk_thread8, p) {

  do {
    chThdYield();
    chThdYield();
    chThdYield();
    chThdYield();
    (*(uint32_t *)p) += 4;
#if defined(SIMULATOR)
    _sim_check_for_interrupts();
#endif
  } while(!chThdShouldTerminateX());
}

/****************************************************************************
 * Test cases.
 ****************************************************************************/

#if (CH_CFG_USE_MESSAGES) || defined(__DOXYGEN__)
/**
 * @page test_013_001 [13.1] Messages performance #1
 *
 * <h2>Description</h2>
 * A message server thread is created with a lower priority than the
 * client thread, the messages throughput per second is measured and
 * the result printed on the output log.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MESSAGES
 * .
 *
 * <h2>Test Steps</h2>
 * - [13.1.1] The messenger thread is started at a lower priority than
 *   the current thread.
 * - [13.1.2] The number of messages exchanged is counted in a one
 *   second time window.
 * - [13.1.3] Score is printed.
 * .
 */

static void test_013_001_execute(void) {
  uint32_t n;

  /* [13.1.1] The messenger thread is started at a lower priority than
     the current thread.*/
  test_set_step(1);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-1, bmk_thread1, NULL);
  }

  /* [13.1.2] The number of messages exchanged is counted in a one
     second time window.*/
  test_set_step(2);
  {
    n = msg_loop_test(threads[0]);
    test_wait_threads();
  }

  /* [13.1.3] Score is printed.*/
  test_set_step(3);
  {
    test_print("--- Score : ");
    test_printn(n);
    test_print(" msgs/S, ");
    test_printn(n << 1);
    test_println(" ctxswc/S");
  }
}

static const testcase_t test_013_001 = {
  "Messages performance #1",
  NULL,
  NULL,
  test_013_001_execute
};
#endif /* CH_CFG_USE_MESSAGES */

#if (CH_CFG_USE_MESSAGES) || defined(__DOXYGEN__)
/**
 * @page test_013_002 [13.2] Messages performance #2
 *
 * <h2>Description</h2>
 * A message server thread is created with an higher priority than the
 * client thread, the messages throughput per second is measured and
 * the result printed on the output log.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MESSAGES
 * .
 *
 * <h2>Test Steps</h2>
 * - [13.2.1] The messenger thread is started at an higher priority
 *   than the current thread.
 * - [13.2.2] The number of messages exchanged is counted in a one
 *   second time window.
 * - [13.2.3] Score is printed.
 * .
 */

static void test_013_002_execute(void) {
  uint32_t n;

  /* [13.2.1] The messenger thread is started at an higher priority
     than the current thread.*/
  test_set_step(1);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()+1, bmk_thread1, NULL);
  }

  /* [13.2.2] The number of messages exchanged is counted in a one
     second time window.*/
  test_set_step(2);
  {
    n = msg_loop_test(threads[0]);
    test_wait_threads();
  }

  /* [13.2.3] Score is printed.*/
  test_set_step(3);
  {
    test_print("--- Score : ");
    test_printn(n);
    test_print(" msgs/S, ");
    test_printn(n << 1);
    test_println(" ctxswc/S");
  }
}

static const testcase_t test_013_002 = {
  "Messages performance #2",
  NULL,
  NULL,
  test_013_002_execute
};
#endif /* CH_CFG_USE_MESSAGES */

#if (CH_CFG_USE_MESSAGES) || defined(__DOXYGEN__)
/**
 * @page test_013_003 [13.3] Messages performance #3
 *
 * <h2>Description</h2>
 * A message server thread is created with an higher priority than the
 * client thread, four lower priority threads crowd the ready list, the
 * messages throughput per second is measured while the ready list and
 * the result printed on the output log.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MESSAGES
 * .
 *
 * <h2>Test Steps</h2>
 * - [13.3.1] The messenger thread is started at an higher priority
 *   than the current thread.
 * - [13.3.2] Four threads are started at a lower priority than the
 *   current thread.
 * - [13.3.3] The number of messages exchanged is counted in a one
 *   second time window.
 * - [13.3.4] Score is printed.
 * .
 */

static void test_013_003_execute(void) {
  uint32_t n;

  /* [13.3.1] The messenger thread is started at an higher priority
     than the current thread.*/
  test_set_step(1);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()+1, bmk_thread1, NULL);
  }

  /* [13.3.2] Four threads are started at a lower priority than the
     current thread.*/
  test_set_step(2);
  {
    threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriorityX()-2, bmk_thread3, NULL);
    threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriorityX()-3, bmk_thread3, NULL);
    threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriorityX()-4, bmk_thread3, NULL);
    threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriorityX()-5, bmk_thread3, NULL);
  }

  /* [13.3.3] The number of messages exchanged is counted in a one
     second time window.*/
  test_set_step(3);
  {
    n = msg_loop_test(threads[0]);
    test_wait_threads();
  }

  /* [13.3.4] Score is printed.*/
  test_set_step(4);
  {
    test_print("--- Score : ");
    test_printn(n);
    test_print(" msgs/S, ");
    test_printn(n << 1);
    test_println(" ctxswc/S");
  }
}

static const testcase_t test_013_003 = {
  "Messages performance #3",
  NULL,
  NULL,
  test_013_003_execute
};
#endif /* CH_CFG_USE_MESSAGES */

/**
 * @page test_013_004 [13.4] Context Switch performance
 *
 * <h2>Description</h2>
 * A thread is created that just performs a @p chSchGoSleepS() into a
 * loop, the thread is awakened as fast is possible by the tester
 * thread.<br> The Context Switch performance is calculated by
 * measuring the number of iterations after a second of continuous
 * operations.
 *
 * <h2>Test Steps</h2>
 * - [13.4.1] Starting the target thread at an higher priority level.
 * - [13.4.2] Waking up the thread as fast as possible in a one second
 *   time window.
 * - [13.4.3] Stopping the target thread.
 * - [13.4.4] Score is printed.
 * .
 */

static void test_013_004_execute(void) {
  thread_t *tp;
  uint32_t n;

  /* [13.4.1] Starting the target thread at an higher priority level.*/
  test_set_step(1);
  {
    tp = threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()+1,
                                        bmk_thread4, NULL);
  }

  /* [13.4.2] Waking up the thread as fast as possible in a one second
     time window.*/
  test_set_step(2);
  {
    systime_t start, end;

    n = 0;
    start = test_wait_tick();
    end = start + MS2ST(1000);
    do {
      chSysLock();
      chSchWakeupS(tp, MSG_OK);
      chSchWakeupS(tp, MSG_OK);
      chSchWakeupS(tp, MSG_OK);
      chSchWakeupS(tp, MSG_OK);
      chSysUnlock();
      n += 4;
#if defined(SIMULATOR)
      _sim_check_for_interrupts();
#endif
    } while (chVTIsSystemTimeWithinX(start, end));
  }

  /* [13.4.3] Stopping the target thread.*/
  test_set_step(3);
  {
    chSysLock();
    chSchWakeupS(tp, MSG_TIMEOUT);
    chSysUnlock();
    test_wait_threads();
  }

  /* [13.4.4] Score is printed.*/
  test_set_step(4);
  {
    test_print("--- Score : ");
    test_printn(n * 2);
    test_println(" ctxswc/S");
  }
}

static const testcase_t test_013_004 = {
  "Context Switch performance",
  NULL,
  NULL,
  test_013_004_execute
};

/**
 * @page test_013_005 [13.5] Threads performance, full cycle
 *
 * <h2>Description</h2>
 * Threads are continuously created and terminated into a loop. A full
 * chThdCreateStatic() / @p chThdExit() / @p chThdWait() cycle is
 * performed in each iteration.<br> The performance is calculated by
 * measuring the number of iterations after a second of continuous
 * operations.
 *
 * <h2>Test Steps</h2>
 * - [13.5.1] A thread is created at a lower priority level and its
 *   termination detected using @p chThdWait(). The operation is
 *   repeated continuously in a one-second time window.
 * - [13.5.2] Score is printed.
 * .
 */

static void test_013_005_execute(void) {
  uint32_t n;
  tprio_t prio = chThdGetPriorityX() - 1;
  systime_t start, end;

  /* [13.5.1] A thread is created at a lower priority level and its
     termination detected using @p chThdWait(). The operation is
     repeated continuously in a one-second time window.*/
  test_set_step(1);
  {
    n = 0;
    start = test_wait_tick();
    end = start + MS2ST(1000);
    do {
      chThdWait(chThdCreateStatic(wa[0], WA_SIZE, prio, bmk_thread3, NULL));
      n++;
#if defined(SIMULATOR)
      _sim_check_for_interrupts();
#endif
    } while (chVTIsSystemTimeWithinX(start, end));
  }

  /* [13.5.2] Score is printed.*/
  test_set_step(2);
  {
    test_print("--- Score : ");
    test_printn(n);
    test_println(" threads/S");
  }
}

static const testcase_t test_013_005 = {
  "Threads performance, full cycle",
  NULL,
  NULL,
  test_013_005_execute
};

/**
 * @page test_013_006 [13.6] Threads performance, create/exit only
 *
 * <h2>Description</h2>
 * Threads are continuously created and terminated into a loop. A
 * partial @p chThdCreateStatic() / @p chThdExit() cycle is performed
 * in each iteration, the @p chThdWait() is not necessary because the
 * thread is created at an higher priority so there is no need to wait
 * for it to terminate.<br> The performance is calculated by measuring
 * the number of iterations after a second of continuous operations.
 *
 * <h2>Test Steps</h2>
 * - [13.6.1] A thread is created at an higher priority level and let
 *   terminate immediately. The operation is repeated continuously in a
 *   one-second time window.
 * - [13.6.2] Score is printed.
 * .
 */

static void test_013_006_execute(void) {
  uint32_t n;
  tprio_t prio = chThdGetPriorityX() + 1;
  systime_t start, end;

  /* [13.6.1] A thread is created at an higher priority level and let
     terminate immediately. The operation is repeated continuously in a
     one-second time window.*/
  test_set_step(1);
  {
    n = 0;
    start = test_wait_tick();
    end = start + MS2ST(1000);
    do {
#if CH_CFG_USE_REGISTRY
      chThdRelease(chThdCreateStatic(wa[0], WA_SIZE, prio, bmk_thread3, NULL));
#else
      chThdCreateStatic(wa[0], WA_SIZE, prio, bmk_thread3, NULL);
#endif
      n++;
#if defined(SIMULATOR)
      _sim_check_for_interrupts();
#endif
    } while (chVTIsSystemTimeWithinX(start, end));
  }

  /* [13.6.2] Score is printed.*/
  test_set_step(2);
  {
    test_print("--- Score : ");
    test_printn(n);
    test_println(" threads/S");
  }
}

static const testcase_t test_013_006 = {
  "Threads performance, create/exit only",
  NULL,
  NULL,
  test_013_006_execute
};

#if (CH_CFG_USE_SEMAPHORES) || defined(__DOXYGEN__)
/**
 * @page test_013_007 [13.7] Mass reschedule performance
 *
 * <h2>Description</h2>
 * Five threads are created and atomically rescheduled by resetting the
 * semaphore where they are waiting on. The operation is performed into
 * a continuous loop.<br> The performance is calculated by measuring
 * the number of iterations after a second of continuous operations.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_SEMAPHORES
 * .
 *
 * <h2>Test Steps</h2>
 * - [13.7.1] Five threads are created at higher priority that
 *   immediately enqueue on a semaphore.
 * - [13.7.2] The semaphore is reset waking up the five threads. The
 *   operation is repeated continuously in a one-second time window.
 * - [13.7.3] The five threads are terminated.
 * - [13.7.4] The score is printed.
 * .
 */

static void test_013_007_setup(void) {
  chSemObjectInit(&sem1, 0);
}

static void test_013_007_execute(void) {
  uint32_t n;

  /* [13.7.1] Five threads are created at higher priority that
     immediately enqueue on a semaphore.*/
  test_set_step(1);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()+5, bmk_thread7, NULL);
    threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriorityX()+4, bmk_thread7, NULL);
    threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriorityX()+3, bmk_thread7, NULL);
    threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriorityX()+2, bmk_thread7, NULL);
    threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriorityX()+1, bmk_thread7, NULL);
  }

  /* [13.7.2] The semaphore is reset waking up the five threads. The
     operation is repeated continuously in a one-second time window.*/
  test_set_step(2);
  {
    systime_t start, end;

    n = 0;
    start = test_wait_tick();
    end = start + MS2ST(1000);
    do {
      chSemReset(&sem1, 0);
      n++;
#if defined(SIMULATOR)
      _sim_check_for_interrupts();
#endif
    } while (chVTIsSystemTimeWithinX(start, end));
  }

  /* [13.7.3] The five threads are terminated.*/
  test_set_step(3);
  {
    test_terminate_threads();
    chSemReset(&sem1, 0);
    test_wait_threads();
  }

  /* [13.7.4] The score is printed.*/
  test_set_step(4);
  {
    test_print("--- Score : ");
    test_printn(n);
    test_print(" reschedules/S, ");
    test_printn(n * 6);
    test_println(" ctxswc/S");
  }
}

static const testcase_t test_013_007 = {
  "Mass reschedule performance",
  test_013_007_setup,
  NULL,
  test_013_007_execute
};
#endif /* CH_CFG_USE_SEMAPHORES */

/**
 * @page test_013_008 [13.8] Round-Robin voluntary reschedule
 *
 * <h2>Description</h2>
 * Five threads are created at equal priority, each thread just
 * increases a variable and yields.<br> The performance is calculated
 * by measuring the number of iterations after a second of continuous
 * operations.
 *
 * <h2>Test Steps</h2>
 * - [13.8.1] The five threads are created at lower priority. The
 *   threds have equal priority and start calling @p chThdYield()
 *   continuously.
 * - [13.8.2] Waiting one second then terminating the 5 threads.
 * - [13.8.3] The score is printed.
 * .
 */

static void test_013_008_execute(void) {
  uint32_t n;

  /* [13.8.1] The five threads are created at lower priority. The
     threds have equal priority and start calling @p chThdYield()
     continuously.*/
  test_set_step(1);
  {
    n = 0;
    test_wait_tick();threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-1, bmk_thread8, (void *)&n);

    threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriorityX()-1, bmk_thread8, (void *)&n);
    threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriorityX()-1, bmk_thread8, (void *)&n);
    threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriorityX()-1, bmk_thread8, (void *)&n);
    threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriorityX()-1, bmk_thread8, (void *)&n);
  }

  /* [13.8.2] Waiting one second then terminating the 5 threads.*/
  test_set_step(2);
  {
    chThdSleepSeconds(1);
    test_terminate_threads();
    test_wait_threads();
  }

  /* [13.8.3] The score is printed.*/
  test_set_step(3);
  {
    test_print("--- Score : ");
    test_printn(n);
    test_println(" ctxswc/S");
  }
}

static const testcase_t test_013_008 = {
  "Round-Robin voluntary reschedule",
  NULL,
  NULL,
  test_013_008_execute
};

/**
 * @page test_013_009 [13.9] Virtual Timers set/reset performance
 *
 * <h2>Description</h2>
 * A virtual timer is set and immediately reset into a continuous
 * loop.<br> The performance is calculated by measuring the number of
 * iterations after a second of continuous operations.
 *
 * <h2>Test Steps</h2>
 * - [13.9.1] Two timers are set then reset without waiting for their
 *   counter to elapse. The operation is repeated continuously in a
 *   one-second time window.
 * - [13.9.2] The score is printed.
 * .
 */

static void test_013_009_execute(void) {
  static virtual_timer_t vt1, vt2;
  uint32_t n;

  /* [13.9.1] Two timers are set then reset without waiting for their
     counter to elapse. The operation is repeated continuously in a
     one-second time window.*/
  test_set_step(1);
  {
    systime_t start, end;

    n = 0;
    start = test_wait_tick();
    end = start + MS2ST(1000);
    do {
      chSysLock();
      chVTDoSetI(&vt1, 1, tmo, NULL);
      chVTDoSetI(&vt2, 10000, tmo, NULL);
      chVTDoResetI(&vt1);
      chVTDoResetI(&vt2);
      chSysUnlock();
      n++;
#if defined(SIMULATOR)
      _sim_check_for_interrupts();
#endif
    } while (chVTIsSystemTimeWithinX(start, end));
  }

  /* [13.9.2] The score is printed.*/
  test_set_step(2);
  {
    test_print("--- Score : ");
    test_printn(n * 2);
    test_println(" timers/S");
  }
}

static const testcase_t test_013_009 = {
  "Virtual Timers set/reset performance",
  NULL,
  NULL,
  test_013_009_execute
};

#if (CH_CFG_USE_SEMAPHORES) || defined(__DOXYGEN__)
/**
 * @page test_013_010 [13.10] Semaphores wait/signal performance
 *
 * <h2>Description</h2>
 * A counting semaphore is taken/released into a continuous loop, no
 * Context Switch happens because the counter is always non
 * negative.<br> The performance is calculated by measuring the number
 * of iterations after a second of continuous operations.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_SEMAPHORES
 * .
 *
 * <h2>Test Steps</h2>
 * - [13.10.1] A semaphore is teken and released. The operation is
 *   repeated continuously in a one-second time window.
 * - [13.10.2] The score is printed.
 * .
 */

static void test_013_010_setup(void) {
  chSemObjectInit(&sem1, 1);
}

static void test_013_010_execute(void) {
  uint32_t n;

  /* [13.10.1] A semaphore is teken and released. The operation is
     repeated continuously in a one-second time window.*/
  test_set_step(1);
  {
    systime_t start, end;

    n = 0;
    start = test_wait_tick();
    end = start + MS2ST(1000);
    do {
      chSemWait(&sem1);
      chSemSignal(&sem1);
      chSemWait(&sem1);
      chSemSignal(&sem1);
      chSemWait(&sem1);
      chSemSignal(&sem1);
      chSemWait(&sem1);
      chSemSignal(&sem1);
      n++;
#if defined(SIMULATOR)
      _sim_check_for_interrupts();
#endif
    } while (chVTIsSystemTimeWithinX(start, end));
  }

  /* [13.10.2] The score is printed.*/
  test_set_step(2);
  {
    test_print("--- Score : ");
    test_printn(n * 4);
    test_println(" wait+signal/S");
  }
}

static const testcase_t test_013_010 = {
  "Semaphores wait/signal performance",
  test_013_010_setup,
  NULL,
  test_013_010_execute
};
#endif /* CH_CFG_USE_SEMAPHORES */

#if (CH_CFG_USE_MUTEXES) || defined(__DOXYGEN__)
/**
 * @page test_013_011 [13.11] Mutexes lock/unlock performance
 *
 * <h2>Description</h2>
 * A mutex is locked/unlocked into a continuous loop, no Context Switch
 * happens because there are no other threads asking for the mutex.<br>
 * The performance is calculated by measuring the number of iterations
 * after a second of continuous operations.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MUTEXES
 * .
 *
 * <h2>Test Steps</h2>
 * - [13.11.1] A mutex is locked and unlocked. The operation is
 *   repeated continuously in a one-second time window.
 * - [13.11.2] The score is printed.
 * .
 */

static void test_013_011_setup(void) {
  chMtxObjectInit(&mtx1);
}

static void test_013_011_execute(void) {
  uint32_t n;

  /* [13.11.1] A mutex is locked and unlocked. The operation is
     repeated continuously in a one-second time window.*/
  test_set_step(1);
  {
    systime_t start, end;

    n = 0;
    start = test_wait_tick();
    end = start + MS2ST(1000);
    do {
      chMtxLock(&mtx1);
      chMtxUnlock(&mtx1);
      chMtxLock(&mtx1);
      chMtxUnlock(&mtx1);
      chMtxLock(&mtx1);
      chMtxUnlock(&mtx1);
      chMtxLock(&mtx1);
      chMtxUnlock(&mtx1);
      n++;
#if defined(SIMULATOR)
      _sim_check_for_interrupts();
#endif
    } while (chVTIsSystemTimeWithinX(start, end));
  }

  /* [13.11.2] The score is printed.*/
  test_set_step(2);
  {
    test_print("--- Score : ");
    test_printn(n * 4);
    test_println(" lock+unlock/S");
  }
}

static const testcase_t test_013_011 = {
  "Mutexes lock/unlock performance",
  test_013_011_setup,
  NULL,
  test_013_011_execute
};
#endif /* CH_CFG_USE_MUTEXES */

/**
 * @page test_013_012 [13.12] RAM Footprint
 *
 * <h2>Description</h2>
 * The memory size of the various kernel objects is printed.
 *
 * <h2>Test Steps</h2>
 * - [13.12.1] The size of the system area is printed.
 * - [13.12.2] The size of a thread structure is printed.
 * - [13.12.3] The size of a virtual timer structure is printed.
 * - [13.12.4] The size of a semaphore structure is printed.
 * - [13.12.5] The size of a mutex is printed.
 * - [13.12.6] The size of a condition variable is printed.
 * - [13.12.7] The size of an event source is printed.
 * - [13.12.8] The size of an event listener is printed.
 * - [13.12.9] The size of a mailbox is printed.
 * .
 */

static void test_013_012_execute(void) {

  /* [13.12.1] The size of the system area is printed.*/
  test_set_step(1);
  {
    test_print("--- System: ");
    test_printn(sizeof(ch_system_t));
    test_println(" bytes");
  }

  /* [13.12.2] The size of a thread structure is printed.*/
  test_set_step(2);
  {
    test_print("--- Thread: ");
    test_printn(sizeof(thread_t));
    test_println(" bytes");
  }

  /* [13.12.3] The size of a virtual timer structure is printed.*/
  test_set_step(3);
  {
    test_print("--- Timer : ");
    test_printn(sizeof(virtual_timer_t));
    test_println(" bytes");
  }

  /* [13.12.4] The size of a semaphore structure is printed.*/
  test_set_step(4);
  {
#if CH_CFG_USE_SEMAPHORES || defined(__DOXYGEN__)
    test_print("--- Semaph: ");
    test_printn(sizeof(semaphore_t));
    test_println(" bytes");
#endif
  }

  /* [13.12.5] The size of a mutex is printed.*/
  test_set_step(5);
  {
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
    test_print("--- Mutex : ");
    test_printn(sizeof(mutex_t));
    test_println(" bytes");
#endif
  }

  /* [13.12.6] The size of a condition variable is printed.*/
  test_set_step(6);
  {
#if CH_CFG_USE_CONDVARS || defined(__DOXYGEN__)
    test_print("--- CondV.: ");
    test_printn(sizeof(condition_variable_t));
    test_println(" bytes");
#endif
  }

  /* [13.12.7] The size of an event source is printed.*/
  test_set_step(7);
  {
#if CH_CFG_USE_EVENTS || defined(__DOXYGEN__)
    test_print("--- EventS: ");
    test_printn(sizeof(event_source_t));
    test_println(" bytes");
#endif
  }

  /* [13.12.8] The size of an event listener is printed.*/
  test_set_step(8);
  {
#if CH_CFG_USE_EVENTS || defined(__DOXYGEN__)
    test_print("--- EventL: ");
    test_printn(sizeof(event_listener_t));
    test_println(" bytes");
#endif
  }

  /* [13.12.9] The size of a mailbox is printed.*/
  test_set_step(9);
  {
#if CH_CFG_USE_MAILBOXES || defined(__DOXYGEN__)
    test_print("--- MailB.: ");
    test_printn(sizeof(mailbox_t));
    test_println(" bytes");
#endif
  }
}

static const testcase_t test_013_012 = {
  "RAM Footprint",
  NULL,
  NULL,
  test_013_012_execute
};

/****************************************************************************
 * Exported data.
 ****************************************************************************/

/**
 * @brief   Benchmarks.
 */
const testcase_t * const test_sequence_013[] = {
#if (CH_CFG_USE_MESSAGES) || defined(__DOXYGEN__)
  &test_013_001,
#endif
#if (CH_CFG_USE_MESSAGES) || defined(__DOXYGEN__)
  &test_013_002,
#endif
#if (CH_CFG_USE_MESSAGES) || defined(__DOXYGEN__)
  &test_013_003,
#endif
  &test_013_004,
  &test_013_005,
  &test_013_006,
#if (CH_CFG_USE_SEMAPHORES) || defined(__DOXYGEN__)
  &test_013_007,
#endif
  &test_013_008,
  &test_013_009,
#if (CH_CFG_USE_SEMAPHORES) || defined(__DOXYGEN__)
  &test_013_010,
#endif
#if (CH_CFG_USE_MUTEXES) || defined(__DOXYGEN__)
  &test_013_011,
#endif
  &test_013_012,
  NULL
};
