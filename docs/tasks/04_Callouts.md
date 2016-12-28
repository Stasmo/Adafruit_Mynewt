# Callouts

**WIP!**

Callouts are SW timers that can be used to fire an event in an event queue
after the specified delay (measured in os ticks.)

If you need to read a sensor at a specific interval like 50Hz, for example, you
can register a callout to fire every 20ms, which will cause the callout
callback handler to fire at that rate (within the limits of the scheduler and
available clock time!), and you would read your sensor inside the callback
function.

These can be more accurate than inserting a fixed delay inside a task handler
using `os_time_delay` in situations where the execution time inside the task
handler may vary, but where you want to maintain a specific known rate of data
sampling or some other activity.

### Initializing a callout

First define the `os_eventq` event queue and the `os_callout` timer:

```
struct os_eventq my_evq;
static struct os_callout my_timer;
```

Then define and implement a callout callback handler:

```
static void
my_timer_cb(struct os_event *ev)
{
    int32_t timeout = OS_TICKS_PER_SEC / 1000;

    /* Do something! */

    /* Reset the timeout so that it fires again */
    os_callout_reset(&my_timer, timeout);
}
```

Then inside main (or elsewhere), initialize the callout as follows:

```
int
main(void)
{
    ...
    /* Initialize eventq for the application task. */
    os_eventq_init(&my_evq);

    /* Create a callout (timer).  This callout is used by the "tx" bletiny
     * command to repeatedly send packets of sequential data bytes.
     */
    os_callout_init(&my_timer, &my_evq, my_timer_cb, NULL);

    /* Start the callout timer */
    os_callout_reset(&my_timer, OS_TICKS_PER_SEC);
    ...
}
```
