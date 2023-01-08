#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <cassert>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <iostream>
#include <signal.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#define DEFAULT_NODE_IPV4 "127.0.0.1"
#define DEFAULT_NODE_PORT 8080
#define DEFAULT_NODE_VERSION "0.3.3Aa6"
#define DEFAULT_NODE_PROTOCOL 2
#define DEFAULT_BUFFER_SIZE 1024

#define LOG(prefix, msg) { \
    std::time_t ts = std::time(nullptr); \
    std::cout << ts \
        << "(" << std::setfill('0') << std::setw(3) << ts % 600 << ")" \
        << prefix << msg << std::endl; \
}

struct cb_context {
    unsigned short conn_status;
    evutil_socket_t socket_fd;
    struct event_base *ev_base;
    struct bufferevent *bev;
    struct event *sigint_ev;
    struct event *ping_ev;
};

static void cleanup_context(struct cb_context *ctx);
static void signal_cb(evutil_socket_t socket_fd, short what, void *cbarg);
static void timer_cb(evutil_socket_t socket_fd, short what, void *cbarg);
static void event_cb(struct bufferevent *bev, short what, void *cbarg);
static void read_cb(struct bufferevent *bev, void *cbarg);
static void write_cb(struct bufferevent *bev, void *cbarg);
static int send_hello(struct bufferevent *bev);
static int send_ping(struct bufferevent *bev, short conn_status);
static int send_pong(struct bufferevent *bev, short conn_status);

std::string g_peer_ipv4 = DEFAULT_NODE_IPV4;
unsigned short g_peer_port = DEFAULT_NODE_PORT;
std::string g_node_ipv4 = DEFAULT_NODE_IPV4;
unsigned short g_node_port = DEFAULT_NODE_PORT;
std::string g_node_version = DEFAULT_NODE_VERSION;
unsigned char g_node_protocol = DEFAULT_NODE_PROTOCOL;
struct timeval g_read_timeout = {200, 0};
struct timeval g_write_timeout = {200, 0};
struct timeval g_ping_interval = {10, 0};

int main(int argc, char **argv) {
    if (argc > 1) {
        g_peer_ipv4 = argv[1];
        if (argc > 2) {
            g_peer_port = std::atoi(argv[2]);
        }
    } else {
        std::cout << "Usage: " << argv[0] << " remote-node-ipv4 [port]" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "PING-PONG "
            << "(local)"
            << g_node_ipv4 << ":" << g_node_port
            << " <-------> "
            << g_peer_ipv4 << ":" << g_peer_port
            << "(remote)"
            << std::endl;
    std::cout << "Press Ctrl+C to break!" << std::endl;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(g_peer_port);
    inet_pton(AF_INET, g_peer_ipv4.c_str(), &sin.sin_addr.s_addr);
    struct cb_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ev_base = event_base_new();
    if(!ctx.ev_base) {
        LOG("", "Failed creating an event base!");
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    ctx.sigint_ev = evsignal_new(ctx.ev_base, SIGINT, signal_cb, (void *)&ctx);
    if (!ctx.sigint_ev) {
        LOG("", "Failed creating an interupting signal event!")
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    ctx.ping_ev = evtimer_new(ctx.ev_base, timer_cb, (void *)&ctx);
    if (!ctx.ping_ev) {
        LOG("", "Failed creating a timer event!");
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    if (evsignal_add(ctx.sigint_ev, NULL)<0) {
        LOG("", "Failed adding an interupting signal event!");
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    ctx.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx.socket_fd < 0) {
        LOG("", "Failed creating a client socket!");
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    if (evutil_make_listen_socket_reuseable(ctx.socket_fd) < 0 ) {
        LOG("", "Failed to set client socket reuseable!");
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    if (evutil_make_socket_nonblocking(ctx.socket_fd) < 0) {
        LOG("", "Failed to set server socket non-blocking!");
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    ctx.bev = bufferevent_socket_new(ctx.ev_base, ctx.socket_fd, BEV_OPT_CLOSE_ON_FREE);
    if(!ctx.bev) {
        LOG("", "Failed creating a bufferevent client socket!");
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    bufferevent_set_timeouts(ctx.bev, &g_read_timeout, &g_write_timeout);
    bufferevent_setcb(ctx.bev, read_cb, write_cb, event_cb, (void *)&ctx);
    bufferevent_enable(ctx.bev, EV_READ | EV_WRITE);
    if (bufferevent_socket_connect(ctx.bev,
            (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        cleanup_context(&ctx);
        return EXIT_FAILURE;
    }
    event_base_dispatch(ctx.ev_base);
    cleanup_context(&ctx);
    LOG( "", "Done!" );
    return EXIT_SUCCESS;
}

static void
write_cb(struct bufferevent *bev, void *cbarg) {
    struct cb_context *ctx = (struct cb_context *)cbarg;
}

static void
read_cb(struct bufferevent *bev, void *cbarg) {
    struct cb_context *ctx = (struct cb_context *)cbarg;
    struct evbuffer *evb_input = bufferevent_get_input(bev);
    char *line = NULL;
    while ((line = evbuffer_readln(evb_input, NULL, EVBUFFER_EOL_ANY))) {
        LOG(" <--- ", line);
        if (std::strstr(line, "Closing NODE")) {
            ctx->conn_status = 0;
            event_base_loopbreak(ctx->ev_base);
        } else {
            ctx->conn_status = 2;
            if (std::strstr(line, " $PING ")) {
                if (send_pong(bev, ctx->conn_status) < 0) {
                    ctx->conn_status = 0;
                    event_base_loopbreak(ctx->ev_base);
                }
            } else if (std::strstr(line, " $PONG ")) {
                // no need to reply pong command, do nothing here
            }
            evtimer_del(ctx->ping_ev);
            if (evtimer_add(ctx->ping_ev, &g_ping_interval) < 0) {
                LOG("", "Failed adding a timer event!");
                ctx->conn_status = 0;
                event_base_loopbreak(ctx->ev_base);
            }
        }
        std::free(line);
    }
}

static void
event_cb(struct bufferevent *bev, short what, void *cbarg) {
    struct cb_context *ctx = (struct cb_context *)cbarg;
    if (what & BEV_EVENT_CONNECTED) {
        ctx->conn_status = 0;
        if (send_hello(bev) < 0
                || send_ping(bev, ctx->conn_status) < 0) {
            ctx->conn_status = 0;
            event_base_loopbreak(ctx->ev_base);
        } else {
            ctx->conn_status = 1;
            if (evtimer_add(ctx->ping_ev, &g_ping_interval) < 0) {
                LOG("", "Failed adding a timer event!");
                ctx->conn_status = 0;
                event_base_loopbreak(ctx->ev_base);
            }
        }
    } else if (what & BEV_EVENT_TIMEOUT) {
        if (what & BEV_EVENT_READING) {
            LOG("", "Reading timeout!");
        }
        if (what & BEV_EVENT_WRITING) {
            LOG("", "Writing timeout!");
        }
        ctx->conn_status = 0;
        event_base_loopbreak(ctx->ev_base);
        //TODO Should try reconnecting here instead!
    } else if (what & BEV_EVENT_EOF) {
        LOG("", "Connection reset by peer!");
        ctx->conn_status = 0;
        // Dump remaining buffer
        struct evbuffer *evb_input = bufferevent_get_input(bev);
        char *line = NULL;
        while ((line = evbuffer_readln(evb_input, NULL, EVBUFFER_EOL_ANY))) {
            LOG(" <--- ", line);
            std::free(line);
        }
        event_base_loopbreak(ctx->ev_base);
    } else if (what & BEV_EVENT_ERROR) {
        LOG( "An error occured: ",
            evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        ctx->conn_status = 0;
        event_base_loopbreak(ctx->ev_base);
    }
}

static void
timer_cb(evutil_socket_t socket_fd, short what, void *cbarg) {
    struct cb_context *ctx = (struct cb_context *)cbarg;
    if (send_ping(ctx->bev, ctx->conn_status) < 0) {
        ctx->conn_status = 0;
        event_base_loopbreak(ctx->ev_base);
    } else {
        evtimer_del(ctx->ping_ev);
        if (evtimer_add(ctx->ping_ev, &g_ping_interval) < 0) {
            LOG("", "Failed adding a timer event!");
            ctx->conn_status = 0;
            event_base_loopbreak(ctx->ev_base);
        }
    }
}

static void
signal_cb(evutil_socket_t socket_fd, short what, void *cbarg) {
    assert(what & EV_SIGNAL);
    static struct timeval delay_tv = { 1, 0 };
    struct cb_context *ctx = (struct cb_context *)cbarg;
    std::cout << "Interrupted (Ctrl-C pressed)!" << std::endl;
    std::cout << "Exiting cleanly in 1 seconds." << std::endl;
    event_base_loopexit(ctx->ev_base, &delay_tv);
}

static void
cleanup_context(struct cb_context *ctx) {
    LOG("", "Cleaning up...");
    assert(ctx);
    if (ctx->sigint_ev) event_free(ctx->sigint_ev);
    if (ctx->ping_ev) event_free(ctx->ping_ev);
    if (ctx->bev) bufferevent_free(ctx->bev);
    if (ctx->ev_base) event_base_free(ctx->ev_base);
}

static int
send_hello(struct bufferevent *bev) {
    assert(bev);
    char buffer[DEFAULT_BUFFER_SIZE];
    std::size_t hello_size = std::snprintf(
            buffer, DEFAULT_BUFFER_SIZE-1,
            "PSK %s %s %llu\n",
            g_node_ipv4.c_str(),
            g_node_version.c_str(),
            (long long)time(0));
    if (bufferevent_write(bev, buffer, strlen(buffer)) < 0 ) {
        LOG("", "Failed sending hello command!");
        return -1;
    }
    buffer[hello_size-1]='\0'; // eliminate the newline char for output
    LOG(" ---> ", buffer);
    return 0;
}

static int
send_ping(struct bufferevent *bev, short conn_status) {
    assert(bev);
    char buffer[DEFAULT_BUFFER_SIZE];
    std::size_t ping_size = std::snprintf(
            buffer, DEFAULT_BUFFER_SIZE-1,
            "PSK %u %s %llu "
            "$PING "                                // Magic string
            "1 "                                    // Current connections
            "0 "                                    // Block number
            "4E8A4743AA6083F3833DDA1216FE3717 "     // Block Hash (Genesis block hash)
            "D41D8CD98F00B204E9800998ECF8427E "     // Hash summary.psk (This is the MD5 hash for empty)
            "0 "                                    // Pending Orders
            "D41D8CD98F00B204E9800998ECF8427E "     // Hash blchhead.nos (This is the MD5 hash for empty)
            "%hu "                                  // Connections status [0=Disconnected,1=Connecting,2=Connected,3=Updated]
            "%hu "                                  // Node IP port
            "D41D8 "                                // Hash(5) masternodes.txt (This is the MD5 hash for empty)
            "0 "                                    // MNs Count
            "00000000000000000000000000000000 "     // NMsData diff/ Besthash diff
            "0 "                                    // Checked Master Nodes
            "D41D8CD98F00B204E9800998ECF8427E "     // Hash gvts.psk (This is the MD5 hash for empty)
            "D41D8\n",                              // Hash(5) CFGs
            g_node_protocol,
            g_node_version.c_str(),
            (long long)time(0),
            g_node_port,
            conn_status);
    if (bufferevent_write(bev, buffer, strlen(buffer)) < 0 ) {
        LOG("", "Failed sending ping command!");
        return -1;
    }
    buffer[ping_size-1]='\0'; // eliminate the newline char for output
    LOG(" ---> ", buffer);
    return 0;
}

static int
send_pong(struct bufferevent *bev, short conn_status) {
    assert(bev);
    char buffer[DEFAULT_BUFFER_SIZE];
    std::size_t pong_size = std::snprintf(
            buffer, DEFAULT_BUFFER_SIZE-1,
            "PSK %u %s %llu "
            "$PONG "                                // Magic string
            "0 "                                    // Current connections
            "0 "                                    // Block number
            "4E8A4743AA6083F3833DDA1216FE3717 "     // Block Hash (Genesis block hash)
            "D41D8CD98F00B204E9800998ECF8427E "     // Hash summary.psk (This is the MD5 hash for empty)
            "0 "                                    // Pending Orders
            "D41D8CD98F00B204E9800998ECF8427E "     // Hash blchhead.nos (This is the MD5 hash for empty)
            "%hu "                                    // Connections status [0=Disconnected,1=Connecting,2=Connected,3=Updated]
            "%hu "                                  // Node IP port
            "D41D8 "                                // Hash(5) masternodes.txt (This is the MD5 hash for empty)
            "0 "                                    // MNs Count
            "00000000000000000000000000000000 "     // NMsData diff/ Besthash diff
            "0 "                                    // Checked Master Nodes
            "D41D8CD98F00B204E9800998ECF8427E "     // Hash gvts.psk (This is the MD5 hash for empty)
            "D41D8\n",                              // Hash(5) CFGs
            g_node_protocol,
            g_node_version.c_str(),
            (long long)time(0),
            g_node_port,
            conn_status);
    if (bufferevent_write(bev, buffer, strlen(buffer)) < 0 ) {
        LOG("", "Failed sending pong command!");
        return -1;
    }
    buffer[pong_size-1]='\0'; // eliminate the newline char for output
    LOG(" ---> ", buffer);
    return 0;
}
