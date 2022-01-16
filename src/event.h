/*
   event.h

   Copyright (c) 2022 by Daniel Kelley

*/

#ifndef   EVENT_H_
#define   EVENT_H_

#include "worker.h"

struct event;

enum event_id {
    EVENT_IDENTIFY_COMPLETE,
    EVENT_IDENTIFY_OUTPUT,
    EVENT_DIGITAL_READ_COMPLETE,
    EVENT_DIGITAL_READ_OUTPUT,
    EVENT_SCOPE_STATUS_OUTPUT,
    EVENT_SCOPE_STATUS_COMPLETE,
    EVENT_UNBLOCK,
};

extern void event_send(struct event *event, enum event_id event_id);
typedef void (*efunc)(void *arg);

extern struct event *event_init(struct worker *worker);
extern void event_done(struct event *event);
extern int event_add(struct event *event,
                     enum event_id event_id,
                     efunc func,
                     void *arg);

#endif /* EVENT_H_ */
