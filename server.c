#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define CLOSESOCKET closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
typedef int socket_t;
#define CLOSESOCKET close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 16384
#define BODY_SIZE 4096

static void send_all(socket_t s, const char *data, size_t len) {
    while (len > 0) {
#ifdef _WIN32
        int n = send(s, data, (int)len, 0);
#else
        ssize_t n = send(s, data, len, 0);
#endif
        if (n <= 0) return;
        data += n;
        len -= (size_t)n;
    }
}

static void respond(socket_t client, const char *status, const char *type, const char *body) {
    char header[512];
    size_t len = strlen(body);
    snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\nConnection: close\r\n"
        "Cache-Control: no-store\r\n\r\n", status, type, len);
    send_all(client, header, strlen(header));
    send_all(client, body, len);
}

static int get_string(const char *json, const char *key, char *out, size_t cap) {
    char needle[80];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '"') return 0;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < cap) out[i++] = *p++;
    out[i] = '\0';
    return 1;
}

static int get_number(const char *json, const char *key, double *value) {
    char needle[80];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    char *end;
    *value = strtod(p, &end);
    return end != p;
}

static double factorial(double x, int *ok) {
    if (x < 0 || x > 170 || floor(x) != x) { *ok = 0; return 0; }
    double result = 1;
    for (int i = 2; i <= (int)x; i++) result *= i;
    return result;
}

static double calculate(const char *op, double a, double b, int degrees, int *ok) {
    const double pi = acos(-1.0);
    double x = degrees ? a * pi / 180.0 : a;
    *ok = 1;

    if (!strcmp(op, "add")) return a + b;
    if (!strcmp(op, "subtract")) return a - b;
    if (!strcmp(op, "multiply")) return a * b;
    if (!strcmp(op, "divide")) { if (b == 0) { *ok = 0; return 0; } return a / b; }
    if (!strcmp(op, "power")) return pow(a, b);
    if (!strcmp(op, "mod")) { if (b == 0) { *ok = 0; return 0; } return fmod(a, b); }
    if (!strcmp(op, "sin")) return sin(x);
    if (!strcmp(op, "cos")) return cos(x);
    if (!strcmp(op, "tan")) { if (fabs(cos(x)) < 1e-12) { *ok = 0; return 0; } return tan(x); }
    if (!strcmp(op, "asin")) { if (a < -1 || a > 1) { *ok = 0; return 0; } double r = asin(a); return degrees ? r * 180.0 / pi : r; }
    if (!strcmp(op, "acos")) { if (a < -1 || a > 1) { *ok = 0; return 0; } double r = acos(a); return degrees ? r * 180.0 / pi : r; }
    if (!strcmp(op, "atan")) { double r = atan(a); return degrees ? r * 180.0 / pi : r; }
    if (!strcmp(op, "sinh")) return sinh(a);
    if (!strcmp(op, "cosh")) return cosh(a);
    if (!strcmp(op, "tanh")) return tanh(a);
    if (!strcmp(op, "sqrt")) { if (a < 0) { *ok = 0; return 0; } return sqrt(a); }
    if (!strcmp(op, "cbrt")) return cbrt(a);
    if (!strcmp(op, "square")) return a * a;
    if (!strcmp(op, "cube")) return a * a * a;
    if (!strcmp(op, "log")) { if (a <= 0) { *ok = 0; return 0; } return log10(a); }
    if (!strcmp(op, "ln")) { if (a <= 0) { *ok = 0; return 0; } return log(a); }
    if (!strcmp(op, "exp")) return exp(a);
    if (!strcmp(op, "factorial")) return factorial(a, ok);
    if (!strcmp(op, "abs")) return fabs(a);
    if (!strcmp(op, "reciprocal")) { if (a == 0) { *ok = 0; return 0; } return 1.0 / a; }
    if (!strcmp(op, "percent")) return a / 100.0;
    if (!strcmp(op, "pi")) return pi;
    if (!strcmp(op, "e")) return exp(1.0);
    *ok = 0;
    return 0;
}

static void handle_api(socket_t client, const char *body) {
    char op[40] = "", angle[16] = "DEG";
    double a = 0, b = 0;
    if (!get_string(body, "op", op, sizeof(op)) || !get_number(body, "a", &a)) {
        respond(client, "400 Bad Request", "application/json",
                "{\"ok\":false,\"error\":\"Missing operation or first number.\"}");
        return;
    }
    get_number(body, "b", &b);
    get_string(body, "angle", angle, sizeof(angle));
    int ok = 1;
    double result = calculate(op, a, b, strcmp(angle, "RAD") != 0, &ok);
    if (!ok || !isfinite(result)) {
        respond(client, "400 Bad Request", "application/json",
                "{\"ok\":false,\"error\":\"Math error: check the input or operation domain.\"}");
        return;
    }
    char out[256];
    snprintf(out, sizeof(out), "{\"ok\":true,\"result\":%.15g}", result);
    respond(client, "200 OK", "application/json", out);
}

static void serve_file(socket_t client, const char *path) {
    char file_path[512];
    if (!strcmp(path, "/")) path = "/index.html";
    if (strstr(path, "..") || strchr(path, '\\')) {
        respond(client, "400 Bad Request", "text/plain", "Bad path");
        return;
    }
    snprintf(file_path, sizeof(file_path), "public%s", path);
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        respond(client, "404 Not Found", "text/plain", "File not found");
        return;
    }
    const char *type = "text/plain";
    if (strstr(path, ".html")) type = "text/html; charset=utf-8";
    else if (strstr(path, ".css")) type = "text/css; charset=utf-8";
    else if (strstr(path, ".js")) type = "application/javascript; charset=utf-8";
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *data = (char *)malloc((size_t)size + 1);
    if (!data) { fclose(f); respond(client, "500 Internal Server Error", "text/plain", "Memory error"); return; }
    size_t got = fread(data, 1, (size_t)size, f);
    fclose(f);
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Connection: close\r\n\r\n", type, got);
    send_all(client, header, strlen(header));
    send_all(client, data, got);
    free(data);
}

int main(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Winsock initialization failed.\n"); return 1;
    }
#endif
    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) { perror("socket"); return 1; }
    int opt = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(server, 10) < 0) {
        fprintf(stderr, "Could not start server on port %d. Is it already in use?\n", PORT);
        CLOSESOCKET(server); return 1;
    }
    printf("Scientific Calculator running at http://localhost:%d\n", PORT);
    printf("Press Ctrl+C to stop.\n");

    while (1) {
        socket_t client = accept(server, NULL, NULL);
        if (client < 0) continue;
        char request[BUFFER_SIZE];
#ifdef _WIN32
        int n = recv(client, request, sizeof(request) - 1, 0);
#else
        ssize_t n = recv(client, request, sizeof(request) - 1, 0);
#endif
        if (n <= 0) { CLOSESOCKET(client); continue; }
        request[n] = '\0';
        char method[8] = "", path[256] = "/";
        sscanf(request, "%7s %255s", method, path);

        if (!strcmp(method, "OPTIONS")) {
            respond(client, "204 No Content", "text/plain", "");
        } else if (!strcmp(method, "POST") && !strcmp(path, "/api/calculate")) {
            char *header_end = strstr(request, "\r\n\r\n");
            if (!header_end) {
                respond(client, "400 Bad Request", "application/json", "{\"ok\":false,\"error\":\"Bad request.\"}");
            } else {
                char *cl = strstr(request, "Content-Length:");
                int content_length = cl ? atoi(cl + 15) : 0;
                char body[BODY_SIZE];
                size_t already = (size_t)(request + n - (header_end + 4));
                if (content_length < 0 || content_length >= BODY_SIZE) {
                    respond(client, "413 Payload Too Large", "application/json", "{\"ok\":false,\"error\":\"Request too large.\"}");
                } else {
                    size_t copy = already < (size_t)content_length ? already : (size_t)content_length;
                    memcpy(body, header_end + 4, copy);
                    size_t total = copy;
                    while (total < (size_t)content_length) {
#ifdef _WIN32
                        int r = recv(client, body + total, content_length - (int)total, 0);
#else
                        ssize_t r = recv(client, body + total, (size_t)content_length - total, 0);
#endif
                        if (r <= 0) break;
                        total += (size_t)r;
                    }
                    body[total] = '\0';
                    handle_api(client, body);
                }
            }
        } else if (!strcmp(method, "GET")) {
            serve_file(client, path);
        } else {
            respond(client, "405 Method Not Allowed", "text/plain", "Method not allowed");
        }
        CLOSESOCKET(client);
    }
    CLOSESOCKET(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
