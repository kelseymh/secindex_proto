// $Id$
// UsageTimer.cc -- Utility to wrap getrusage() for collecting data within
// a program.
//
// 20151026  Michael Kelsey
// 20151114  Replace clock_t with time_t to get wall-clock duration

#include "UsageTimer.hh"
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <iostream>


// Static buffer to initialize timers for incremental work

const struct timeval UsageTimer::tZero = {0,0};
const struct rusage UsageTimer::uZero =
  { tZero,tZero,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


// Implentation of usage functions

void UsageTimer::zero() {		// Reset total count
  uTotal = uZero;
  tTotal = tZero;
}

void UsageTimer::start() {		// Begin recording
  gettimeofday(&tStart, NULL);
  getrusage(RUSAGE_SELF, &uStart);
}

void UsageTimer::end() {		// Finish recording
  gettimeofday(&tEnd, NULL);
  getrusage(RUSAGE_SELF, &uEnd);
  uTotal += uEnd;
  uTotal -= uStart;
  tTotal += tEnd;
  tTotal -= tStart;
}


// Dump information about cumulative results in CSH "time" format

void UsageTimer::report(std::ostream& os) const {
  double ttime = elapsed();

  char tbuf[80];
  snprintf(tbuf, sizeof(tbuf),
	   "%5.3fu %5.3fs %d:%05.2f %3.1f%%\t%ld+%ldk %ld+%ldio %ldpf+%ldw",
	   userTime(), sysTime(), int(ttime/60.), ttime-60.*int(ttime/60.),
	   100.*cpuEff(), (long)memoryText(),(long)(memoryData()+memoryStack()),
	   ioInput(), ioOutput(), pageFaults(), swaps());

  os << tbuf;
}


// Global operators to do timeval arithmetic, used by UsageTimer

struct timeval operator+(const struct timeval& tva, const struct timeval& tvb) {
  struct timeval tvSum = { tva.tv_sec+tvb.tv_sec, tva.tv_usec+tvb.tv_usec };
  if (tvSum.tv_usec > 1000000) { tvSum.tv_usec-=1000000; tvSum.tv_sec++; }
  return tvSum;
}

struct timeval operator-(const struct timeval& tva, const struct timeval& tvb) {
  struct timeval tvDiff = { tva.tv_sec-tvb.tv_sec, tva.tv_usec-tvb.tv_usec };
  if (tvDiff.tv_usec < 0) { tvDiff.tv_usec+=1000000; tvDiff.tv_sec--; }
  return tvDiff;
}

struct timeval& operator+=(struct timeval& tva, const struct timeval& tvb) {
  tva.tv_sec += tvb.tv_sec;
  tva.tv_usec += tvb.tv_usec;
  if (tva.tv_usec > 1000000) { tva.tv_usec-=1000000; tva.tv_sec++; }
  return tva;
}

struct timeval& operator-=(struct timeval& tva, const struct timeval& tvb) {
  tva.tv_sec -= tvb.tv_sec;
  tva.tv_usec -= tvb.tv_usec;
  if (tva.tv_usec < 0) { tva.tv_usec+=1000000; tva.tv_sec--; }
  return tva;
}

// Global operators to do rusage arithmetic, used by UsageTimer
 
struct rusage operator+(const struct rusage& rua, const struct rusage& rub) {
  struct rusage ruSum = {
    rua.ru_utime+rub.ru_utime, rua.ru_stime+rub.ru_stime,
    rua.ru_maxrss+rub.ru_maxrss, rua.ru_ixrss+rub.ru_ixrss,
    rua.ru_idrss+rub.ru_idrss, rua.ru_isrss+rub.ru_isrss,
    rua.ru_minflt+rub.ru_minflt, rua.ru_majflt+rub.ru_majflt,
    rua.ru_nswap+rub.ru_nswap, rua.ru_inblock+rub.ru_inblock,
    rua.ru_oublock+rub.ru_oublock, rua.ru_msgsnd+rub.ru_msgsnd,
    rua.ru_msgrcv+rub.ru_msgrcv, rua.ru_nsignals+rub.ru_nsignals,
    rua.ru_nvcsw+rub.ru_nvcsw, rua.ru_nivcsw+rub.ru_nivcsw };
  return ruSum;
}

struct rusage operator-(const struct rusage& rua, const struct rusage& rub) {
  struct rusage ruDiff = {
    rua.ru_utime-rub.ru_utime, rua.ru_stime-rub.ru_stime,
    rua.ru_maxrss-rub.ru_maxrss, rua.ru_ixrss-rub.ru_ixrss,
    rua.ru_idrss-rub.ru_idrss, rua.ru_isrss-rub.ru_isrss,
    rua.ru_minflt-rub.ru_minflt, rua.ru_majflt-rub.ru_majflt,
    rua.ru_nswap-rub.ru_nswap, rua.ru_inblock-rub.ru_inblock,
    rua.ru_oublock-rub.ru_oublock, rua.ru_msgsnd-rub.ru_msgsnd,
    rua.ru_msgrcv-rub.ru_msgrcv, rua.ru_nsignals-rub.ru_nsignals,
    rua.ru_nvcsw-rub.ru_nvcsw, rua.ru_nivcsw-rub.ru_nivcsw };
  return ruDiff;
}

struct rusage& operator+=(struct rusage& rua, const struct rusage& rub) {
  rua.ru_utime+=rub.ru_utime; rua.ru_stime+=rub.ru_stime;
  rua.ru_maxrss+=rub.ru_maxrss; rua.ru_ixrss+=rub.ru_ixrss;
  rua.ru_idrss+=rub.ru_idrss; rua.ru_isrss+=rub.ru_isrss;
  rua.ru_minflt+=rub.ru_minflt; rua.ru_majflt+=rub.ru_majflt;
  rua.ru_nswap+=rub.ru_nswap; rua.ru_inblock+=rub.ru_inblock;
  rua.ru_oublock+=rub.ru_oublock; rua.ru_msgsnd+=rub.ru_msgsnd;
  rua.ru_msgrcv+=rub.ru_msgrcv; rua.ru_nsignals+=rub.ru_nsignals;
  rua.ru_nvcsw+=rub.ru_nvcsw; rua.ru_nivcsw+=rub.ru_nivcsw;
  return rua;
}

struct rusage& operator-=(struct rusage& rua, const struct rusage& rub) {
  rua.ru_utime-=rub.ru_utime; rua.ru_stime-=rub.ru_stime;
  rua.ru_maxrss-=rub.ru_maxrss; rua.ru_ixrss-=rub.ru_ixrss;
  rua.ru_idrss-=rub.ru_idrss; rua.ru_isrss-=rub.ru_isrss;
  rua.ru_minflt-=rub.ru_minflt; rua.ru_majflt-=rub.ru_majflt;
  rua.ru_nswap-=rub.ru_nswap; rua.ru_inblock-=rub.ru_inblock;
  rua.ru_oublock-=rub.ru_oublock; rua.ru_msgsnd-=rub.ru_msgsnd;
  rua.ru_msgrcv-=rub.ru_msgrcv; rua.ru_nsignals-=rub.ru_nsignals;
  rua.ru_nvcsw-=rub.ru_nvcsw; rua.ru_nivcsw-=rub.ru_nivcsw;
  return rua;
}
