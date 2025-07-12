#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <sstream>

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

        vector<char> buffer(3000, 0);
        int bytes_read = read(client_fd, buffer.data(), buffer.size() - 1);

        if (bytes_read > 0) {
            // cout << "Raw Request:\n" << buffer.data() << endl;

            // === Parse the request line ===
            string request(buffer.data());
            cout << "Request:\n" << request << endl;
            size_t pos = request.find("\r\n");
            string request_line = request.substr(0, pos);

            cout << "Request Line: " << request_line << endl;
            istringstream iss(request_line); 
            string method , path , version ; 
            iss >> method >> path >> version; 

            cout << "Method: " << method << endl;
            cout << "Path: " << path << endl;

            // === Routing ===
            string response_body;

            if (path == "/") {
                response_body = "<html><body><h1>Home Page</h1></body></html>";
            } else if (path == "/about") {
                response_body = "<html><body><h1>About Page</h1></body></html>";
            } else {
                response_body = "<html><body><h1>404 Not Found</h1></body></html>";
            }

            string http_response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + to_string(response_body.size()) + "\r\n"
                "\r\n" + response_body;

            write(client_fd, http_response.c_str(), http_response.size());
        } else {
            cout << "Could not read the request or request is empty" << endl;
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
