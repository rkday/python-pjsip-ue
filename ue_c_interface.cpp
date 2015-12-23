#include "ue_c_interface.h"
#include "ue.hpp"

void* ue_new(pj_log_func* logger,
       const char* server,
       const char* myurl,
       const char* username,
       const char* password,
       const char* proxy)
{
  UE* ue = UE::init(logger, server, myurl, username, password, proxy);
  return ue;
}

void ue_delete(void* ue)
{
  delete (UE*)ue;
}

int ue_register(void* ue, int expiry)
{
  return ((UE*)ue)->do_register(expiry);
}

int ue_send_message(void* ue, const char* dest, const char* content)
{
  return ((UE*)ue)->send_message(dest, content);
}

