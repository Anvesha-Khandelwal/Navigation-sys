#pragma once
// ============================================================
//  Minimal HTTP/1.1 server — Winsock2 (Windows) compatible
//  No external dependencies. Works on Windows + Linux/Mac.
// ============================================================
#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <iostream>
#include <cstring>
#include <thread>

// ---- Platform detection ----
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
  #define CLOSE_SOCKET(s) closesocket(s)
  #define SOCK_ERR        SOCKET_ERROR
  typedef SOCKET sock_t;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <signal.h>
  #define CLOSE_SOCKET(s) ::close(s)
  #define SOCK_ERR        (-1)
  #define INVALID_SOCKET  (-1)
  typedef int sock_t;
#endif

// ============================================================
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
};

struct HttpResponse {
    int         status      = 200;
    std::string contentType = "application/json";
    std::string body;
};

using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;

// ---- Simple flat JSON body parser { "key":"val", "key2":123 } ----
std::unordered_map<std::string, std::string> parseJson(const std::string& body) {
    std::unordered_map<std::string, std::string> m;
    size_t start = body.find('{');
    size_t end   = body.rfind('}');
    if (start == std::string::npos) return m;
    std::string s = body.substr(start + 1, end - start - 1);

    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && s[i] != '"') i++;
        if (i >= s.size()) break;
        i++;
        size_t ks = i;
        while (i < s.size() && s[i] != '"') i++;
        std::string key = s.substr(ks, i - ks);
        i++;
        while (i < s.size() && s[i] != ':') i++;
        i++;
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
        std::string val;
        if (i < s.size() && s[i] == '"') {
            i++;
            size_t vs = i;
            while (i < s.size() && s[i] != '"') i++;
            val = s.substr(vs, i - vs);
            i++;
        } else {
            size_t vs = i;
            while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ' ' && s[i] != '\r' && s[i] != '\n') i++;
            val = s.substr(vs, i - vs);
        }
        if (!key.empty()) m[key] = val;
        while (i < s.size() && s[i] != ',') i++;
        if (i < s.size()) i++;
    }
    return m;
}

static std::string statusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

static std::string buildHttpResponse(const HttpResponse& res) {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << res.status << " " << statusText(res.status) << "\r\n";
    ss << "Content-Type: "  << res.contentType << "; charset=utf-8\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
    ss << "Content-Length: " << res.body.size() << "\r\n";
    ss << "Connection: close\r\n\r\n";
    ss << res.body;
    return ss.str();
}

static HttpRequest parseRequest(const std::string& raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    std::getline(stream, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream rl(line);
    std::string path;
    rl >> req.method >> path;

    auto qpos = path.find('?');
    if (qpos != std::string::npos) {
        req.path = path.substr(0, qpos);
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

    size_t contentLength = 0;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            while (!val.empty() && val.front() == ' ') val.erase(val.begin());
            req.headers[key] = val;
            if (key == "Content-Length") {
                try { contentLength = std::stoul(val); } catch(...) {}
            }
        }
    }

    if (contentLength > 0 && raw.size() >= contentLength) {
        req.body = raw.substr(raw.size() - contentLength);
    }

    return req;
}

// ============================================================
class HttpServer {
public:
    void get (const std::string& path, Handler h) { gets_[path]  = h; }
    void post(const std::string& path, Handler h) { posts_[path] = h; }

    bool listen(int port) {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            std::cerr << "WSAStartup failed\n"; return false;
        }
#else
        signal(SIGPIPE, SIG_IGN);
#endif
        sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCKET) { std::cerr << "socket() failed\n"; return false; }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons((u_short)port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) == SOCK_ERR) {
            std::cerr << "bind() failed — is port " << port << " already in use?\n";
            CLOSE_SOCKET(fd); return false;
        }
        if (::listen(fd, 64) == SOCK_ERR) {
            std::cerr << "listen() failed\n";
            CLOSE_SOCKET(fd); return false;
        }

        while (true) {
            sockaddr_in client{};
            socklen_t   len  = sizeof(client);
            sock_t      conn = accept(fd, (sockaddr*)&client, &len);
            if (conn == INVALID_SOCKET) continue;

            std::thread([this, conn]() {
                handleConnection(conn);
            }).detach();
        }

#ifdef _WIN32
        WSACleanup();
#endif
        return true;
    }

private:
    std::unordered_map<std::string, Handler> gets_, posts_;

    void handleConnection(sock_t conn) {
        char buf[32768]{};
        int n = recv(conn, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { CLOSE_SOCKET(conn); return; }

        std::string     raw(buf, n);
        HttpRequest     req = parseRequest(raw);
        HttpResponse    res;

        if (req.method == "OPTIONS") {
            res.status = 204;
        } else if (req.method == "GET" && gets_.count(req.path)) {
            gets_[req.path](req, res);
        } else if (req.method == "POST" && posts_.count(req.path)) {
            posts_[req.path](req, res);
        } else if (req.method == "GET") {
            std::string filePath = "../frontend" + req.path;
            if (req.path == "/") filePath = "../frontend/index.html";
            FILE* f = fopen(filePath.c_str(), "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long sz = ftell(f);
                rewind(f);
                if (sz > 0) {
                    std::string content(sz, '\0');
                    (void)fread(&content[0], 1, sz, f);
                    res.body = content;
                }
                fclose(f);
                if      (filePath.find(".html") != std::string::npos) res.contentType = "text/html";
                else if (filePath.find(".css")  != std::string::npos) res.contentType = "text/css";
                else if (filePath.find(".js")   != std::string::npos) res.contentType = "application/javascript";
            } else {
                res.status = 404;
                res.body   = R"({"error":"file not found"})";
            }
        } else {
            res.status = 404;
            res.body   = R"({"error":"route not found"})";
        }

        std::string response = buildHttpResponse(res);
        send(conn, response.c_str(), (int)response.size(), 0);
        CLOSE_SOCKET(conn);
    }
};
