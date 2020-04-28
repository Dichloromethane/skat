
#include"ctimer.h"
#include<pthread.h>
#include<signal.h>
#include<time.h>
#include<sys/time.h>

static void
ctimer_tick(sigval_t sv) {
  ctimer *t = sv.sival_ptr;
  int noverruns = timer_getoverrun(t->timer_id);
  for (int i = 0; i < noverruns; i++)
    sem_post(&t->activations);
}

void 
ctimer_create(ctimer *t, void *arg, void (*timerf) (void *), int nsecs) {
  struct sigevent sev;
  
  t->timerf = timerf;
  t->arg = arg;
  sem_init(&t->activations, 0, 0);

  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_value.sival_ptr = t;
  sev.sigev_notify_function = ctimer_tick;
  sev.sigev_notify_attributes = NULL;

  timer_create(CLOCK_REALTIME, &sev, &t->timer_id);
}

_Noreturn void 
ctimer_run(ctimer *t) {
  struct itimerspec dummy;
  struct itimerspec itspec;

  itspec.it_interval.tv_nsec = t->nsecs;
  itspec.it_interval.tv_sec = 0;
  itspec.it_value.tv_nsec = 0;
  itspec.it_value.tv_sec = 0;

  timer_settime(t->timer_id, 0, &itspec, &dummy);

  for (;;) {
	sem_wait(&t->activations);
	t->timerf(t->arg);
  }
}
