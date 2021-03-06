#ifndef NETDATA_WEB_CLIENT_H
#define NETDATA_WEB_CLIENT_H 1

#define DEFAULT_DISCONNECT_IDLE_WEB_CLIENTS_AFTER_SECONDS 60
extern int web_client_timeout;

#ifdef NETDATA_WITH_ZLIB
extern int web_enable_gzip,
        web_gzip_level,
        web_gzip_strategy;
#endif /* NETDATA_WITH_ZLIB */

extern int respect_web_browser_do_not_track_policy;
extern char *web_x_frame_options;

typedef enum web_client_mode {
    WEB_CLIENT_MODE_NORMAL      = 0,
    WEB_CLIENT_MODE_FILECOPY    = 1,
    WEB_CLIENT_MODE_OPTIONS     = 2,
    WEB_CLIENT_MODE_STREAM      = 3
} WEB_CLIENT_MODE;

typedef enum web_client_flags {
    WEB_CLIENT_FLAG_OBSOLETE          = 1 << 0, // if set, the listener will remove this client
    // after setting this, you should not touch
    // this web_client

    WEB_CLIENT_FLAG_DEAD              = 1 << 1, // if set, this client is dead

    WEB_CLIENT_FLAG_KEEPALIVE         = 1 << 2, // if set, the web client will be re-used

    WEB_CLIENT_FLAG_WAIT_RECEIVE      = 1 << 3, // if set, we are waiting more input data
    WEB_CLIENT_FLAG_WAIT_SEND         = 1 << 4, // if set, we have data to send to the client

    WEB_CLIENT_FLAG_DO_NOT_TRACK      = 1 << 5, // if set, we should not set cookies on this client
    WEB_CLIENT_FLAG_TRACKING_REQUIRED = 1 << 6, // if set, we need to send cookies

    WEB_CLIENT_FLAG_TCP_CLIENT        = 1 << 7, // if set, the client is using a TCP socket
    WEB_CLIENT_FLAG_UNIX_CLIENT       = 1 << 8  // if set, the client is using a UNIX socket
} WEB_CLIENT_FLAGS;

//#ifdef HAVE_C___ATOMIC
//#define web_client_flag_check(w, flag) (__atomic_load_n(&((w)->flags), __ATOMIC_SEQ_CST) & flag)
//#define web_client_flag_set(w, flag)   __atomic_or_fetch(&((w)->flags), flag, __ATOMIC_SEQ_CST)
//#define web_client_flag_clear(w, flag) __atomic_and_fetch(&((w)->flags), ~flag, __ATOMIC_SEQ_CST)
//#else
#define web_client_flag_check(w, flag) ((w)->flags & flag)
#define web_client_flag_set(w, flag)   (w)->flags |= flag
#define web_client_flag_clear(w, flag) (w)->flags &= ~flag
//#endif

#define WEB_CLIENT_IS_OBSOLETE(w) web_client_flag_set(w, WEB_CLIENT_FLAG_OBSOLETE)
#define web_client_check_obsolete(w) web_client_flag_check(w, WEB_CLIENT_FLAG_OBSOLETE)

#define WEB_CLIENT_IS_DEAD(w) web_client_flag_set(w, WEB_CLIENT_FLAG_DEAD)
#define web_client_check_dead(w) web_client_flag_check(w, WEB_CLIENT_FLAG_DEAD)

#define web_client_has_keepalive(w) web_client_flag_check(w, WEB_CLIENT_FLAG_KEEPALIVE)
#define web_client_enable_keepalive(w) web_client_flag_set(w, WEB_CLIENT_FLAG_KEEPALIVE)
#define web_client_disable_keepalive(w) web_client_flag_clear(w, WEB_CLIENT_FLAG_KEEPALIVE)

#define web_client_has_donottrack(w) web_client_flag_check(w, WEB_CLIENT_FLAG_DO_NOT_TRACK)
#define web_client_enable_donottrack(w) web_client_flag_set(w, WEB_CLIENT_FLAG_DO_NOT_TRACK)
#define web_client_disable_donottrack(w) web_client_flag_clear(w, WEB_CLIENT_FLAG_DO_NOT_TRACK)

#define web_client_has_tracking_required(w) web_client_flag_check(w, WEB_CLIENT_FLAG_TRACKING_REQUIRED)
#define web_client_enable_tracking_required(w) web_client_flag_set(w, WEB_CLIENT_FLAG_TRACKING_REQUIRED)
#define web_client_disable_tracking_required(w) web_client_flag_clear(w, WEB_CLIENT_FLAG_TRACKING_REQUIRED)

#define web_client_has_wait_receive(w) web_client_flag_check(w, WEB_CLIENT_FLAG_WAIT_RECEIVE)
#define web_client_enable_wait_receive(w) web_client_flag_set(w, WEB_CLIENT_FLAG_WAIT_RECEIVE)
#define web_client_disable_wait_receive(w) web_client_flag_clear(w, WEB_CLIENT_FLAG_WAIT_RECEIVE)

#define web_client_has_wait_send(w) web_client_flag_check(w, WEB_CLIENT_FLAG_WAIT_SEND)
#define web_client_enable_wait_send(w) web_client_flag_set(w, WEB_CLIENT_FLAG_WAIT_SEND)
#define web_client_disable_wait_send(w) web_client_flag_clear(w, WEB_CLIENT_FLAG_WAIT_SEND)

#define web_client_set_tcp(w) web_client_flag_set(w, WEB_CLIENT_FLAG_TCP_CLIENT)
#define web_client_set_unix(w) web_client_flag_set(w, WEB_CLIENT_FLAG_UNIX_CLIENT)

#define web_client_is_corkable(w) web_client_flag_check(w, WEB_CLIENT_FLAG_TCP_CLIENT)

#define URL_MAX 8192
#define ZLIB_CHUNK  16384
#define HTTP_RESPONSE_HEADER_SIZE 4096
#define COOKIE_MAX 1024
#define ORIGIN_MAX 1024

struct response {
    BUFFER *header;                 // our response header
    BUFFER *header_output;          // internal use
    BUFFER *data;                   // our response data buffer

    int code;                       // the HTTP response code

    size_t rlen;                    // if non-zero, the excepted size of ifd (input of firecopy)
    size_t sent;                    // current data length sent to output

    int zoutput;                    // if set to 1, web_client_send() will send compressed data
#ifdef NETDATA_WITH_ZLIB
    z_stream zstream;               // zlib stream for sending compressed output to client
    Bytef zbuffer[ZLIB_CHUNK];      // temporary buffer for storing compressed output
    size_t zsent;                   // the compressed bytes we have sent to the client
    size_t zhave;                   // the compressed bytes that we have received from zlib
    int zinitialized:1;
#endif /* NETDATA_WITH_ZLIB */

};

struct web_client {
    unsigned long long id;

    WEB_CLIENT_FLAGS flags;             // status flags for the client
    WEB_CLIENT_MODE mode;               // the operational mode of the client

    int tcp_cork;                       // 1 = we have a cork on the socket

    int ifd;
    int ofd;

    char client_ip[NI_MAXHOST+1];
    char client_port[NI_MAXSERV+1];

    char decoded_url[URL_MAX + 1];  // we decode the URL in this buffer
    char last_url[URL_MAX+1];       // we keep a copy of the decoded URL here

    struct timeval tv_in, tv_ready;

    char cookie1[COOKIE_MAX+1];
    char cookie2[COOKIE_MAX+1];
    char origin[ORIGIN_MAX+1];

    struct response response;

    size_t stats_received_bytes;
    size_t stats_sent_bytes;

    pthread_t thread;               // the thread servicing this client

    struct web_client *prev;
    struct web_client *next;
};

extern struct web_client *web_clients;
extern SIMPLE_PATTERN *web_allow_connections_from;
extern SIMPLE_PATTERN *web_allow_registry_from;
extern SIMPLE_PATTERN *web_allow_badges_from;
extern SIMPLE_PATTERN *web_allow_streaming_from;
extern SIMPLE_PATTERN *web_allow_netdataconf_from;

extern uid_t web_files_uid(void);
extern uid_t web_files_gid(void);

extern int web_client_permission_denied(struct web_client *w);

extern struct web_client *web_client_create(int listener);
extern struct web_client *web_client_free(struct web_client *w);
extern ssize_t web_client_send(struct web_client *w);
extern ssize_t web_client_receive(struct web_client *w);
extern void web_client_process_request(struct web_client *w);
extern void web_client_reset(struct web_client *w);

extern void *web_client_main(void *ptr);

extern int web_client_api_request_v1_data_group(char *name, int def);
extern const char *group_method2string(int group);

extern void buffer_data_options2string(BUFFER *wb, uint32_t options);

extern int mysendfile(struct web_client *w, char *filename);

#endif
