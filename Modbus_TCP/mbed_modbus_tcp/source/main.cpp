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

#include "mbed.h"
#include "mbed-trace/mbed_trace.h"

#include "rtos.h"
#include "main.h"

#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

Thread thread;
DigitalOut led3(LED3);
volatile bool running = true;
volatile bool running_keyboard = true;


class SocketDemo {
    

#if MBED_CONF_APP_USE_TLS_SOCKET
    static constexpr size_t REMOTE_PORT = 443; // tls port
#else
    static constexpr size_t REMOTE_PORT = 80; // standard HTTP port
#endif // MBED_CONF_APP_USE_TLS_SOCKET

protected:
    #if MBED_CONF_APP_USE_TLS_SOCKET
        TLSSocket _socket;
    #else
        TCPSocket _socket;
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


        printf("Connecting to the network...\r\n");

        nsapi_size_or_error_t result = _net->connect();
        if (result != 0) {
            printf("Error! _net->connect() returned: %d\r\n", result);
            return;
        }

        print_network_info();

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

        // if (!resolve_hostname(address)) {
        //     return;
        // }

        address.set_port(PORT);
        address.set_ip_address(IP);

        /* we are connected to the network but since we're using a connection oriented
         * protocol we still need to open a connection on the socket */
        printf("Socket Connect IP : %s\r\n", IP);
        printf("Opening connection to remote port : %d\r\n", PORT);

        result = _socket.connect(address);
        if (result != 0) {
            printf("Error! _socket.connect() returned: %d\r\n", result);
            printf("Please check modbus connection or IP\r\n");
            return;
        }else{
            printf("Socket connect successfully\r\n");
            return;
        }
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

    bool send_http_request()
    {
        /* loop until whole request sent */
        const char buffer[] = "GET / HTTP/1.1\r\n"
                              "Host: ifconfig.io\r\n"
                              "Connection: close\r\n"
                              "\r\n";

        nsapi_size_t bytes_to_send = strlen(buffer);
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

    bool receive_http_response()
    {
        char buffer[MAX_MESSAGE_RECEIVED_LENGTH];
        int remaining_bytes = MAX_MESSAGE_RECEIVED_LENGTH;
        int received_bytes = 0;

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

        return true;
    }

    void print_network_info()
    {
        /* print the network info */
        SocketAddress a;
        _net->get_ip_address(&a);
        printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_netmask(&a);
        printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_gateway(&a);
        printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    }

private:
    NetworkInterface *_net;


};


class Modbus_Protocol:SocketDemo{
    Thread monitor;
    short transaction_ID;
    short protocol_ID;
    short data_length;
    char unit_ID;
    char function_code;
public:
    Modbus_Protocol()
    {
        Modbus_Protocol::_Modbus_ADU ADU;
        transaction_ID = 0;
    }
    ~Modbus_Protocol()
    {

    }

    struct _Modbus_ADU{
        
    }Modbus_ADU;

    static void monitoring(DigitalOut* led)
    {
        while(running)
        {
            *led = !*led;
            printf("monitoring start\r\n");
            ThisThread::sleep_for(1s);
        }
    }

    void start()
    {
        monitor.start(callback(Modbus_Protocol::monitoring, &led3));
    }

    // int receive_response(struct _Modbus_ADU *ADU, int received_bytes){
    int receive_response(int received_bytes){
        uint8_t buffer[MAX_MESSAGE_RECEIVED_LENGTH];
        int remaining_bytes = MAX_MESSAGE_RECEIVED_LENGTH;
        int number_of_bytes = 0;
        _socket.recv(buffer, remaining_bytes);
        
        printf("=====================================================================\r\n");
        printf("%d : ", transaction_ID);
        for (int i = 2; i < received_bytes; i++) {
            printf("%02x ",  buffer[i]);
        }
        printf("\r\n=====================================================================\r\n");
        transaction_ID = transaction_ID + 1;
        return number_of_bytes;
    }
    

private:


};


int main() {
    printf("\r\nStarting socket demo\r\n\r\n");

#ifdef MBED_CONF_MBED_TRACE_ENABLE
    mbed_trace_init();
#endif

    SocketDemo *example = new SocketDemo();
    MBED_ASSERT(example);
    Modbus_Protocol *modbus = new Modbus_Protocol();
    MBED_ASSERT(modbus); 
    
    // check with ifconfig or ipconfig
    const char* modbus_ip = "192.168.0.37";
    int modbus_port = 502;
    example->run(modbus_ip, modbus_port);

    // ThisThread::sleep_for(1s);
    wait_ns(1000);
    // thread.start(callback(blink, &led3));
    modbus->start();
    

    while(running_keyboard){
        char ch = getchar();
        printf("Input char : ");
        putchar(ch);
        
        if(ch == 'q'){
            printf("\r\nQuit\r\n");
            break;
        }else if(ch == '3'){
            printf("\r\nRead Holding Register\r\n");
            modbus->receive_response(20);
        }else if(ch == '6'){
            printf("\r\nWrite Holding Register\r\n");
        }
        wait_ns(10);
    }

    running = false; 
    printf("End of Main\r\n");
    thread.join();  // blocking

    return 0;
}
