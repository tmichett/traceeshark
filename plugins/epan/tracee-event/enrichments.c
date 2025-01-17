#include <ctype.h>

#include <epan/packet.h>
#include "tracee.h"

static int hf_decoded_data = -1;
static int hf_file_type = -1;

static int enrich_sched_process_exec(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data)
{
    const gchar *pathname, *cmdline;
    struct tracee_dissector_data *dissector_data = (struct tracee_dissector_data *)data;

    pathname = wanted_field_get_str("tracee.args.sched_process_exec.pathname");
    cmdline = wanted_field_get_str("tracee.args.command_line");

    dissector_data->process->exec_path = pathname;
    dissector_data->process->command_line = cmdline;

    if (pathname && cmdline) {
        if (strncmp(pathname, cmdline, strlen(pathname)) == 0)
            col_add_str(pinfo->cinfo, COL_INFO, cmdline);
        else
            col_add_fstr(pinfo->cinfo, COL_INFO, "%s: %s", pathname, cmdline);
    }

    return 0;
}

static int enrich_net_packet_http_request(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    const gchar *method, *protocol, *uri_path, *content_type;

    method = wanted_field_get_str("tracee.proto_http_request.method");
    protocol = wanted_field_get_str("tracee.proto_http_request.protocol");
    uri_path = wanted_field_get_str("tracee.proto_http_request.uri_path");

    if (!method || !protocol || !uri_path)
        return 0;
    
    col_add_fstr(pinfo->cinfo, COL_INFO, "%s %s %s", method, uri_path, protocol);
    
    if (strcmp(method, "POST") && ((content_type = wanted_field_get_str("http.content_type")) != NULL))
        col_append_fstr(pinfo->cinfo, COL_INFO, "  (%s)", content_type);

    return 0;
}

static int enrich_net_packet_http(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    const gchar *direction, *method, *protocol, *uri_path, *content_type, *status;
    gchar *tmp, *content_type_short = NULL;
    gboolean request;

    direction = wanted_field_get_str("tracee.proto_http.direction");
    if (direction == NULL)
        return 0;
    if (strcmp(direction, "request") == 0)
        request = TRUE;
    else if (strcmp(direction, "response") == 0)
        request = FALSE;
    else
        return 0;
    
    protocol = wanted_field_get_str("tracee.proto_http.protocol");
    content_type = wanted_field_get_str("http.content_type");

    // discard semicolon in content type
    if (content_type != NULL) {
        content_type_short = wmem_strdup(pinfo->pool, content_type);
        tmp = strchr(content_type_short, ';');
        if (tmp != NULL)
            *tmp = '\0';
    }

    if (request) {
        method = wanted_field_get_str("tracee.proto_http.method");
        uri_path = wanted_field_get_str("tracee.proto_http.uri_path");

        if (!method || !protocol || !uri_path)
            return 0;
        
        col_add_fstr(pinfo->cinfo, COL_INFO, "%s %s %s", method, uri_path, protocol);
        
        if (content_type_short != NULL)
            col_append_fstr(pinfo->cinfo, COL_INFO, " (%s)", content_type_short);
    }

    else {
        status = wanted_field_get_str("tracee.proto_http.status");

        if (!protocol || !status)
            return 0;
        
        col_add_fstr(pinfo->cinfo, COL_INFO, "%s %s", protocol, status);

        if (content_type_short != NULL)
            col_append_fstr(pinfo->cinfo, COL_INFO, "  (%s)", content_type_short);
    }

    return 0;
}

gchar *enrichments_get_security_socket_bind_connect_description(packet_info *pinfo, const gchar *verb)
{
    const gchar *family, *addr, *port = NULL;

    if ((family = wanted_field_get_str("tracee.sockaddr.sa_family")) == NULL)
        return NULL;
    
    if (strcmp(family, "AF_INET") == 0) {
        addr = wanted_field_get_str("tracee.sockaddr.sin_addr");
        port = wanted_field_get_str("tracee.sockaddr.sin_port");
    }
    else if (strcmp(family, "AF_INET6") == 0) {
        addr = wanted_field_get_str("tracee.sockaddr.sin6_addr");
        port = wanted_field_get_str("tracee.sockaddr.sin6_port");
    }
    else if (strcmp(family, "AF_UNIX") == 0) {
        addr = wanted_field_get_str("tracee.sockaddr.sun_path");
    }
    else
        return NULL;
    
    if (addr) {
        if (port)
            return wmem_strdup_printf(pinfo->pool, "%s to %s port %s", verb, addr, port);
        else
            return wmem_strdup_printf(pinfo->pool, "%s to %s", verb, addr);
    }
    
    return NULL;
}

static int enrich_security_socket_bind_connect(packet_info *pinfo, const gchar *verb)
{
    gchar *description = enrichments_get_security_socket_bind_connect_description(pinfo,verb);

    if (description != NULL)
        col_add_str(pinfo->cinfo, COL_INFO, description);
    
    return 0;
}

static int enrich_security_socket_bind(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    return enrich_security_socket_bind_connect(pinfo, "Bind");
}

static int enrich_security_socket_connect(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    return enrich_security_socket_bind_connect(pinfo, "Connect");
}

static int enrich_dynamic_code_loading(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    const gchar *alert = wanted_field_get_str("tracee.args.dynamic_code_loading.triggered_by.alert");

    if (alert)
        col_append_str(pinfo->cinfo, COL_INFO, alert);
    
    return 0;
}

static int enrich_fileless_execution(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    const gchar *pathname = wanted_field_get_str("tracee.args.fileless_execution.triggered_by.pathname");

    if (pathname)
        col_append_fstr(pinfo->cinfo, COL_INFO, "Running from %s", pathname);
    
    return 0;
}

static int enrich_stdio_over_socket(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    gint *fd;
    const gchar *addr, *port, *stream;

    fd = wanted_field_get_int("tracee.args.stdio_over_socket.File_descriptor");
    addr = wanted_field_get_str("tracee.args.stdio_over_socket.IP_address");
    port = wanted_field_get_str("tracee.args.stdio_over_socket.Port");

    if (fd && addr && port) {
        switch (*fd) {
            case 0:
                stream = "STDIN";
                break;
            case 1:
                stream = "STDOUT";
                break;
            case 2:
                stream = "STDERR";
                break;
            default:
                return 0;
        }

        col_add_fstr(pinfo->cinfo, COL_INFO, "%s forwarded to %s port %s", stream, addr, port);
    }

    return 0;
}

static const char *stringify_decoded_data(packet_info *pinfo, guchar *decoded_data, gsize len)
{
    gsize i;
#if ((WIRESHARK_VERSION_MAJOR < 4) || ((WIRESHARK_VERSION_MAJOR == 4) && (WIRESHARK_VERSION_MINOR < 1)))
    wmem_strbuf_t *str = wmem_strbuf_sized_new(pinfo->pool, len, 0);
#else
    wmem_strbuf_t *str = wmem_strbuf_new_sized(pinfo->pool, len);
#endif

    for (i = 0; i < len; i++) {
        if (isprint(decoded_data[i]))
            wmem_strbuf_append_c(str, (char)decoded_data[i]);
        else {
            switch (decoded_data[i]) {
                case '\n':
                    wmem_strbuf_append(str, "\\n");
                    break;
                case '\r':
                    wmem_strbuf_append(str, "\\r");
                    break;
                case '\t':
                    wmem_strbuf_append(str, "\\t");
                    break;
                default:
                    wmem_strbuf_append_printf(str, "\\x%s%x", decoded_data[i] < 16 ? "0" : "", decoded_data[i]);
                    break;
            }
        }
    }

    return wmem_strbuf_get_str(str);
}

static const char *get_file_type(guchar *decoded_data, gsize len)
{
    const char *data = (const char *)decoded_data;

    if (len >= 4 && strncmp(data, "\x7F" "ELF", 4) == 0)
        return "ELF";
    else if (len >= 2 && strncmp(data, "#!", 2) == 0)
        return "Script";
    else if (len >= 4 && strncmp(data, "\xED\xAB\xEE\xDB", 4) == 0)
        return "RPM package";
    else if (len >= 8 && strncmp(data, "!<arch>\x0A", 8) == 0)
        return "DEB package";
    
    return NULL;
}

static int enrich_magic_write(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree _U_, void *data)
{
    struct tracee_dissector_data *dissector_data = (struct tracee_dissector_data *)data;
    const gchar *bytes, *pathname;
    guchar *decoded_data;
    gsize len;
    const char *decoded_string, *file_type;
    proto_item *tmp_item;

    if ((bytes = wanted_field_get_str("tracee.args.magic_write.bytes")) == NULL)
        return 0;
    
    // decode written data
    decoded_data = g_base64_decode(bytes, &len);

    // add decoded data
    decoded_string = stringify_decoded_data(pinfo, decoded_data, len);
    tmp_item = proto_tree_add_string_wanted(dissector_data->args_tree, hf_decoded_data, tvb, 0, 0, decoded_string);
    proto_item_set_generated(tmp_item);

    // add file type, if known
    if ((file_type = get_file_type(decoded_data, len)) != NULL) {
        tmp_item = proto_tree_add_string_wanted(dissector_data->args_tree, hf_file_type, tvb, 0, 0, file_type);
        proto_item_set_generated(tmp_item);
    }

    // set info column
    if ((pathname = wanted_field_get_str("tracee.args.magic_write.pathname")) != NULL) {
        col_add_fstr(pinfo->cinfo, COL_INFO, "%s magic written to %s", file_type != NULL ? file_type : "Unknown", pathname);
    }

    g_free(decoded_data);
    return 0;
}

static int enrich_security_file_open(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree _U_, void *data _U_)
{
    const gchar *pathname, *syscall_pathname;

    if ((syscall_pathname = wanted_field_get_str("tracee.args.security_file_open.syscall_pathname")) == NULL)
        return 0;

    col_add_fstr(pinfo->cinfo, COL_INFO, "Open %s", syscall_pathname);

    if ((pathname = wanted_field_get_str("tracee.args.security_file_open.pathname")) == NULL)
        return 0;
    
    if (strlen(pathname) > 0 && strcmp(pathname, syscall_pathname) != 0)
        col_append_fstr(pinfo->cinfo, COL_INFO, " (%s)", pathname);
    
    return 0;
}

static void register_wanted_fields(void)
{
    // needed for enrich_sched_process_exec
    register_wanted_field("tracee.args.sched_process_exec.pathname");
    register_wanted_field("tracee.args.command_line");

    // needed for enrich_net_packet_http_request
    register_wanted_field("tracee.proto_http_request.method");
    register_wanted_field("tracee.proto_http_request.protocol");
    register_wanted_field("tracee.proto_http_request.uri_path");
    register_wanted_field("http.content_type");

    // needed for enrich_net_packet_http
    register_wanted_field("tracee.proto_http.direction");
    register_wanted_field("tracee.proto_http.method");
    register_wanted_field("tracee.proto_http.protocol");
    register_wanted_field("tracee.proto_http.uri_path");
    register_wanted_field("tracee.proto_http.status");

    // needed for enrich_security_socket_bind_connect
    register_wanted_field("tracee.sockaddr.sa_family");
    register_wanted_field("tracee.sockaddr.sin_addr");
    register_wanted_field("tracee.sockaddr.sin_port");
    register_wanted_field("tracee.sockaddr.sin6_addr");
    register_wanted_field("tracee.sockaddr.sin6_port");
    register_wanted_field("tracee.sockaddr.sun_path");

    // needed for enrich_dynamic_code_loading
    register_wanted_field("tracee.args.dynamic_code_loading.triggered_by.alert");

    // needed for enrich_fileless_execution
    register_wanted_field("tracee.args.fileless_execution.triggered_by.pathname");

    // needed for enrich_stdio_over_socket
    register_wanted_field("tracee.args.stdio_over_socket.File_descriptor");
    register_wanted_field("tracee.args.stdio_over_socket.IP_address");
    register_wanted_field("tracee.args.stdio_over_socket.Port");

    // needed for enrich_magic_write
    register_wanted_field("tracee.args.magic_write.bytes");
    register_wanted_field("tracee.args.magic_write.pathname");

    // needed for enrich_security_file_open
    register_wanted_field("tracee.args.security_file_open.pathname");
    register_wanted_field("tracee.args.security_file_open.syscall_pathname");
}

void register_tracee_enrichments(int proto)
{
    static hf_register_info hf[] = {
        { &hf_decoded_data,
          { "Decoded data", "tracee.args.magic_write.decoded_data",
            FT_STRINGZ, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_file_type,
          { "File type", "tracee.args.magic_write.file_type",
            FT_STRINGZ, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
    };

    proto_register_field_array(proto, hf, array_length(hf));

    register_wanted_fields();
}

static void register_tracee_event_enrichment(const gchar *event_name, dissector_t dissector)
{
    int proto_tracee;
    dissector_handle_t dissector_handle;
    
    DISSECTOR_ASSERT((proto_tracee = proto_get_id_by_filter_name("tracee")) != -1);

    dissector_handle = create_dissector_handle(dissector, proto_tracee);
    dissector_add_string("tracee.eventName", event_name, dissector_handle);
}

void proto_reg_handoff_tracee_enrichments(void)
{
    register_tracee_event_enrichment("sched_process_exec", enrich_sched_process_exec);
    register_tracee_event_enrichment("net_packet_http_request", enrich_net_packet_http_request);
    register_tracee_event_enrichment("net_packet_http", enrich_net_packet_http);
    register_tracee_event_enrichment("security_socket_bind", enrich_security_socket_bind);
    register_tracee_event_enrichment("security_socket_connect", enrich_security_socket_connect);
    register_tracee_event_enrichment("dynamic_code_loading", enrich_dynamic_code_loading);
    register_tracee_event_enrichment("fileless_execution", enrich_fileless_execution);
    register_tracee_event_enrichment("stdio_over_socket", enrich_stdio_over_socket);
    register_tracee_event_enrichment("magic_write", enrich_magic_write);
    register_tracee_event_enrichment("security_file_open", enrich_security_file_open);
}