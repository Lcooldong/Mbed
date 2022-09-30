#include "main.h"
#include "mbed.h"
#include "wifi_helper.h"
#include "mbed-trace/mbed_trace.h"
#include "iostream"


static BufferedSerial Serial(USBTX, USBRX, 115200);
using namespace std;

FileHandle *mbed::mbed_override_console(int fd)
{
    return &Serial;
}

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

static bool running = 0;
static uint32_t cnt = 0;



int main() {
    cout << "\r\nStart Program" << endl;
    printf(
        "Mbed OS version %d.%d.%d\n",
        MBED_MAJOR_VERSION,
        MBED_MINOR_VERSION,
        MBED_PATCH_VERSION
    );

    char* buffer = new (std::nothrow) char[MAX_BUFFER_SIZE];
    



    if(!buffer){
        cout << "Could not allocate memory" << endl;
    }else{
        running = 1;
        printf("Buffer is now available\r\n");
    }

    

    for (int i=0; i<MAX_BUFFER_SIZE; i++) {
        buffer[i] = 0;
    }

    strcpy(buffer, "here is data");

    printf("pointer size : %d\r\n", sizeof(buffer));
    printf("buffer size : %d\r\n", strlen(buffer));

    

    Serial.set_format(/*bits*/8, 
                      /*parity*/BufferedSerial::None, 
                      /*stop bit*/1);


    while(running){
        //initialize
        for (int i=0; i<MAX_BUFFER_SIZE; i++) {
            buffer[i] = 0;
        }
        
        printf("\r\n===================================================\r\n");
        printf("first buffer_size : %d\r\n", strlen(buffer));   // 0
        sprintf(buffer, "target %d :", cnt++);
        printf("second buffer_size : %d\r\n", strlen(buffer));  // 10
        Serial.write(buffer, strlen(buffer));
        Serial.read(buffer, MAX_BUFFER_SIZE);
        printf("third buffer_size : %d\r\n", strlen(buffer));
        Serial.write(buffer, strlen(buffer));
        // wait_ms(1000);
        printf("\r\n===================================================\r\n");
    }

    delete[] buffer;

    return 0;
}
