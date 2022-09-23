/* Sockets Example
 * Copyright (c) 2016-2020 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// nsapi => network socket api

#include "mbed.h"
#include "wifi_helper.h"
#include "mbed-trace/mbed_trace.h"
#include "string.h"
#include <cstdio>
#include <ratio>
#include "stdio.h"
#include "mbed_events.h"

#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

#define TCP_DHCP 

#ifdef TCP_DHCP
    #define TCP_DHCP_ENABLE 1
#else
    #define TCP_DHCP_ENABLE 0
#endif

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
InterruptIn button1(BUTTON1);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t1;
int32_t temp;
char recv_data[32] = {0x00, };
static uint32_t tr = 0;

static BufferedSerial uart(USBTX, USBRX, 9600);
static char serial_buffer[1024];

FileHandle *mbed::mbed_override_console(int fd)
{
    return &uart;
}

static int cnt = 0;
void count_handler(){
    cnt++;
    printf("COUNT : %d\r\n", cnt);
}

struct _MBAP_Header{
    short transaction_id;
    short protocol_id;
    short length;
    char unit_id;
}MBAP_Header;

struct _PDU{
    char function_code;
    struct PDU_data;
}PDU;

struct _PDU_data{
    char rw_flag;
    short start_address;
    char* data;
}PDU_data;

struct _modbus_data{
    int* packet;
    int size_of_packet;
    struct MBAP_Header;
    struct PDU;
}modbus_data;

class SocketDemo {
    static constexpr size_t MAX_NUMBER_OF_ACCESS_POINTS = 10;
    // static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 100;
    static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 1024;

#if MBED_CONF_APP_USE_TLS_SOCKET
    static constexpr size_t REMOTE_PORT = 443; // tls port
#else
    static constexpr size_t REMOTE_PORT = 80; // standard HTTP port
    static constexpr size_t MODEBUS_PORT = 502;
#endif // MBED_CONF_APP_USE_TLS_SOCKET

public:
    SocketDemo() : _net(NetworkInterface::get_default_instance())
    {
    }

    ~SocketDemo()
    {
        if (_net) {
            _net->disconnect();
        }
    }

    void run(const char* IP, uint16_t PORT)
    {
        if (!_net) {
            printf("Error! No network interface found.\r\n");
            return;
        }
        if (_net->wifiInterface()) {
            wifi_scan();
        }
        printf("Connecting to the network...\r\n");
        // _net->set_dhcp(TCP_DHCP_ENABLE);

#ifndef TCP_DHCP
        // fixed ethernet ip
        SocketAddress ip_address = "192.168.0.125";
        SocketAddress netmask    = "255.255.255.0";
        SocketAddress gateway    = "192.168.0.1";
        _net->set_network(ip_address, netmask, gateway);
#endif
        nsapi_size_or_error_t result = _net->connect();
        if (result != 0) {
            printf("Error! _net->connect() returned: %d\r\n", result);
            return;
        }

        print_network_info();   // function below : IP address

        /* opening the socket only allocates resources */
        result = _socket.open(_net);
        if (result != 0) {
            printf("Error! _socket.open() returned: %d\r\n", result);
            return;
        }

#if MBED_CONF_APP_USE_TLS_SOCKET
        result = _socket.set_root_ca_cert(root_ca_cert);
        if (result != NSAPI_ERROR_OK) {
            printf("Error: _socket.set_root_ca_cert() returned %d\n", result);
            return;
        }
        _socket.set_hostname(MBED_CONF_APP_HOSTNAME);
#endif // MBED_CONF_APP_USE_TLS_SOCKET

        /* now we have to find where to connect */
        SocketAddress address;

        address.set_port(PORT);
        address.set_ip_address(IP);

        printf("Socket Connect IP : %s\r\n", IP);
        printf("Opening connection to remote port %d : \r\n", PORT);

        /////////////////////CONNECT////////////////////
        result = _socket.connect(address);
        if (result != 0) {
            printf("Error! _socket.connect() returned: %d\r\n", result);
            return;
        }else{
            printf("Socket connect successfully\r\n");
            // return;
        }
        
        return;
    }

    bool send_modbus_request(void* data, size_t bytes_to_send)
    {
        char* buffer = (char*)data;

        nsapi_size_or_error_t bytes_sent = 0;

        printf("\r\nSending message: \r\n%s", buffer);

        while (bytes_to_send) {
            bytes_sent = _socket.send(buffer + bytes_sent, bytes_to_send);
            if (bytes_sent < 0) {
                printf("Error! _socket.send() returned: %d\r\n", bytes_sent);
                return false;
            } else {
                printf("sent %d bytes\r\n", bytes_sent);
            }
            bytes_to_send -= bytes_sent;
        }

        printf("Complete message sent\r\n");

        return true;
    }

    // bool receive_modbus_response(char buffer[], int remaining_bytes)
    bool receive_modbus_response()
    {
        char buffer[MAX_MESSAGE_RECEIVED_LENGTH];
        int remaining_bytes = MAX_MESSAGE_RECEIVED_LENGTH;
        int received_bytes = 0;
        printf("====Start to receive Modbus====\r\n");
        /* loop until there is nothing received or we've ran out of buffer space */
        nsapi_size_or_error_t result = remaining_bytes;
        while (result > 0 && remaining_bytes > 0) {
            result = _socket.recv(buffer + received_bytes, remaining_bytes);
            if (result < 0) {
                printf("Error! _socket.recv() returned: %d\r\n", result);
                return false;
            }

            received_bytes += result;
            remaining_bytes -= result;
        }

        /* the message is likely larger but we only want the HTTP response code */

        printf("received %d bytes:\r\n%.*s\r\n\r\n", received_bytes, strstr(buffer, "\n") - buffer, buffer);
        for (int i = 0; i < received_bytes; i++) {
            printf("%02x",  buffer[i]);
        }
        printf("\r\n");

        return true;
    }

    int receive_response(int received_bytes){
        uint8_t buffer[MAX_MESSAGE_RECEIVED_LENGTH];
        int remaining_bytes = MAX_MESSAGE_RECEIVED_LENGTH;
        int number_of_bytes = 0;
        _socket.recv(buffer, remaining_bytes);
        
        printf("=====================================================================\r\n");
        printf("%d : ", tr);
        for (int i = 2; i < received_bytes; i++) {
            printf("%02x ",  buffer[i]);
        }
        printf("\r\n=====================================================================\r\n");
        tr = tr + 1;
        return number_of_bytes;
    }


    bool open_socket()
    {
        nsapi_size_or_error_t result = _socket.open(_net);
        if (result != 0) {
            printf("Error! _socket.open() returned: %d\r\n", result);
            return false;
        }
        SocketAddress address;

        address.set_ip_address("192.168.0.37");
        address.set_port(MODEBUS_PORT);
        
        printf("Opening connection to remote port %d\r\n", MODEBUS_PORT);

        result = _socket.connect(address);
        if (result != 0) {
            printf("Error! _socket.connect() returned: %d\r\n", result);
            return false;
        }

        return true;
    }

    void close_socket()
    {
        _socket.close();
        printf("Close Socket\r\n");
        // _net->disconnect();
    }

private:
    bool resolve_hostname(SocketAddress &address)
    {
        const char hostname[] = MBED_CONF_APP_HOSTNAME;

        /* get the host address */
        printf("\nResolve hostname %s\r\n", hostname);
        nsapi_size_or_error_t result = _net->gethostbyname(hostname, &address);
        if (result != 0) {
            printf("Error! gethostbyname(%s) returned: %d\r\n", hostname, result);
            return false;
        }

        printf("%s address is %s\r\n", hostname, (address.get_ip_address() ? address.get_ip_address() : "None") );

        return true;
    }


    void wifi_scan()
    {
        WiFiInterface *wifi = _net->wifiInterface();

        WiFiAccessPoint ap[MAX_NUMBER_OF_ACCESS_POINTS];

        /* scan call returns number of access points found */
        int result = wifi->scan(ap, MAX_NUMBER_OF_ACCESS_POINTS);

        if (result <= 0) {
            printf("WiFiInterface::scan() failed with return value: %d\r\n", result);
            return;
        }

        printf("%d networks available:\r\n", result);

        for (int i = 0; i < result; i++) {
            printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\r\n",
                   ap[i].get_ssid(), get_security_string(ap[i].get_security()),
                   ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
                   ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5],
                   ap[i].get_rssi(), ap[i].get_channel());
        }
        printf("\r\n");
    }

    void print_network_info()
    {
        /* print the network info */
        SocketAddress a;
        _net->get_ip_address(&a);       // get ip from socket
        printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_netmask(&a);
        printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_gateway(&a);
        printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    }


private:
    NetworkInterface *_net;

#if MBED_CONF_APP_USE_TLS_SOCKET
    TLSSocket _socket;
#else
    TCPSocket _socket;
#endif // MBED_CONF_APP_USE_TLS_SOCKET
};


void press_btn_handler(void)
{
    queue.call(printf, "Button Press %p", ThisThread::get_id());
    // printf("rise_handler in context %p\n", ThisThread::get_id());
    printf("Butten LED2 Pressed\r\n");
    led2 = !led2;
    
}

void function_while(){
    SocketDemo *example = new SocketDemo();
    MBED_ASSERT(example);

    // check with ifconfig or ipconfig
    const char* modbus_ip = "192.168.0.37";
    int modbus_port = 502;
    
    example->run(modbus_ip, modbus_port);
    char* c = new char[1];
    while(true){
        if(button1.read() == GPIO_PIN_SET){
            if(HAL_GetTick() - temp > 100){
                printf("Butten LED2 Pressed\r\n");
                led2 = !led2;
    
            }
            while(button1.read() == GPIO_PIN_RESET){}
            temp = HAL_GetTick();
        }

        // uart.read(&serial_buffer, sizeof(serial_buffer));
        uart.read(c, sizeof(c));
        uart.write(c, sizeof(c));
        if(*c == 'a')
        {
            // printf("a\r\n");
            led1 = !led1;
            // Last 2bytes -> get number of data
            unsigned char receive_code[] = {00, 00, 00,00, 00, 06, 01, 03, 00, 00, 00, 10};
            example->send_modbus_request((char*)receive_code, sizeof(receive_code));
            char receive_buffer[1024] = {0,};

            // nsapi_size_or_error_t result = example->receive_modbus_response(receive_buffer, 1024);
            // nsapi_size_or_error_t result = example->receive_modbus_response();
            example->receive_response(20);
            // if (result != 0) {
            //     printf("Receive Error! : %d\r\n", result);           
            // }else{
            //     printf("Receive Completed");
            // }
            thread_sleep_for(1);
        }
        else if(*c == 'b')
        {
            // printf("b\r\n");
            led3 = !led3;
        }
        else if(*c == 'q')
        {
            printf("\r\nStop Program\r\n");
            break;
        }
    }
}



///////////////////////////////////////////MAIN///////////////////////////////////////////


int main() {
    printf("\r\n[Starting socket demo]\r\n\r\n");

#ifdef MBED_CONF_MBED_TRACE_ENABLE
    mbed_trace_init();
#endif
    
    
    SocketDemo *example = new SocketDemo();
    MBED_ASSERT(example);

    // check with ifconfig or ipconfig
    const char* modbus_ip = "192.168.0.37";
    int modbus_port = 502;
    
    example->run(modbus_ip, modbus_port);

    
    // example->run(local_ip, modbus_port);
    // unsigned char data[12] = {00,00, 00,00, 00,06, 01, 06, 00,00,00,19};


    // //Send
    // example->send_modbus_request((char*)data, sizeof(data));
    // thread_sleep_for(1500);

    // //Receive
    // // Transaction ID 2bytes, Protocol ID 2bytes, Length 2bytes, [Unit ID 1byte, Function code 1byte, Data]
    // Data => Start Register 2bytes, Number of data 2bytes

    const char* uart_text = "a or b : ";
    uart.write(uart_text, strlen(uart_text));
    char* c = new char[1];
    char ch = getchar();
    putchar(ch);

    queue.call_every(1s, count_handler);
    // queue.call_every(2s, function_while);
    queue.dispatch_forever();


    while(true){
        if(button1.read() == GPIO_PIN_SET){
            if(HAL_GetTick() - temp > 100){
                printf("Butten LED2 Pressed\r\n");
                led2 = !led2;
    
            }
            while(button1.read() == GPIO_PIN_RESET){}
            temp = HAL_GetTick();
        }

        // uart.read(&serial_buffer, sizeof(serial_buffer));
        uart.read(c, sizeof(c));
        uart.write(c, sizeof(c));
        if(*c == 'a')
        {
            // printf("a\r\n");
            led1 = !led1;
            // Last 2bytes -> get number of data
            unsigned char receive_code[] = {00, 00, 00,00, 00, 06, 01, 03, 00, 00, 00, 10};
            example->send_modbus_request((char*)receive_code, sizeof(receive_code));
            char receive_buffer[1024] = {0,};

            // nsapi_size_or_error_t result = example->receive_modbus_response(receive_buffer, 1024);
            // nsapi_size_or_error_t result = example->receive_modbus_response();
            example->receive_response(20);
            // if (result != 0) {
            //     printf("Receive Error! : %d\r\n", result);           
            // }else{
            //     printf("Receive Completed");
            // }
            thread_sleep_for(1);
        }
        else if(*c == 'b')
        {
            // printf("b\r\n");
            led3 = !led3;
        }
        else if(*c == 'q')
        {
            printf("\r\nStop Program\r\n");
            break;
        }
    }
    
    example->close_socket();

    return 0;
}
