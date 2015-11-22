#include "ue.hpp"
#include "pjsip_core.hpp"

#include <map>
#include <sys/time.h>

static std::map<pjsip_transport*, UE*> transport_mapping;

pj_bool_t on_rx_request( pjsip_rx_data *rdata )
{
  UE* ue = transport_mapping[rdata->tp_info.transport];
  ue->on_rx_request(rdata);
  return PJ_TRUE;
}

void on_tsx_state(pjsip_transaction *tsx, pjsip_event *event)
{
  UE* ue = (UE*)tsx->mod_data[ua_module()->id];
  ue->on_tsx_state(tsx, event);
}


void regc_cb(struct pjsip_regc_cbparam *param)
{
  UE* ue = (UE*)param->token;
  ue->on_completed_registration(param);
}

void reg_tsx_cb(struct pjsip_regc_tsx_cb_param *param)
{
}

UE::UE(std::string realm,
    std::string myurl,
    std::string username,
    std::string password,
    std::string outbound_proxy)
: _reg_mutex(PTHREAD_MUTEX_INITIALIZER),
  _reg_cond(PTHREAD_COND_INITIALIZER),
  _msg_mutex(PTHREAD_MUTEX_INITIALIZER),
  _msg_cond(PTHREAD_COND_INITIALIZER)
{
  init_pjsip();
  std::string server_uri = std::string("sip:") + realm + std::string(";lr;transport=tcp");
  pj_status_t status;
  pj_sockaddr addr;
  _pool = pj_pool_create(get_global_pool_factory(), "a", 256, 256, NULL);

  pj_sockaddr_init(pj_AF_INET(), &addr, NULL, (pj_uint16_t)0);

  //status = pjsip_udp_transport_start(get_global_endpoint(), &addr.ipv4, NULL, 1, &_transport);
  //assert(status == PJ_SUCCESS);

  if (outbound_proxy.empty())
  {
    outbound_proxy = realm;
  }
  
  pj_str_t remote;
  pj_cstr(&remote, outbound_proxy.c_str());
  pj_sockaddr remote_addr;
  pj_sockaddr_init(pj_AF_INET(), &remote_addr, &remote, (pj_uint16_t)5060);

  pjsip_tpselector sel2;
  sel2.type = PJSIP_TPSELECTOR_LISTENER;
  sel2.u.listener = get_global_tcp_factory();
  status = pjsip_endpt_acquire_transport(get_global_endpoint(),
      PJSIP_TRANSPORT_TCP,
      &remote_addr,
      pj_sockaddr_get_len(&remote_addr),
      &sel2,
      &_transport);
  assert(status == PJ_SUCCESS);
  
  transport_mapping[_transport] = this;

  status = pjsip_regc_create(get_global_endpoint(), this, &regc_cb, &_regc);
  assert(status == PJ_SUCCESS);

  pjsip_regc_set_reg_tsx_cb(_regc, &reg_tsx_cb);

  pjsip_tpselector sel;
  sel.type = PJSIP_TPSELECTOR_TRANSPORT;
  sel.u.transport = _transport;
  pjsip_regc_set_transport(_regc, &sel);

  pjsip_cred_info cred;
  stra(&cred.realm, realm.c_str());
  stra(&cred.scheme, "Digest");
  stra(&cred.username, username.c_str());
  stra(&cred.data, password.c_str());
  cred.data_type = 0; // Plaintext password
  pjsip_cred_info creds[1] = {cred};
  pjsip_regc_set_credentials(_regc, 1, creds);

  char contact[32];
  snprintf(contact, 32, "sip:phone@%.*s:%d", (int)_transport->local_name.host.slen, _transport->local_name.host.ptr, _transport->local_name.port);

  stra(&_realm, realm.c_str());
  stra(&_server, server_uri.c_str());
  _server_uri = pjsip_parse_uri(_pool, _server.ptr, _server.slen, 0);
  stra(&_my_uri, myurl.c_str());
  stra(&_username, username.c_str());
  stra(&_contact, contact);
}

void UE::on_rx_request(pjsip_rx_data *rdata)
{
 pjsip_msg* msg = rdata->msg_info.msg;

  pj_str_t message;
  stra(&message, "MESSAGE");
  pjsip_method msg_method;
  pjsip_method_init(&msg_method, _pool, &message);
  if (pjsip_method_cmp(&msg_method, &msg->line.req.method) == 0)
  {
    std::string body((char*)msg->body->data, msg->body->len);
    pjsip_transaction* tsx;
    pjsip_endpt_respond(get_global_endpoint(), NULL, rdata, 200, NULL, NULL, NULL, &tsx);
  } else {
    pjsip_endpt_respond_stateless(get_global_endpoint(), rdata, 
        500, NULL,
        NULL, NULL);
  }
}


void UE::on_tsx_state(pjsip_transaction *tsx, pjsip_event *event)
{
  if ((tsx == _msg_tsx) && (tsx->state >= PJSIP_TSX_STATE_COMPLETED))
  {   
    pthread_mutex_lock(&_msg_mutex);
    pthread_cond_signal(&_msg_cond);
    pthread_mutex_unlock(&_msg_mutex);
  }
}

void UE::stra(pj_str_t* out, const char* in)
{
  pj_strdup2(_pool, out, in);
}

int UE::do_register(int expiry)
{
  pthread_mutex_lock(&_reg_mutex);
  pj_str_t contact[1] = {_contact};
  pj_status_t status = pjsip_regc_init(_regc, &_server,
      &_my_uri,
      &_my_uri,
      1,
      contact,
      expiry);
  assert(status == PJ_SUCCESS);

  pjsip_tx_data* tdata;
  pjsip_regc_register(_regc, 1, &tdata);

  pjsip_authorization_hdr* auth = pjsip_authorization_hdr_create(tdata->pool);
  stra(&auth->scheme, "Digest");
  auth->credential.digest.realm = _realm;
  auth->credential.digest.username = _username;
  pjsip_msg_insert_first_hdr(tdata->msg, (pjsip_hdr*)auth);

  pjsip_regc_send(_regc, tdata);

  pthread_cond_wait(&_reg_cond, &_reg_mutex);
  int ret = _last_register_response->line.status.code;
  pthread_mutex_unlock(&_reg_mutex);
  return ret;
}

int UE::send_message(std::string dest, std::string contents)
{
  timeval before;
  timeval after;
  pthread_mutex_lock(&_msg_mutex);
  pj_str_t to;
  stra(&to, dest.c_str());
  pj_str_t data;
  stra(&data, contents.c_str());
  pj_str_t message;
  stra(&message, "MESSAGE");
  pjsip_tx_data* tdata;
  pjsip_method msg_method;
  pjsip_method_init(&msg_method, _pool, &message);

  pjsip_endpt_create_request(get_global_endpoint(),
      &msg_method,
      &to,
      &_my_uri,
      &to,
      &_contact,
      NULL,
      -1,
      &data,
      &tdata);


  pjsip_tsx_create_uac(ua_module(), tdata, &_msg_tsx);

  pjsip_tpselector sel;
  sel.type = PJSIP_TPSELECTOR_TRANSPORT;
  sel.u.transport = _transport;
  pjsip_tsx_set_transport(_msg_tsx, &sel);
  
  pjsip_route_hdr* rt_hdr = pjsip_route_hdr_create(tdata->pool);
  rt_hdr->name_addr.uri = _server_uri;
  pjsip_msg_insert_first_hdr(tdata->msg, (pjsip_hdr*)rt_hdr);

  _msg_tsx->mod_data[ua_module()->id] = this;

  pj_grp_lock_add_ref(_msg_tsx->grp_lock);
  gettimeofday(&before, NULL);
  pj_status_t status = pjsip_tsx_send_msg(_msg_tsx, NULL);
  if (status != PJ_SUCCESS)
  {
    pthread_mutex_unlock(&_msg_mutex);
    return -1;
  }
  while (_msg_tsx->state < PJSIP_TSX_STATE_COMPLETED)
  {
    pthread_cond_wait(&_msg_cond, &_msg_mutex);
  }
  gettimeofday(&after, NULL);
  //unsigned long latency = ((after.tv_sec - before.tv_sec) * 1000000) + (after.tv_usec - before.tv_usec);
  //printf("Message latency is %lu\n", latency);
  int ret = _msg_tsx->status_code;
  pj_grp_lock_dec_ref(_msg_tsx->grp_lock);
  _msg_tsx = NULL;
  pthread_mutex_unlock(&_msg_mutex);
  return ret;
}

void UE::on_completed_registration(struct pjsip_regc_cbparam *param)
{
  pthread_mutex_lock(&_reg_mutex);

  _last_register_response = pjsip_msg_clone(_pool, param->rdata->msg_info.msg);

  pthread_cond_signal(&_reg_cond);

  pthread_mutex_unlock(&_reg_mutex);
}

