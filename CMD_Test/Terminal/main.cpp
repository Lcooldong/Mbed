#include "mbed.h"
#include "iostream"

#define BUFF_SIZE 100
#define wait_ms(ms) _wait_us_inline((int)1000 * ms)

static BufferedSerial Serial(USBTX, USBRX, 9600);
char buffer[BUFF_SIZE];
size_t len = 0;
size_t cursor_position = 0;
string string_buffer = "";

using namespace std;

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

FileHandle *mbed::mbed_override_console(int fd)
{
    return &Serial;
}


// main() runs in its own thread in the OS
int main()
{
    wait_ms(1000);

    cout << "program start" << endl;

    Thread serial_thread;
    // serial_thread.start(callback(rx_callback));
    char* key_char = new char[1];

    while(true){
        while (Serial.readable()) {
            Serial.read(key_char, sizeof(key_char));
            printf("%d : %c \r\n", len, *key_char);

            buffer[len++] = *key_char;
            cursor_position++;

            // if(buffer[len] != '\0'){
            //     
            //     printf("string null check LEN : %d  %d", len, cursor_position);                
            // }
            // Serial.write(key_char, sizeof(key_char));
           
            // printf("%c\r\n", *key_char);
            // cout << *key_char << '\n';

            if(*key_char == '\b'){
                // char* text = new char[] {"press backspace\r\n"};
                // Serial.write(text, strlen(text));
                led1 = !led1;
                cursor_position--;
                if(len == cursor_position){
                    
                }
                

            }else if(*key_char == '\r'){
                char text[] = "enter pressed\r\n";
                Serial.write(text, strlen(text));
                led2 = !led2;

                for (int i=0; i<len; i++) {
                    Serial.write(&buffer[i], sizeof(buffer[i]));
                }
            }

            

            wait_us(10);
        }
    }

    return 0;
}

