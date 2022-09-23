#include "mbed.h"
#include "mbed_events.h"


// Creates static event queue
static EventQueue queue(0);
EventQueue queue2(32*EVENTS_EVENT_SIZE);
static int cnt = 0;

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);


void handler(int count);

// Creates events for later bound
auto event1 = make_user_allocated_event(handler, 1);
auto event2 = make_user_allocated_event(handler, 2);

// Creates event bound to the specified event queue
auto event3 = queue.make_user_allocated_event(handler, 3);
auto event4 = queue.make_user_allocated_event(handler, 4);

void handler(int count)
{
    printf("UserAllocatedEvent = %d \n", count);
    return;
}

void post_events(void)
{
    // Single instance of user allocated event can be posted only once.
    // Event can be posted again if the previous dispatch has finished or event has been canceled.

    // bind & post
    event1.call_on(&queue);

    // event cannot be posted again until dispatched or canceled
    bool post_succeed = event1.try_call();
    assert(!post_succeed);

    queue.cancel(&event1);

    // try to post
    post_succeed = event1.try_call();
    assert(post_succeed);

    // bind & post
    post_succeed = event2.try_call_on(&queue);
    assert(post_succeed);

    // post
    event3.call();

    // post
    event4();
}

void count_handler(){
    cnt++;
    printf("COUNT : %d\r\n", cnt);
}

void toggle_led1(){
    led1 = !led1;
    printf("Toggle LED1\r\n");
}

void toggle_led2(){
    led2 = !led2;
    printf("Toggle LED2\r\n");
}
void toggle_led3(){
    led3 = !led3;
    printf("Toggle LED3\r\n");
}

#include "rtos.h"
Thread thread;
volatile bool running = true;
void blink(DigitalOut* led){
    while(running){
        *led = !*led;
        ThisThread::sleep_for(1s);
        printf("Toggle Selected led\r\n");
    }
}

int main()
{
    printf("*** start ***\n");
    Thread event_thread;

    // The event can be manually configured for special timing requirements.
    // Timings are specified in milliseconds.
    // Starting delay - 100 msec
    // Delay between each event - 200msec
    event1.delay(100);
    event1.period(200);
    event2.delay(100);
    event2.period(200);
    event3.delay(100);
    event3.period(200);
    event4.delay(100);
    event4.period(200);

    event_thread.start(callback(post_events));

    // Posted events are dispatched in the context of the queue's dispatch function
    // queue.dispatch_for(400ms);        // Dispatch time - 400msec
    // 400 msec - Only 2 set of events will be dispatched as period is 200 msec

    event_thread.join();

    queue2.call_every(1s, toggle_led1);
    queue2.call_every(2s, toggle_led2);
    queue2.dispatch_forever();

    thread.start(callback(blink, &led3));
    // ThisThread::sleep_for(5000);    // 5초간 메인 멈춤
    wait_ns(1000);
    running = false;
    thread.join();


    return 0;
}