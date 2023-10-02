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
#include "wifi_helper.h"
#include "mbed-trace/mbed_trace.h"

#include "math.h"
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Sensors drivers present in the BSP library
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_magneto.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_accelero.h"

#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

class SocketDemo {
    static constexpr size_t MAX_NUMBER_OF_ACCESS_POINTS = 10;
    static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 100;

#if MBED_CONF_APP_USE_TLS_SOCKET
    static constexpr size_t REMOTE_PORT = 30000; // tls port
#else
    static constexpr size_t REMOTE_PORT = 30000; // standard HTTP port
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

    void run()
    {
        if (!_net) {
            printf("Error! No network interface found.\r\n");
            return;
        }

        /* if we're using a wifi interface run a quick scan */
        if (_net->wifiInterface()) {
            /* the scan is not required to connect and only serves to show visible access points */
            wifi_scan();

            /* in this example we use credentials configured at compile time which are used by
             * NetworkInterface::connect() but it's possible to do this at runtime by using the
             * WiFiInterface::connect() which takes these parameters as arguments */
        }

        /* connect will perform the action appropriate to the interface type to connect to the network */

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

        if (!resolve_hostname(address)) {
            return;
        }

        address.set_port(REMOTE_PORT);

        /* we are connected to the network but since we're using a connection oriented
         * protocol we still need to open a connection on the socket */

        printf("Opening connection to remote port %d\r\n", REMOTE_PORT);

        result = _socket.connect(address);
        if (result != 0) {
            printf("Error! _socket.connect() returned: %d\r\n", result);
            return;
        }

        /* exchange an HTTP request and response */

        // if (!send_http_request()) {
        //     return;
        // }

        // if (!receive_http_response()) {
        //     return;
        // }

        printf("Demo concluded successfully \r\n");

        BSP_TSENSOR_Init();
        BSP_HSENSOR_Init();
        BSP_PSENSOR_Init();

        BSP_MAGNETO_Init();
        BSP_GYRO_Init();
        BSP_ACCELERO_Init();
        int sample_num=0;
        float sensor_value = 0;
        int16_t pDataXYZ[3] = {0};
        float pGyroDataXYZ[3] = {0};
        // int SCALE_MULTIPLIER=1;
        int response=0;
        double aSensitivity = LSM6DSL_ACC_SENSITIVITY_2G/1000; /*!< accelerometer sensitivity with 2 g full scale  [mgauss/LSB]/1000 */
        double gSensitivity = LSM6DSL_GYRO_SENSITIVITY_2000DPS/1000; /**< Sensitivity value for 2000 dps full scale [mdps/LSB] /1000  */ 
        int TIME_STEP_MS = 1; //ms
        double gx, gy, gz, ax, ay, az = 0;
        double gx_10, gy_10, gz_10;
        double bias_gyro_x, bias_gyro_y, bias_gyro_z = 0;
        int bias = 1;

        BSP_GYRO_GetXYZ(pGyroDataXYZ);
        bias_gyro_x = pGyroDataXYZ[0]*gSensitivity;
        bias_gyro_y = pGyroDataXYZ[1]*gSensitivity;
        bias_gyro_z = pGyroDataXYZ[2]*gSensitivity;
        ThisThread::sleep_for(100);

        while (1){
            ++sample_num;
            char acc_json[64];
            BSP_ACCELERO_AccGetXYZ(pDataXYZ);
            BSP_GYRO_GetXYZ(pGyroDataXYZ);
            // float x = pDataXYZ[0]*SCALE_MULTIPLIER, y = pDataXYZ[1]*SCALE_MULTIPLIER,
            //     z = pDataXYZ[2]*SCALE_MULTIPLIER;
            // int len = sprintf(acc_json,"{\"x\":%f,\"y\":%f,\"z\":%f,\"s\":%d}",(float)((int)(x*10000))/10000,
            //     (float)((int)(y*10000))/10000, (float)((int)(z*10000))/10000, sample_num);
            double accel_x = pDataXYZ[0]*aSensitivity, accel_y = pDataXYZ[1]*aSensitivity, accel_z = pDataXYZ[2]*aSensitivity;
            double gyro_x = pGyroDataXYZ[0]*gSensitivity, gyro_y = pGyroDataXYZ[1]*gSensitivity, gyro_z = pGyroDataXYZ[0]*gSensitivity;

            // if (sample_num <= bias){
            //     bias_gyro_x = bias_gyro_x + gyro_x;
            //     bias_gyro_y = bias_gyro_y + gyro_y;
            //     bias_gyro_z = bias_gyro_z + gyro_z;
            // }
            // else {
                // angles based on gyro (deg/s)
            gx = gx + (gyro_x - bias_gyro_x/bias) * TIME_STEP_MS / 1000;
            gy = gy + (gyro_y - bias_gyro_y/bias) * TIME_STEP_MS / 1000;
            gz = gz + (gyro_z - bias_gyro_z/bias) * TIME_STEP_MS / 1000;
            // angles based on accelerometer
            ax = atan2(accel_y, accel_z) * 180 / M_PI;                                     // roll
            ay = atan2(-accel_x, sqrt( pow(accel_y, 2) + pow(accel_z, 2))) * 180 / M_PI;    // pitch

            // complementary filter
            gx = gx * 0.96 + ax * 0.04;
            gy = gy * 0.96 + ay * 0.04;


            int len = sprintf(acc_json,"{\"x\":%f,\"y\":%f,\"z\":%f,\"s\":%d}", gx, gy, gz, sample_num);
            response = _socket.send(acc_json,len);
            if (0 >= response){
                printf("Error seding: %d\n", response);
            }
            

            
            ThisThread::sleep_for(TIME_STEP_MS);
            // }

            // // angles based on gyro (deg/s)
            // gx = gx + (gyro_x) * TIME_STEP_MS / 1000;
            // gy = gy + (gyro_y) * TIME_STEP_MS / 1000;
            // gz = gz + (gyro_z) * TIME_STEP_MS / 1000;
            // // angles based on accelerometer
            // ax = atan2(accel_y, accel_z) * 180 / M_PI;                                     // roll
            // ay = atan2(-accel_x, sqrt( pow(accel_y, 2) + pow(accel_z, 2))) * 180 / M_PI;    // pitch

            // // complementary filter
            // gx = gx * 0.96 + ax * 0.04;
            // gy = gy * 0.96 + ay * 0.04;

            // int len = sprintf(acc_json,"{\"x\":%f,\"y\":%f,\"z\":%f,\"s\":%d}", gx, gy, gz, sample_num);
            // response = _socket.send(acc_json,len);
            // if (0 >= response){
            //     printf("Error seding: %d\n", response);
            // }
            // ThisThread::sleep_for(TIME_STEP_MS);
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
        _net->get_ip_address(&a);
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

int main() {
    printf("\r\nStarting socket demo\r\n\r\n");

#ifdef MBED_CONF_MBED_TRACE_ENABLE
    mbed_trace_init();
#endif

    SocketDemo *example = new SocketDemo();
    MBED_ASSERT(example);
    printf("\r\nStarting running\r\n\r\n");
    example->run();

    return 0;
}
