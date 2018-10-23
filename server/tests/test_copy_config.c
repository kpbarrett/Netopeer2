/**
 * @file test_copy_config.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief Cmocka np2srv <copy-config> test.
 *
 * Copyright (c) 2016-2017 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */
#define _GNU_SOURCE

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"
#include "tests/config.h"

#define main server_main
#undef NP2SRV_PIDFILE
#define NP2SRV_PIDFILE "/tmp/test_np2srv.pid"

#ifdef NP2SRV_ENABLED_URL_CAPABILITY
#define URL_TESTFILE "/tmp/nc2_copy_config.xml"
#endif

#include "../main.c"

#undef main

volatile int initialized;
int pipes[2][2], p_in, p_out;

/*
 * SYSREPO WRAPPER FUNCTIONS
 */
int
__wrap_sr_connect(const char *app_name, const sr_conn_options_t opts, sr_conn_ctx_t **conn_ctx)
{
    (void)app_name;
    (void)opts;
    (void)conn_ctx;
    return SR_ERR_OK;
}

int
__wrap_sr_session_start(sr_conn_ctx_t *conn_ctx, const sr_datastore_t datastore,
                        const sr_sess_options_t opts, sr_session_ctx_t **session)
{
    (void)conn_ctx;
    (void)datastore;
    (void)opts;
    (void)session;
    return SR_ERR_OK;
}

int
__wrap_sr_session_start_user(sr_conn_ctx_t *conn_ctx, const char *user_name, const sr_datastore_t datastore,
                             const sr_sess_options_t opts, sr_session_ctx_t **session)
{
    (void)conn_ctx;
    (void)user_name;
    (void)datastore;
    (void)opts;
    (void)session;
    return SR_ERR_OK;
}

int
__wrap_sr_session_stop(sr_session_ctx_t *session)
{
    (void)session;
    return SR_ERR_OK;
}

void
__wrap_sr_disconnect(sr_conn_ctx_t *conn_ctx)
{
    (void)conn_ctx;
}

int
__wrap_sr_session_refresh(sr_session_ctx_t *session)
{
    (void)session;
    return SR_ERR_OK;
}

int
__wrap_sr_module_install_subscribe(sr_session_ctx_t *session, sr_module_install_cb callback, void *private_ctx,
                                   sr_subscr_options_t opts, sr_subscription_ctx_t **subscription)
{
    (void)session;
    (void)callback;
    (void)private_ctx;
    (void)opts;
    (void)subscription;
    return SR_ERR_OK;
}

int
__wrap_sr_feature_enable_subscribe(sr_session_ctx_t *session, sr_feature_enable_cb callback, void *private_ctx,
                                   sr_subscr_options_t opts, sr_subscription_ctx_t **subscription)
{
    (void)session;
    (void)callback;
    (void)private_ctx;
    (void)opts;
    (void)subscription;
    return SR_ERR_OK;
}

int
__wrap_sr_module_change_subscribe(sr_session_ctx_t *session, const char *module_name, sr_module_change_cb callback,
                                  void *private_ctx, uint32_t priority, sr_subscr_options_t opts,
                                  sr_subscription_ctx_t **subscription)
{
    (void)session;
    (void)module_name;
    (void)callback;
    (void)private_ctx;
    (void)priority;
    (void)opts;
    (void)subscription;
    return SR_ERR_OK;
}

int
__wrap_sr_session_switch_ds(sr_session_ctx_t *session, sr_datastore_t ds)
{
    (void)session;
    (void)ds;

    return SR_ERR_OK;
}

void
__wrap_sr_free_val_iter(sr_val_iter_t *iter)
{
    if (iter) {
        free(iter);
    }
}

static int set_item_count = 0;

int
__wrap_sr_set_item(sr_session_ctx_t *session, const char *xpath, const sr_val_t *value, const sr_edit_options_t opts)
{
    (void)session;
    (void)value;
    (void)opts;

    switch (set_item_count) {
    case 0:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']");
        break;
    case 1:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/description");
        break;
    case 2:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/type");
        break;
    case 3:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/enabled");
        break;
    case 4:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/link-up-down-trap-enable");
        break;
    case 5:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv4");
        break;
    case 6:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv4/enabled");
        break;
    case 7:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv4/forwarding");
        break;
    case 8:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv4/mtu");
        break;
    case 9:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv4/neighbor[ip='10.0.0.2']");
        break;
    case 10:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv4/neighbor[ip='10.0.0.2']/link-layer-address");
        break;
    case 11:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv6");
        break;
    case 12:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv6/enabled");
        break;
    case 13:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='iface1/1']/ietf-ip:ipv6/forwarding");
        break;
    case 14:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]");
        break;
    case 15:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/description");
        break;
    case 16:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/type");
        break;
    case 17:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/enabled");
        break;
    case 18:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/link-up-down-trap-enable");
        break;
    case 19:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv4");
        break;
    case 20:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv4/enabled");
        break;
    case 21:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv4/forwarding");
        break;
    case 22:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv4/mtu");
        break;
    case 23:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv4/neighbor[ip='10.0.0.2']");
        break;
    case 24:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv4/neighbor[ip='10.0.0.2']/link-layer-address");
        break;
    case 25:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv6");
        break;
    case 26:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv6/enabled");
        break;
    case 27:
        assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name=\"'iface1/2'\"]/ietf-ip:ipv6/forwarding");
        break;
    default:
        assert_string_equal(xpath, "too many nodes");
        break;
    }
    ++set_item_count;

    return SR_ERR_OK;
}

int
__wrap_sr_commit(sr_session_ctx_t *session)
{
    (void)session;
    return SR_ERR_OK;
}

int
__wrap_sr_delete_item(sr_session_ctx_t *session, const char *xpath, const sr_edit_options_t opts)
{
    (void)session;
    (void)xpath;
    (void)opts;

    return SR_ERR_OK;
}

int
__wrap_sr_event_notif_send(sr_session_ctx_t *session, const char *xpath, const sr_val_t *values,
                           const size_t values_cnt, sr_ev_notif_flag_t opts)
{
    (void)session;
    (void)xpath;
    (void)values;
    (void)values_cnt;
    (void)opts;
    return SR_ERR_OK;
}

int
__wrap_sr_check_exec_permission(sr_session_ctx_t *session, const char *xpath, bool *permitted)
{
    (void)session;
    (void)xpath;
    *permitted = true;
    return SR_ERR_OK;
}

/*
 * LIBNETCONF2 WRAPPER FUNCTIONS
 */
NC_MSG_TYPE
__wrap_nc_accept(int timeout, struct nc_session **session)
{
    NC_MSG_TYPE ret;

    if (!initialized) {
        pipe(pipes[0]);
        pipe(pipes[1]);

        fcntl(pipes[0][0], F_SETFL, O_NONBLOCK);
        fcntl(pipes[0][1], F_SETFL, O_NONBLOCK);
        fcntl(pipes[1][0], F_SETFL, O_NONBLOCK);
        fcntl(pipes[1][1], F_SETFL, O_NONBLOCK);

        p_in = pipes[0][0];
        p_out = pipes[1][1];

        *session = calloc(1, sizeof **session);
        (*session)->status = NC_STATUS_RUNNING;
        (*session)->side = 1;
        (*session)->id = 1;
        (*session)->io_lock = malloc(sizeof *(*session)->io_lock);
        pthread_mutex_init((*session)->io_lock, NULL);
        (*session)->opts.server.rpc_lock = malloc(sizeof *(*session)->opts.server.rpc_lock);
        pthread_mutex_init((*session)->opts.server.rpc_lock, NULL);
        (*session)->opts.server.rpc_cond = malloc(sizeof *(*session)->opts.server.rpc_cond);
        pthread_cond_init((*session)->opts.server.rpc_cond, NULL);
        (*session)->opts.server.rpc_inuse = malloc(sizeof *(*session)->opts.server.rpc_inuse);
        *(*session)->opts.server.rpc_inuse = 0;
        (*session)->ti_type = NC_TI_FD;
        (*session)->ti.fd.in = pipes[1][0];
        (*session)->ti.fd.out = pipes[0][1];
        (*session)->ctx = np2srv.ly_ctx;
        (*session)->flags = 1; //shared ctx
        (*session)->username = "user1";
        (*session)->host = "localhost";
        (*session)->opts.server.session_start = (*session)->opts.server.last_rpc = time(NULL);
        printf("test: New session 1\n");
        initialized = 1;
        ret = NC_MSG_HELLO;
    } else {
        usleep(timeout * 1000);
        ret = NC_MSG_WOULDBLOCK;
    }

    return ret;
}

void
__wrap_nc_session_free(struct nc_session *session, void (*data_free)(void *))
{
    if (data_free) {
        data_free(session->data);
    }
    pthread_mutex_destroy(session->io_lock);
    free(session->io_lock);
    pthread_mutex_destroy(session->opts.server.rpc_lock);
    free(session->opts.server.rpc_lock);
    pthread_cond_destroy(session->opts.server.rpc_cond);
    free(session->opts.server.rpc_cond);
    free((int *)session->opts.server.rpc_inuse);
    free(session);
}

int
__wrap_nc_server_endpt_count(void)
{
    return 1;
}

/*
 * SERVER THREAD
 */
pthread_t server_tid;
static void *
server_thread(void *arg)
{
    (void)arg;
    char *argv[] = {"netopeer2-server", "-d", "-v2"};

    return (void *)(int64_t)server_main(3, argv);
}

/*
 * TEST
 */
static int
np_start(void **state)
{
    (void)state; /* unused */

    optind = 1;
    control = LOOP_CONTINUE;
    initialized = 0;
    assert_int_equal(pthread_create(&server_tid, NULL, server_thread, NULL), 0);

    while (!initialized) {
        usleep(100000);
    }

    return 0;
}

static int
np_stop(void **state)
{
    (void)state; /* unused */
    int64_t ret;

    control = LOOP_STOP;
    assert_int_equal(pthread_join(server_tid, (void **)&ret), 0);

    close(pipes[0][0]);
    close(pipes[0][1]);
    close(pipes[1][0]);
    close(pipes[1][1]);

#ifdef NP2SRV_ENABLED_URL_CAPABILITY
    unlink(URL_TESTFILE);
#endif
    return ret;
}

static void
test_copy_config(void **state)
{
    (void)state; /* unused */
    const char *copy_rpc =
    "<rpc msgid=\"1\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
        "<copy-config>"
            "<target>"
                "<running/>"
            "</target>"
            "<source>"
                "<config>"
"<interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">"
  "<interface>"
    "<name>iface1/1</name>"
    "<description>iface1/1 dsc</description>"
    "<type xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">ianaift:ethernetCsmacd</type>"
    "<enabled>true</enabled>"
    "<link-up-down-trap-enable>disabled</link-up-down-trap-enable>"
    "<ipv4 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>true</forwarding>"
      "<mtu>68</mtu>"
      "<neighbor>"
        "<ip>10.0.0.2</ip>"
        "<link-layer-address>01:34:56:78:9a:bc:de:f0</link-layer-address>"
      "</neighbor>"
    "</ipv4>"
    "<ipv6 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>false</forwarding>"
    "</ipv6>"
  "</interface>"
  "<interface>"
    "<name>'iface1/2'</name>"
    "<description>iface1/2 dsc</description>"
    "<type xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">ianaift:ethernetCsmacd</type>"
    "<enabled>true</enabled>"
    "<link-up-down-trap-enable>disabled</link-up-down-trap-enable>"
    "<ipv4 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>true</forwarding>"
      "<mtu>68</mtu>"
      "<neighbor>"
        "<ip>10.0.0.2</ip>"
        "<link-layer-address>01:34:56:78:9a:bc:de:f0</link-layer-address>"
      "</neighbor>"
    "</ipv4>"
    "<ipv6 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>false</forwarding>"
    "</ipv6>"
  "</interface>"
"</interfaces>"
                "</config>"
            "</source>"
        "</copy-config>"
    "</rpc>";
    const char *copy_rpl =
    "<rpc-reply msgid=\"1\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
        "<ok/>"
    "</rpc-reply>";

    set_item_count = 0;
    test_write(p_out, copy_rpc, __LINE__);
    test_read(p_in, copy_rpl, __LINE__);
}

#ifdef NP2SRV_ENABLED_URL_CAPABILITY
static void
test_copy_config_url(void **state)
{
    (void)state; /* unused */
    const char *copy_data =
"<interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">"
  "<interface>"
    "<name>iface1/1</name>"
    "<description>iface1/1 dsc</description>"
    "<type xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">ianaift:ethernetCsmacd</type>"
    "<enabled>true</enabled>"
    "<link-up-down-trap-enable>disabled</link-up-down-trap-enable>"
    "<ipv4 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>true</forwarding>"
      "<mtu>68</mtu>"
      "<neighbor>"
        "<ip>10.0.0.2</ip>"
        "<link-layer-address>01:34:56:78:9a:bc:de:f0</link-layer-address>"
      "</neighbor>"
    "</ipv4>"
    "<ipv6 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>false</forwarding>"
    "</ipv6>"
  "</interface>"
  "<interface>"
    "<name>'iface1/2'</name>"
    "<description>iface1/2 dsc</description>"
    "<type xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">ianaift:ethernetCsmacd</type>"
    "<enabled>true</enabled>"
    "<link-up-down-trap-enable>disabled</link-up-down-trap-enable>"
    "<ipv4 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>true</forwarding>"
      "<mtu>68</mtu>"
      "<neighbor>"
        "<ip>10.0.0.2</ip>"
        "<link-layer-address>01:34:56:78:9a:bc:de:f0</link-layer-address>"
      "</neighbor>"
    "</ipv4>"
    "<ipv6 xmlns=\"urn:ietf:params:xml:ns:yang:ietf-ip\">"
      "<enabled>true</enabled>"
      "<forwarding>false</forwarding>"
    "</ipv6>"
  "</interface>"
"</interfaces>"
            ;
    const char *copy_rpc =
    "<rpc msgid=\"1\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
        "<copy-config>"
            "<target>"
                "<running/>"
            "</target>"
            "<source>"
                "<url>file://" URL_TESTFILE "</url>"
            "</source>"
        "</copy-config>"
    "</rpc>";
    const char *copy_rpl =
    "<rpc-reply msgid=\"1\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
        "<ok/>"
    "</rpc-reply>";

    FILE* xmlfile = fopen(URL_TESTFILE, "w");
    fprintf(xmlfile, "%s", copy_data);
    fclose(xmlfile);

    set_item_count = 0;
    test_write(p_out, copy_rpc, __LINE__);
    test_read(p_in, copy_rpl, __LINE__);
}
#endif

static void
test_startstop(void **state)
{
    (void)state; /* unused */
    return;
}


int
main(void)
{
    const struct CMUnitTest tests[] = {
                    cmocka_unit_test_setup(test_startstop, np_start),
                    cmocka_unit_test(test_copy_config),
#ifdef NP2SRV_ENABLED_URL_CAPABILITY
                    cmocka_unit_test(test_copy_config_url),
#endif
                    cmocka_unit_test_teardown(test_startstop, np_stop),
    };

    if (setenv("CMOCKA_TEST_ABORT", "1", 1)) {
        fprintf(stderr, "Cannot set Cmocka thread environment variable.\n");
    }
    return cmocka_run_group_tests(tests, NULL, NULL);
}
