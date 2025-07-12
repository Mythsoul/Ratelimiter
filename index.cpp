#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <arpa/inet.h>
#include <algorithm> 

using namespace std;
using namespace std::chrono;

string getClientIP(int client_fd) { 
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    // Get the client's IP address
    
    if (getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_len) == 0) { 
        return string(inet_ntoa(client_addr.sin_addr));
    }
    return "unknown";  
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(3000);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    cout << "Rate Limiter running on port 3000..." << endl;
   
    unordered_map<string, vector<steady_clock::time_point>> ip_requests;

    while (true) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addlen);
        cout << "Client connected, fd: " << client_fd << endl;

        vector<char> buffer(3000, 0);
        int bytes_read = read(client_fd, buffer.data(), buffer.size() - 1);

        if (bytes_read > 0) {
            string request(buffer.data());
            cout << "Request:\n" << request << endl;
  
            string client_ip = getClientIP(client_fd);
            cout << "Client IP: " << client_ip << endl;

            // === Rate limiting logic ===
            auto now = steady_clock::now(); 
            auto &timestamps = ip_requests[client_ip];
            cout << "timestamps" << endl << timestamps.size();
            // Remove timestamps older than 10 seconds
            auto it = timestamps.begin();
            while(it != timestamps.end()) {
                if(duration_cast<seconds> (now - *it).count()> 10 ){ 
                    it = timestamps.erase(it);

                }else{ 
                    ++it; 
                }
            }
            // Check if client has exceeded rate limit (5 requests per 10 seconds)
            if (timestamps.size() >= 5) {
                cout << "Client " << client_ip << " has exceeded the rate limit" << endl;
                
                string http_response =
                    "HTTP/1.1 429 Too Many Requests\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 28\r\n"
                    "\r\n"
                    "Too Many Requests. Slow down!";

                write(client_fd, http_response.c_str(), http_response.size());
            } else {
                // Add current timestamp and allow request
                timestamps.push_back(now);
                
                cout << "Request allowed for " << client_ip << " (" << timestamps.size() << "/5)" << endl;
                
                string http_response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 18\r\n"
                    "\r\n"
                    "Request Allowed.\n";

                write(client_fd, http_response.c_str(), http_response.size());
            }

        } else {
            cout << "Could not read the request or request is empty" << endl;
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}