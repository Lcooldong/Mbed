#include "mbed.h"
#include "iostream"
#include <cstdio>
#include <string.h>
#include <string>

#define SERIAL_INT

using namespace std;

static BufferedSerial Serial(USBTX, USBRX, 115200);
char buffer[100];
string string_buffer = "";
int string_cursor = 0;
int len = 0;
int cursor = 0;

uint8_t motion_flag = 0;
uint8_t end_flag = 0;

enum{
    STOP = 0,
    MOVE,

};

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

FileHandle *mbed::mbed_override_console(int fd)
{
    return &Serial;
}

// void rx_callback(){
//     while (Serial.readable()){
//         char new_char = getchar();
//         buffer[len++] = new_char;
//         buffer[len] = '\0';

//         if(new_char == '\n'){
//             printf("%s", buffer);
//             len = 0;
//             printf("String : ");
//         }

//     }
// }

// main() runs in its own thread in the OS
int main()
{   
    cout << "program start" << endl;

    Thread serial_thread;
    // serial_thread.start(callback(rx_callback));
    char* new_char = new char[1];

    while(true){

#ifndef SERIAL_INT
        char ch = getchar();
        // putchar(ch);
        printf("0x%02X", ch);
        buffer[len] = ch;
        cursor += 1;
   
        if(ch == 'r')
        {
            led3 = !led3;
        }
        else if(ch == 0x7F) // backspace
        {   
            // printf("backspace pressed\r\n");
            if(len >= 0) {
                len = len - 1;
                buffer[len] = '\0';
            }
        }else if(ch == 0x1B){
            
        }


        len += 1;
        buffer[len] = '\0';
        // printf("%c %d\r\n", ch, len);

        printf("print len : %d\r\n", len);
        printf("\r");
        for(int i=0; i <= len; i++)
        {
            printf("%c", buffer[i]);
        }
#else
        while (Serial.readable())
        {
            Serial.read(new_char, sizeof(new_char));
            // string str;
            // getline(cin, str);
            // cout << str << '\n';
            
            if(*new_char == 0x7F)
            {   
                
                if(string_cursor > 0)
                {
                    string_buffer[string_cursor-1] = '\b';
                    cout << string_buffer[string_cursor-1] <<flush;
                    if(string_cursor == string_buffer.length()){
                        string_buffer.pop_back();
                    }else{
                        string_buffer.erase(string_cursor-1, string_cursor);
                    }
                    
                    string_cursor -= 1;
                    
                    
                    
                }
                else
                {   

                    string_buffer.clear();
                }
                
            }
            else if((*new_char >= 0x30 && *new_char <= 0x40)|| // 0 ~ 9
                    (*new_char >= 0x41 && *new_char <= 0x5A)|| // A ~ Z
                    (*new_char >= 0x61 && *new_char <= 0x7A))  // a ~ z
            {
                if(string_cursor == string_buffer.length())
                {
                    string_buffer += *new_char;
                }else{
                    string_buffer[string_cursor] = *new_char;
                }              
                string_cursor += 1;
            }
            else if(*new_char == 0x20)  // space
            {
                string_buffer.insert(string_cursor, " ");
                string_cursor += 1;
            }
            else if(*new_char == 0x0D)  // enter
            {
                // string_buffer.clear();
                string_cursor = 0;
                motion_flag = 1;
                printf("\r\n");
                break;
            }

            
            // cout << "\r\n===================\r\n";
            // Serial.write(&string_cursor, sizeof(string_cursor));
            // cout <<string_buffer << endl;
            // printf("\r");
            cout << "\r" << flush;
            for(int i=0; i< string_cursor; i++){
                // printf("%c", string_buffer[i]);
                cout << string_buffer[i]<< flush;
            }
            
            // printf("%c : 0x%X\r\n", *new_char, *new_char);
            // for (int i = 0; i < string_buffer.length(); i++) {
                
            // }

            // printf("cursor:%d, length : %d\r\n", string_cursor, string_buffer.length());
            // cout << "\r\n===================\r\n";
            wait_us(10);
        }
        
        
        if(motion_flag == 1){
            string delimiter = " ";
            size_t pos = 0;
            string string_temp;
            if(!string_buffer.compare("move")) printf("--help\r\n");

            while((pos = string_buffer.find(delimiter)) != string::npos){
                string_temp = string_buffer.substr(0, pos);
                if(!string_temp.compare("move")) end_flag = 1;  // same = 0
                cout << string_temp << endl;
                string_buffer.erase(0, pos + delimiter.length());
            }
            motion_flag = 0;
            if(end_flag == 1) break;
        }
        
        
    }
    printf("end of program\r\n");

#endif

    return 0;
}

