#include "mbed.h"
#include "stdio.h"
#include "EthernetInterface.h"


#define TARGET_TX_PIN   USBTX
#define TARGET_RX_PIN   USBRX

static BufferedSerial serial_port(TARGET_TX_PIN, TARGET_RX_PIN, 115200);

FileHandle *mbed::mbed_override_console(int fd)
{
    return &serial_port;
}

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

EthernetInterface eth;
const char* SERVER_ADDRESS = "192.168.0.1";
// SocketAddress SERVER_ADDRESS = "192.168.0.1";

int main()
{
    if(led1.is_connected()){
        printf("LED Start\n");
    }

    eth.connect();
    printf("\nServer IP Address %s\n", SERVER_ADDRESS);
    // eth.get_ip_address((SocketAddress *)SERVER_ADDRESS);

    TCPSocket server;


    
    while (true) {
        
        led1.write(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
        GPIOB->ODR |= 0x01 <<14;
        thread_sleep_for(500);

        led1.write(0);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
        GPIOB->ODR &= ~(0x01 <<14);
        HAL_Delay(500);
        
    }
}

