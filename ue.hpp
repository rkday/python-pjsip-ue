#include <string>

#include <pjsip.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjlib-util.h>
#include <pjlib.h>

pj_bool_t on_rx_request( pjsip_rx_data *rdata );

class UE {
  public:
    UE(std::string realm,
       std::string myurl,
       std::string username,
       std::string password,
       std::string outbound_proxy="");

    ~UE() {};

    int do_register(int expiry=300);
    
    int send_message(std::string dest, std::string content);

    void on_completed_registration(struct pjsip_regc_cbparam *param);
    void on_rx_request(pjsip_rx_data *rdata);
    void on_tsx_state(pjsip_transaction *tsx, pjsip_event *event);
  private:
    void stra(pj_str_t*, const char*);

    pj_pool_t* _pool;
    pj_str_t _realm;
    pj_str_t _server;
    pjsip_uri* _server_uri;
    pj_str_t _my_uri;
    pj_str_t _username;
    pj_str_t _contact;
    pjsip_regc* _regc;
    pjsip_transport* _transport;

    pthread_mutex_t _reg_mutex;
    pthread_cond_t _reg_cond;
    pjsip_msg* _last_register_response;

    pthread_mutex_t _msg_mutex;
    pthread_cond_t _msg_cond;
    pjsip_transaction* _msg_tsx;
    //std::vector<std::string> inbox;
};
