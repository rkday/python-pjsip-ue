#ifdef __cplusplus
extern "C" {
#endif

  void* ue_new(const char* server,
       const char* myurl,
       const char* username,
       const char* password,
       const char* proxy);

  void ue_delete(void* ue);

  int ue_register(void* ue, int expiry);
  int ue_send_message(void* ue, const char* dest, const char* content);
#ifdef __cplusplus
}
#endif
