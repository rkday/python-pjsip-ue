#include "pjsip_core.hpp"
#include <pthread.h>

static pjsip_endpoint	*g_endpt = NULL;	    /* SIP endpoint.		*/
static pj_caching_pool cp;	    /* Global pool factory.	*/
static pjsip_tpfactory* tp;
static pthread_t evthread;
static bool g_stop = false;

#define THIS_FILE   "pjsip_core.cpp"

pjsip_endpoint* get_global_endpoint()
{
  return g_endpt;
}

pj_pool_factory* get_global_pool_factory()
{
  return &cp.factory;
}

pjsip_tpfactory* get_global_tcp_factory()
{
  return tp;
}


void* handle_events(void* x)
{
  pj_thread_desc desc = {0};
  pj_thread_t* t = NULL;
  pj_thread_register("event_thread", desc, &t);
  while (!g_stop)
  {
    pj_time_val timeout = {0, 10};
    pjsip_endpt_handle_events(g_endpt, &timeout);
  }
  return NULL;
}

// Module that dispatches events to the right UE object
extern pj_bool_t on_rx_request( pjsip_rx_data *rdata );
extern void on_tsx_state(pjsip_transaction *tsx, pjsip_event *event);
static pjsip_module mod_dispatcher = {0};

pjsip_module* ua_module()
{
  return &mod_dispatcher;
}

/* Notification on incoming messages */
static pj_bool_t logging_on_rx_msg(pjsip_rx_data *rdata)
{
  /* Always return false, otherwise messages will not get processed! */
  return PJ_FALSE;
}

/* Notification on outgoing messages */
static pj_status_t logging_on_tx_msg(pjsip_tx_data *tdata)
{
  /* Always return success, otherwise message will not get sent! */
  return PJ_SUCCESS;
}

static pjsip_module msg_logger = 
{
  NULL, NULL,				/* prev, next.		*/
  { const_cast<char*>("mod-msg-log"), 13 },		/* Name.		*/
  -1,					/* Id			*/
  PJSIP_MOD_PRIORITY_TRANSPORT_LAYER-1,/* Priority	        */
  NULL,				/* load()		*/
  NULL,				/* start()		*/
  NULL,				/* stop()		*/
  NULL,				/* unload()		*/
  &logging_on_rx_msg,			/* on_rx_request()	*/
  &logging_on_rx_msg,			/* on_rx_response()	*/
  &logging_on_tx_msg,			/* on_tx_request.	*/
  &logging_on_tx_msg,			/* on_tx_response()	*/
  NULL,				/* on_tsx_state()	*/

};

#define CHECK(X) { status = X ; if (status != PJ_SUCCESS) { PJ_LOG(1, (__FILE__, "Call to %s failed with code %d", #X, status)); return 1; }; }

/*
 * Callback when incoming requests outside any transactions and any
 * dialogs are received. We're only interested to hande incoming INVITE
 * request, and we'll reject any other requests with 500 response.
 */
int init_pjsip(pj_log_func* logger)
{
  if (g_endpt != NULL)
  {
    return 0;
  }

  mod_dispatcher.priority = PJSIP_MOD_PRIORITY_APPLICATION;
  mod_dispatcher.on_rx_request = &on_rx_request;
  mod_dispatcher.on_tsx_state = &on_tsx_state;

  pj_log_set_level(99);
  pj_log_set_decor(PJ_LOG_HAS_SENDER | PJ_LOG_HAS_THREAD_ID | PJ_LOG_HAS_INDENT);
  pj_log_set_log_func(logger);
  pj_status_t status;
  CHECK(pj_init());


  CHECK(pjlib_util_init());

  pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);
  CHECK(pjsip_endpt_create(&cp.factory, "single_endpoint", &g_endpt));

  pj_sockaddr addr;
  pj_sockaddr_init(pj_AF_INET(), &addr, NULL, (pj_uint16_t)0);
  CHECK(pjsip_tcp_transport_start(get_global_endpoint(), &addr.ipv4, 1, &tp));
  
  CHECK(pjsip_tsx_layer_init_module(g_endpt));
  CHECK(pjsip_ua_init_module( g_endpt, NULL ));

  CHECK(pjsip_endpt_register_module( g_endpt, &mod_dispatcher));

  CHECK(pjsip_endpt_register_module( g_endpt, &msg_logger));

  pthread_create(&evthread, NULL, &handle_events, NULL);
  return 0;
}

void pjsip_teardown()
{
  g_stop = true;
  pthread_join(evthread, NULL);
  pjsip_endpt_destroy(g_endpt);
  pj_caching_pool_destroy(&cp);
}


