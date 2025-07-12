#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <string>

using namespace std;

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(3000);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    cout << "Waiting for connections..." << endl;

    while (true) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addlen);
        cout << "Client connected, fd: " << client_fd << endl;


            cout << "Request:\n" << client_fd << endl;

            string http_response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 38\r\n"
                "\r\n"
                "<html><body><h1>Hello from C++ server </h1></body></html>";

            write(client_fd, http_response.c_str(), http_response.size());
        }

          close(client_fd);
    
    close(server_fd);
    return 0;
}
