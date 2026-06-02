#include <microhttpd.h>
#include <wrouter.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct app {
    wrouter_t *router;
};

typedef struct {
    struct app *app;
    unsigned int code;
    struct MHD_Response *response;
} app_dispatch_ctx_t;

static void rcb_root(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    (void)params;
    (void)route_ctx;

    const char *page = "<html><body>Go <a href=\"/hello/world\">"
                       "somewhere interesting</a>.</body></html>";

    app_dispatch_ctx_t *dx = dispatch_ctx;
    dx->response =
        MHD_create_response_from_buffer(strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(dx->response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
}

static void rcb_hello(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    const char *addressee = params->base[0].value;
    const char *port = route_ctx;

    char page[128];
    char *welcome = "<html><body>Hello, %s; from %s.</body></html>";
    int n = snprintf(page, sizeof(page), welcome, addressee, port);

    if (n < 0)
        return;

    char *copy = malloc((size_t)n + 1);
    if (!copy)
        return;

    memcpy(copy, page, (size_t)n + 1);

    app_dispatch_ctx_t *dx = dispatch_ctx;

    dx->response = MHD_create_response_from_buffer((size_t)n, copy, MHD_RESPMEM_MUST_FREE);

    MHD_add_response_header(dx->response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
}

static void rcb_not_found(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    (void)params;
    (void)route_ctx;

    const char *page = "<html><b>Not found.</b></html>";

    app_dispatch_ctx_t *dx = dispatch_ctx;
    dx->response =
        MHD_create_response_from_buffer(strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);
    dx->code = MHD_HTTP_NOT_FOUND;
    MHD_add_response_header(dx->response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
}

static _Thread_local wrouter_dispatcher_t *tls_disp;

static wrouter_dispatcher_t *get_thread_dispatcher(wrouter_t *router)
{
    if (!tls_disp)
        tls_disp = wrouter_dispatcher_create(router);

    return tls_disp;
}

static enum MHD_Result ahc_echo(void *cls, struct MHD_Connection *connection, const char *url,
                                const char *method, const char *version, const char *upload_data,
                                size_t *upload_data_size, void **ptr)
{
    (void)upload_data;
    (void)version;

    static int dummy;

    struct app *app = cls;

    wrouter_dispatcher_t *dispatcher = get_thread_dispatcher(app->router);

    enum MHD_Result ret;

    if (0 != strcmp(method, "GET"))
        return MHD_NO; /* unexpected method */
    if (&dummy != *ptr) {
        /* The first time only the headers are valid,
           do not respond in the first round... */
        *ptr = &dummy;
        return MHD_YES;
    }
    if (0 != *upload_data_size)
        return MHD_NO; /* upload data in a GET!? */
    *ptr = NULL;       /* clear context pointer */

    app_dispatch_ctx_t dx = {
        .app = app,
        .code = MHD_HTTP_OK,
    };

    wrouter_dispatch(dispatcher, url, &dx);

    ret = MHD_queue_response(connection, dx.code, dx.response);
    MHD_destroy_response(dx.response);
    return ret;
}

int main(int argc, char **argv)
{
    struct MHD_Daemon *d;

    if (argc != 2) {
        printf("%s PORT\n", argv[0]);
        return 1;
    }
    const char *port = argv[1];

    struct app app = { 0 };

    wrouter_options_t router_options = {
        .param_syntax = WROUTER_SYNTAX_COLON,
        .fallback_handler = rcb_not_found,
        .fallback_ctx = NULL,
    };

    wrouter_builder_t *builder = wrouter_builder_create(router_options);
    wrouter_add_handler(builder, "/", rcb_root);
    wrouter_add_handler_ctx(builder, "/hello/:addressee", rcb_hello, port);

    wrouter_error_t rcerr;
    app.router = wrouter_compile(builder, &rcerr);
    wrouter_builder_free(builder);

    if (rcerr)
        return 1;

    d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, atoi(port), NULL, NULL, &ahc_echo, &app,
                         MHD_OPTION_END);
    if (NULL == d)
        return 1;
    (void)getc(stdin);
    MHD_stop_daemon(d);

    wrouter_free(app.router);

    return 0;
}
