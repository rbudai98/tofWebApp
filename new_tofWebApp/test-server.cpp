#include <getopt.h>
#include <libwebsockets.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32)
#else
#include <unistd.h>
#endif

int close_testing;
int max_poll_elements;
int debug_level = 7;

volatile int force_exit = 0, dynamic_vhost_enable = 0;
struct lws_vhost *dynamic_vhost;
struct lws_context *context;
struct lws_plat_file_ops fops_plat;
static int test_options;

/* http server gets files from this path */
#define LOCAL_RESOURCE_PATH "/home/analog/Workspace/ToF/apps/http-server"

char *resource_path = LOCAL_RESOURCE_PATH;
#if defined(LWS_WITH_TLS) && defined(LWS_HAVE_SSL_CTX_set1_param)
char crl_path[1024] = "";
#endif

#define LWS_PLUGIN_STATIC
#include "plugins/protocol_tof.cpp"

static int lws_callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len) {
    const unsigned char *c;
    char buf[1024];
    int n = 0, hlen;

    switch (reason) {
    case LWS_CALLBACK_HTTP:

        /* non-mount-handled accesses will turn up here */

        /* dump the headers */

        do {
            c = lws_token_to_string((lws_token_indexes)n);
            if (!c) {
                n++;
                continue;
            }

            hlen = lws_hdr_total_length(wsi, (lws_token_indexes)n);
            if (!hlen || hlen > (int)sizeof(buf) - 1) {
                n++;
                continue;
            }

            if (lws_hdr_copy(wsi, buf, sizeof buf, (lws_token_indexes)n) < 0)
                fprintf(stderr, "    %s (too big)\n", (char *)c);
            else {
                buf[sizeof(buf) - 1] = '\0';

                fprintf(stderr, "    %s = %s\n", (char *)c, buf);
            }
            n++;
        } while (c);

        /* dump the individual URI Arg parameters */

        n = 0;
        while (lws_hdr_copy_fragment(wsi, buf, sizeof(buf),
                                     WSI_TOKEN_HTTP_URI_ARGS, n) > 0) {
            lwsl_notice("URI Arg %d: %s\n", ++n, buf);
        }

        if (lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL))
            return -1;

        if (lws_http_transaction_completed(wsi))
            return -1;

        return 0;
    default:
        break;
    }

    return lws_callback_http_dummy(wsi, reason, user, in, len);
}

/* list of supported protocols and callbacks */

static struct lws_protocols protocols[] = {
    /* first protocol must always be HTTP handler */

    {
        "http-only",
        lws_callback_http,
        0,
        0,
    },
    {"dumb-increment-protocol", callback_tof, sizeof(struct pss__tof),
     300, /* rx buf size must be >= permessage-deflate rx size */
     0, NULL, 0},
    {NULL, NULL, 0, 0} /* terminator */
};

/* this shows how to override the lws file operations.	You don't need
 * to do any of this unless you have a reason (eg, want to serve
 * compressed files without decompressing the whole archive)
 */
static lws_fop_fd_t test_server_fops_open(const struct lws_plat_file_ops *fops,
                                          const char *vfs_path,
                                          const char *vpath,
                                          lws_fop_flags_t *flags) {
    lws_fop_fd_t fop_fd;

    /* call through to original platform implementation */
    fop_fd = fops_plat.open(fops, vfs_path, vpath, flags);

    if (fop_fd)
        lwsl_info("%s: opening %s, ret %p, len %lu\n", __func__, vfs_path,
                  fop_fd, (long)lws_vfs_get_length(fop_fd));
    else
        lwsl_info("%s: open %s failed\n", __func__, vfs_path);

    return fop_fd;
}

void sighandler(int sig) {
#if !defined(WIN32) && !defined(_WIN32)
    /* because windows is too dumb to have SIGUSR1... */
    if (sig == SIGUSR1) {
        /*
		 * For testing, you can fire a SIGUSR1 at the test server
		 * to toggle the existence of an identical server on
		 * port + 1
		 */
        dynamic_vhost_enable ^= 1;
        lws_cancel_service(context);
        lwsl_notice("SIGUSR1: dynamic_vhost_enable: %d\n",
                    dynamic_vhost_enable);
        return;
    }
#endif
    force_exit = 1;
    lws_cancel_service(context);
}

static const struct lws_extension exts[] = {
    {"permessage-deflate", lws_extension_callback_pm_deflate,
     "permessage-deflate"},
    {NULL, NULL, NULL /* terminator */}};

/*
 * mount handlers for sections of the URL space
 */

static const struct lws_http_mount mount_ziptest = {
    NULL,       /* linked-list pointer to next*/
    "/ziptest", /* mountpoint in URL namespace on this vhost */
    LOCAL_RESOURCE_PATH "/candide.zip", /* handler */
    NULL,                               /* default filename if none given */
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    0,
    0,
    0,
    0,
    LWSMPRO_FILE, /* origin points to a callback */
    8,            /* strlen("/ziptest"), ie length of the mountpoint */
    NULL,

    {NULL, NULL} // sentinel
};

static const struct lws_http_mount mount_post = {
    (struct lws_http_mount *)&mount_ziptest, /* linked-list pointer to next*/
    "/formtest",          /* mountpoint in URL namespace on this vhost */
    "protocol-post-demo", /* handler */
    NULL,                 /* default filename if none given */
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    0,
    0,
    0,
    0,
    LWSMPRO_CALLBACK, /* origin points to a callback */
    9,                /* strlen("/formtest"), ie length of the mountpoint */
    NULL,

    {NULL, NULL} // sentinel
};

/*
 * mount a filesystem directory into the URL space at /
 * point it to our /usr/share directory with our assets in
 * stuff from here is autoserved by the library
 */

static const struct lws_http_mount mount = {
    (struct lws_http_mount *)&mount_post, /* linked-list pointer to next*/
    "/",                 /* mountpoint in URL namespace on this vhost */
    LOCAL_RESOURCE_PATH, /* where to go on the filesystem for that */
    "test.html",         /* default filename if none given */
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    0,
    0,
    0,
    0,
    LWSMPRO_FILE, /* mount type is a directory in a filesystem */
    1,            /* strlen("/"), ie length of the mountpoint */
    NULL,

    {NULL, NULL} // sentinel
};

static const struct lws_protocol_vhost_options pvo_options = {
    NULL, NULL, "options",      /* pvo name */
    (const char *)&test_options /* pvo value */
};

static const struct lws_protocol_vhost_options pvo = {
    NULL,                      /* "next" pvo linked-list */
    &pvo_options,              /* "child" pvo linked-list */
    "dumb-increment-protocol", /* protocol name we belong to on this vhost */
    ""                         /* ignored */
};

static struct option options[] = {
    {"help", no_argument, NULL, 'h'},
    {"debug", required_argument, NULL, 'd'},
    {"port", required_argument, NULL, 'p'},
    {"ssl", no_argument, NULL, 's'},
    {"allow-non-ssl", no_argument, NULL, 'a'},
    {"interface", required_argument, NULL, 'i'},
    {"closetest", no_argument, NULL, 'c'},
    {"ssl-cert", required_argument, NULL, 'C'},
    {"ssl-key", required_argument, NULL, 'K'},
    {"ssl-ca", required_argument, NULL, 'A'},
#if defined(LWS_WITH_TLS)
    {"ssl-verify-client", no_argument, NULL, 'v'},
#if defined(LWS_HAVE_SSL_CTX_set1_param)
    {"ssl-crl", required_argument, NULL, 'R'},
#endif
#endif
    {"libev", no_argument, NULL, 'e'},
    {"unix-socket", required_argument, NULL, 'U'},
#ifndef LWS_NO_DAEMONIZE
    {"daemonize", no_argument, NULL, 'D'},
#endif
    {"pingpong-secs", required_argument, NULL, 'P'},
    {NULL, 0, 0, 0}};

int main(int argc, char **argv) {
    struct lws_context_creation_info info;
    struct lws_vhost *vhost;
    char interface_name[128] = "";
    const char *iface = NULL;
    char cert_path[1024] = "";
    char key_path[1024] = "";
    char ca_path[1024] = "";
    int uid = -1, gid = -1;
    int use_ssl = 0;
    int pp_secs = 0;
    int opts = 0;
    int n = 0;
#ifndef LWS_NO_DAEMONIZE
    int daemonize = 0;
#endif

    /*
	 * take care to zero down the info struct, he contains random garbaage
	 * from the stack otherwise
	 */
    memset(&info, 0, sizeof info);
    info.port = 7681;

    while (n >= 0) {
        n = getopt_long(argc, argv, "eci:hsap:d:DC:K:A:R:vu:g:P:kU:n", options,
                        NULL);
        if (n < 0)
            continue;
        switch (n) {
        case 'e':
            opts |= LWS_SERVER_OPTION_LIBEV;
            break;
#ifndef LWS_NO_DAEMONIZE
        case 'D':
            daemonize = 1;
            break;
#endif
        case 'u':
            uid = atoi(optarg);
            break;
        case 'g':
            gid = atoi(optarg);
            break;
        case 'd':
            debug_level = atoi(optarg);
            break;
        case 'n':
            /* no dumb increment send */
            test_options |= 1;
            break;
        case 's':
            use_ssl = 1;
            opts |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
            break;
        case 'a':
            opts |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
            break;
        case 'p':
            info.port = atoi(optarg);
            break;
        case 'i':
            lws_strncpy(interface_name, optarg, sizeof interface_name);
            iface = interface_name;
            break;
        case 'U':
            lws_strncpy(interface_name, optarg, sizeof interface_name);
            iface = interface_name;
            opts |= LWS_SERVER_OPTION_UNIX_SOCK;
            break;
        case 'k':
            info.bind_iface = 1;
#if defined(LWS_HAVE_SYS_CAPABILITY_H) && defined(LWS_HAVE_LIBCAP)
            info.caps[0] = CAP_NET_RAW;
            info.count_caps = 1;
#endif
            break;
        case 'c':
            close_testing = 1;
            fprintf(stderr, " Close testing mode -- closes on "
                            "client after 50 dumb increments"
                            "and suppresses lws_mirror spam\n");
            break;
        case 'C':
            lws_strncpy(cert_path, optarg, sizeof(cert_path));
            break;
        case 'K':
            lws_strncpy(key_path, optarg, sizeof(key_path));
            break;
        case 'A':
            lws_strncpy(ca_path, optarg, sizeof(ca_path));
            break;
        case 'P':
            pp_secs = atoi(optarg);
            lwsl_notice("Setting pingpong interval to %d\n", pp_secs);
            break;
#if defined(LWS_WITH_TLS)
        case 'v':
            use_ssl = 1;
            opts |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
            break;

#if defined(LWS_HAVE_SSL_CTX_set1_param)
        case 'R':
            lws_strncpy(crl_path, optarg, sizeof(crl_path));
            break;
#endif
#endif
        case 'h':
            fprintf(stderr, "Usage: test-server "
                            "[--port=<p>] [--ssl] "
                            "[-d <log bitfield>]\n");
            exit(1);
        }
    }

#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
    /*
	 * normally lock path would be /var/lock/lwsts or similar, to
	 * simplify getting started without having to take care about
	 * permissions or running as root, set to /tmp/.lwsts-lock
	 */
    if (daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
        fprintf(stderr, "Failed to daemonize\n");
        return 10;
    }
#endif

    signal(SIGINT, sighandler);
#if !defined(WIN32) && !defined(_WIN32)
    /* because windows is too dumb to have SIGUSR1... */
    /* dynamic vhost create / destroy toggle (on port + 1) */
    signal(SIGUSR1, sighandler);
#endif

    /* tell the library what debug level to emit and to send it to stderr */
    lws_set_log_level(debug_level, NULL);

    lwsl_notice("libwebsockets test server - license LGPL2.1+SLE\n");

    printf("Using resource path \"%s\"\n", resource_path);
    info.iface = iface;
    info.protocols = protocols;
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;
    info.ws_ping_pong_interval = pp_secs;

    if (use_ssl) {
        if (strlen(resource_path) > sizeof(cert_path) - 32) {
            lwsl_err("resource path too long\n");
            return -1;
        }
        if (!cert_path[0])
            sprintf(cert_path, "%s/libwebsockets-test-server.pem",
                    resource_path);
        if (strlen(resource_path) > sizeof(key_path) - 32) {
            lwsl_err("resource path too long\n");
            return -1;
        }
        if (!key_path[0])
            sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
                    resource_path);

        info.ssl_cert_filepath = cert_path;
        info.ssl_private_key_filepath = key_path;
        if (ca_path[0])
            info.ssl_ca_filepath = ca_path;
    }
    info.gid = gid;
    info.uid = uid;
    info.options = opts | LWS_SERVER_OPTION_VALIDATE_UTF8 |
                   LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    info.extensions = exts;
    info.timeout_secs = 5;
    info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
                           "ECDHE-RSA-AES256-GCM-SHA384:"
                           "DHE-RSA-AES256-GCM-SHA384:"
                           "ECDHE-RSA-AES256-SHA384:"
                           "HIGH:!aNULL:!eNULL:!EXPORT:"
                           "!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
                           "!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
                           "!DHE-RSA-AES128-SHA256:"
                           "!AES128-GCM-SHA256:"
                           "!AES128-SHA256:"
                           "!DHE-RSA-AES256-SHA256:"
                           "!AES256-GCM-SHA384:"
                           "!AES256-SHA256";
    info.mounts = &mount;
    info.ip_limit_ah = 24;   /* for testing */
    info.ip_limit_wsi = 400; /* for testing */

    if (use_ssl)
        /* redirect guys coming on http */
        info.options |= LWS_SERVER_OPTION_REDIRECT_HTTP_TO_HTTPS;

    context = lws_create_context(&info);
    if (context == NULL) {
        lwsl_err("libwebsocket init failed\n");
        return -1;
    }

    info.pvo = &pvo;

    vhost = lws_create_vhost(context, &info);
    if (!vhost) {
        lwsl_err("vhost creation failed\n");
        return -1;
    }

    /*
	 * For testing dynamic vhost create / destroy later, we use port + 1
	 * Normally if you were creating more vhosts, you would set info.name
	 * for each to be the hostname external clients use to reach it
	 */

    info.port++;

#if !defined(LWS_NO_CLIENT) && defined(LWS_WITH_TLS)
    lws_init_vhost_client_ssl(&info, vhost);
#endif

    /* this shows how to override the lws file operations.	You don't need
	 * to do any of this unless you have a reason (eg, want to serve
	 * compressed files without decompressing the whole archive)
	 */
    /* stash original platform fops */
    fops_plat = *(lws_get_fops(context));
    /* override the active fops */
    lws_get_fops(context)->open = test_server_fops_open;

    n = 0;
    while (n >= 0 && !force_exit) {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        /*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */

        n = lws_service(context, 50);

        if (dynamic_vhost_enable && !dynamic_vhost) {
            lwsl_notice("creating dynamic vhost...\n");
            dynamic_vhost = lws_create_vhost(context, &info);
        } else if (!dynamic_vhost_enable && dynamic_vhost) {
            lwsl_notice("destroying dynamic vhost...\n");
            lws_vhost_destroy(dynamic_vhost);
            dynamic_vhost = NULL;
        }
    }
    lws_context_destroy(context);

    lwsl_notice("libwebsockets-test-server exited cleanly\n");

    return 0;
}
