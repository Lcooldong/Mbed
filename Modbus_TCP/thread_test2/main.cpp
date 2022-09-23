#include "mbed.h"
#include "rtos.h"
Thread thread;
DigitalOut led3(LED3);
volatile bool running = true;
volatile bool running_keyboard = true;
// Blink function toggles the led in a long running loop
void blink(DigitalOut *led) {
    while (running) {
        *led = !*led;
        printf("Toggle LED\r\n");
        ThisThread::sleep_for(1s);
    }
}

void input_key(){
    while(running_keyboard){
        char ch = getchar();
        putchar(ch);
    }
}
// Spawns a thread to run blink for 5 seconds
int main() {
    printf("Start Main\r\n");
    thread.start(callback(blink, &led3));

    ThisThread::sleep_for(1s);

    
    while(running_keyboard){
        char ch = getchar();
        printf("Input char : ");
        putchar(ch);
        
        if(ch == 'q'){
            printf("\r\nQuit\r\n");
            break;
        }
        wait_ns(10);
    }

    running = false; 
    // thread.join(); // while(false)-> thread 를 멈춰야 join 작동 blocking
    printf("End of Main\r\n");
}