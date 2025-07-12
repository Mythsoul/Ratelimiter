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
   
    if (getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_len) == 0) { 
        return string(inet_ntoa(client_addr.sin_addr));
    }
    return "unknown";  
}

void sendHttpResponse(int client_fd, int status_code, const string& status_text, 
                     const string& content_type, const string& body) {
    ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    string response_str = response.str();
    write(client_fd, response_str.c_str(), response_str.size());
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(3000);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    cout << "Rate Limiter running on port 3000..." << endl;
    cout << "Rate limit: 5 requests per 10 seconds per IP" << endl;
    cout << "Endpoint: GET /api/check" << endl;
   
    unordered_map<string, vector<steady_clock::time_point>> ip_requests;

    while (true) {
        client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        
        cout << "Client connected, fd: " << client_fd << endl;

        vector<char> buffer(4096, 0);
        int bytes_read = read(client_fd, buffer.data(), buffer.size() - 1);

        if (bytes_read <= 0) {
            cout << "Could not read request or connection closed" << endl;
            close(client_fd);
            continue;
        }

        string request(buffer.data(), bytes_read);
        cout << "Request received:" << endl << request << endl;
        
        istringstream request_stream(request);
        string method, path, version;
        request_stream >> method >> path >> version;
        
        string client_ip = getClientIP(client_fd);
        cout << "Client IP: " << client_ip << endl;
        
        if (client_ip == "unknown") {
            cout << "Could not determine client IP" << endl;
            sendHttpResponse(client_fd, 400, "Bad Request", "text/plain", 
                            "Could not determine client IP");
            close(client_fd);
            continue;
        }
        
        if (method == "GET" && path == "/api/check") { 
            // Rate limiting logic
            auto now = steady_clock::now(); 
            auto& timestamps = ip_requests[client_ip];
            
            cout << "Timestamps before cleanup: " << timestamps.size() << endl;
            
            // Remove timestamps older than 10 seconds
            timestamps.erase(
                remove_if(timestamps.begin(), timestamps.end(),
                         [now](const steady_clock::time_point& timestamp) {
                             return duration_cast<seconds>(now - timestamp).count() > 10;
                         }),
                timestamps.end()
            );
            
            cout << "Timestamps after cleanup: " << timestamps.size() << endl;
            
            // Check if rate limit exceeded
            if (timestamps.size() >= 5) {
                cout << "Rate limit exceeded for " << client_ip << endl;
                sendHttpResponse(client_fd, 429, "Too Many Requests", "text/plain",
                               "Rate limit exceeded. Maximum 5 requests per 10 seconds.");
            } else {
                // Add current timestamp and allow request
                timestamps.push_back(now);
                cout << "Request allowed for " << client_ip << " (" << timestamps.size() << "/5)" << endl;
                sendHttpResponse(client_fd, 200, "OK", "text/plain", "Request allowed");
            }
        } else {
            cout << "Unsupported method or path: " << method << " " << path << endl;
            sendHttpResponse(client_fd, 404, "Not Found", "text/plain",
                            "Endpoint not found. Use GET /api/check");
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}