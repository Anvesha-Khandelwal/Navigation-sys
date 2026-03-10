#pragma once
// ============================================================
//  Minimal HTTP/1.1 server — POSIX sockets, no dependencies
//  Supports: GET, POST, OPTIONS, CORS, JSON bodies
// ============================================================
#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <iostream>
#include <cstring>
#include <thread>
#include <vector>

// POSIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;  // query string
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json";
    std::string body;
};

using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;

// ---- Simple JSON body parser (key:"val" or key:number) ----
std::unordered_map<std::string, std::string> parseJson(const std::string& body) {
    std::unordered_map<std::string, std::string> m;
    std::string s = body;
    // strip braces
    size_t start = s.find('{'), end = s.rfind('}');
    if (start == std::string::npos) return m;
    s = s.substr(start + 1, end - start - 1);

    size_t i = 0;
    while (i < s.size()) {
        // find key
        while (i < s.size() && s[i] != '"') i++;
        if (i >= s.size()) break;
        i++; // skip "
        size_t ks = i;
        while (i < s.size() && s[i] != '"') i++;
        std::string key = s.substr(ks, i - ks);
        i++; // skip "
        // find :
        while (i < s.size() && s[i] != ':') i++;
        i++; // skip :
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
        // value
        std::string val;
        if (i < s.size() && s[i] == '"') {
            i++;
            size_t vs = i;
            while (i < s.size() && s[i] != '"') i++;
            val = s.substr(vs, i - vs);
            i++;
        } else {
            size_t vs = i;
            while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ' ') i++;
            val = s.substr(vs, i - vs);
        }
        m[key] = val;
        while (i < s.size() && s[i] != ',') i++;
        i++;
    }
    return m;
}

std::string statusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

std::string buildResponse(const HttpResponse& res) {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << res.status << " " << statusText(res.status) << "\r\n";
    ss << "Content-Type: " << res.contentType << "; charset=utf-8\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
    ss << "Content-Length: " << res.body.size() << "\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << res.body;
    return ss.str();
}

HttpRequest parseRequest(const std::string& raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    // Request line
    std::getline(stream, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream rl(line);
    std::string path;
    rl >> req.method >> path;

    // Split path and query string
    auto qpos = path.find('?');
    if (qpos != std::string::npos) {
        req.path = path.substr(0, qpos);
        // parse query params
        std::string qs = path.substr(qpos + 1);
        std::istringstream qss(qs);
        std::string kv;
        while (std::getline(qss, kv, '&')) {
            auto ep = kv.find('=');
            if (ep != std::string::npos)
                req.params[kv.substr(0, ep)] = kv.substr(ep + 1);
        }
    } else {
        req.path = path;
    }

    // Headers
    size_t contentLength = 0;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 2);
            req.headers[key] = val;
            if (key == "Content-Length") contentLength = std::stoul(val);
        }
    }

    // Body
    if (contentLength > 0) {
        std::string bodyPart;
        std::getline(stream, bodyPart, '\0');
        req.body = raw.substr(raw.size() - contentLength);
    }

    return req;
}

class HttpServer {
public:
    void get(const std::string& path, Handler h)  { gets_[path]  = h; }
    void post(const std::string& path, Handler h) { posts_[path] = h; }

    bool listen(int port) {
        signal(SIGPIPE, SIG_IGN);
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) { perror("socket"); return false; }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind"); close(fd); return false;
        }
        if (::listen(fd, 128) < 0) {
            perror("listen"); close(fd); return false;
        }

        while (true) {
            sockaddr_in client{};
            socklen_t len = sizeof(client);
            int conn = accept(fd, (sockaddr*)&client, &len);
            if (conn < 0) continue;

            std::thread([this, conn]() {
                handleConnection(conn);
            }).detach();
        }
        return true;
    }

private:
    std::unordered_map<std::string, Handler> gets_, posts_;

    void handleConnection(int conn) {
        char buf[16384]{};
        ssize_t n = recv(conn, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { close(conn); return; }

        std::string raw(buf, n);
        HttpRequest  req = parseRequest(raw);
        HttpResponse res;

        // OPTIONS preflight
        if (req.method == "OPTIONS") {
            res.status = 204;
        } else if (req.method == "GET" && gets_.count(req.path)) {
            gets_[req.path](req, res);
        } else if (req.method == "POST" && posts_.count(req.path)) {
            posts_[req.path](req, res);
        } else if (req.method == "GET") {
            // Try static file serving from frontend/
            std::string filePath = "../frontend" + req.path;
            if (req.path == "/") filePath = "../frontend/index.html";
            FILE* f = fopen(filePath.c_str(), "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long sz = ftell(f);
                rewind(f);
                std::string content(sz, '\0');
                fread(&content[0], 1, sz, f);
                fclose(f);
                res.body = content;
                // Set content type
                if (filePath.find(".html") != std::string::npos) res.contentType = "text/html";
                else if (filePath.find(".css") != std::string::npos) res.contentType = "text/css";
                else if (filePath.find(".js") != std::string::npos)  res.contentType = "application/javascript";
            } else {
                res.status = 404;
                res.body   = R"({"error":"not found"})";
            }
        } else {
            res.status = 404;
            res.body   = R"({"error":"route not found"})";
        }

        std::string response = buildResponse(res);
        send(conn, response.c_str(), response.size(), 0);
        close(conn);
    }
};
