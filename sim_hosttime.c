/****************************************************************************
 * arch/sim/src/sim/posix/sim_hosttime.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "sim_internal.h"

/****************************************************************************
 * Platform detection
 ****************************************************************************/

#if defined(__APPLE__) && defined(__MACH__)
#  define NUTTX_DARWIN
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint64_t g_start;

#ifndef NUTTX_DARWIN
static timer_t g_timer;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: host_inittimer
 ****************************************************************************/

int host_inittimer(void)
{
  struct timespec tp;

  clock_gettime(CLOCK_MONOTONIC, &tp);
  g_start = 1000000000ull * tp.tv_sec + tp.tv_nsec;

#ifdef NUTTX_DARWIN
  /* macOS: setitimer requires no explicit timer creation */
  return 0;
#else
  struct sigevent sigev;
  sigev.sigev_notify = SIGEV_SIGNAL;
  sigev.sigev_signo  = SIGALRM;
  sigev.sigev_value.sival_ptr = NULL;

  return timer_create(CLOCK_MONOTONIC, &sigev, &g_timer);
#endif
}

/****************************************************************************
 * Name: host_gettime
 ****************************************************************************/

uint64_t host_gettime(bool rtc)
{
  struct timespec tp;
  uint64_t current;

  clock_gettime(rtc ? CLOCK_REALTIME : CLOCK_MONOTONIC, &tp);
  current = 1000000000ull * tp.tv_sec + tp.tv_nsec;

  return rtc ? current : current - g_start;
}

/****************************************************************************
 * Name: host_sleep
 ****************************************************************************/

void host_sleep(uint64_t nsec)
{
  usleep((nsec + 999) / 1000);
}

/****************************************************************************
 * Name: host_sleepuntil
 ****************************************************************************/

void host_sleepuntil(uint64_t nsec)
{
  uint64_t now = host_gettime(false);

  if (nsec > now + 1000)
    {
      usleep((nsec - now) / 1000);
    }
}

/****************************************************************************
 * Name: host_settimer
 *
 * Description:
 *   Set up a timer to send periodic signals.
 *
 * Input Parameters:
 *   nsec - timer expire time
 *
 * Returned Value:
 *   On success, (0) zero value is returned, otherwise a negative value.
 *
 ****************************************************************************/

int host_settimer(uint64_t nsec)
{
#ifdef NUTTX_DARWIN
  /* macOS implementation using setitimer() */

  struct itimerval it;
  uint64_t usec = nsec / 1000;

  it.it_value.tv_sec  = usec / 1000000;
  it.it_value.tv_usec = usec % 1000000;

  /* One-shot timer */
  it.it_interval.tv_sec  = 0;
  it.it_interval.tv_usec = 0;

  return setitimer(ITIMER_REAL, &it, NULL);

#else
  /* Linux / POSIX implementation */

  struct itimerspec tspec;
  uint64_t abs;

  abs = nsec + g_start;

  tspec.it_value.tv_sec  = abs / 1000000000;
  tspec.it_value.tv_nsec = abs % 1000000000;

  tspec.it_interval.tv_sec  = 0;
  tspec.it_interval.tv_nsec = 0;

  return timer_settime(g_timer, TIMER_ABSTIME, &tspec, NULL);
#endif
}

/****************************************************************************
 * Name: host_timerirq
 *
 * Description:
 *   Get timer irq
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   On success, irq num returned, otherwise a negative value.
 *
 ****************************************************************************/

int host_timerirq(void)
{
  return SIGALRM;
}
