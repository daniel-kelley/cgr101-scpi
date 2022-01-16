/*
   event.c

   Copyright (c) 2022 by Daniel Kelley

*/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "misc.h"
#include "event.h"

#define MAXE 20

struct event {
    /* Event Queue */
    int eventq[2];
    int count;
    struct {
        enum event_id event_id;
        efunc func;
        void *arg;
    } e[MAXE];
};

/*
 * Event Handler
 */

static int event_handler(void *arg)
{
    struct event *event = arg;
    int err = 1;
    ssize_t len;
    char eventc;
    enum event_id event_id;
    int idx;

    assert(event);
    len = read(
        event->eventq[0],
        &eventc,
        sizeof(eventc));
    assert(len == sizeof(eventc));
    event_id = (enum event_id)eventc;
    for (idx=0; idx < event->count; idx++) {
        if (event->e[idx].event_id == event_id) {
            event->e[idx].func(event->e[idx].arg);
            err = 0;
            break;
        }
    }

    return err;
}


/*
 * Event Queue Sender
 */

void event_send(struct event *event, enum event_id event_id)
{
    ssize_t len_out;
    char cevent = (char)event_id;

    assert(event_id == (enum event_id)cevent);
    len_out = write(event->eventq[1], &cevent, sizeof(cevent));
    assert(len_out == sizeof(cevent));
}

int event_add(struct event *event,
              enum event_id event_id,
              efunc func, void *arg)
{
    int err = 1;

    assert(event);
    if (event->count < MAXE) {
        event->e[event->count].event_id = event_id;
        event->e[event->count].func = func;
        event->e[event->count].arg = arg;
        event->count++;
        err = 0;
    }

    return err;
}

struct event *event_init(struct worker *worker)
{
    struct event *event;
    int err;

    do {

        event = calloc(1,sizeof(*event));
        assert(event);

        /* Event queue. */
        err = pipe(event->eventq);
        if (err) {
            break;
        }

        cloexec(event->eventq[0]);
        cloexec(event->eventq[1]);

        err = worker_add(
            worker,
            event->eventq[0],
            event_handler,
            event);
        if (err) {
            break;
        }
    } while (0);

    assert(!err);

    return event;
}

void event_done(struct event *event)
{
    assert(event);
    close(event->eventq[0]);
    close(event->eventq[1]);
    free(event);

}
