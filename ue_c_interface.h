#ifdef __cplusplus
extern "C" {
#endif
typedef void pj_log_func(int level, const char *data, int len);

  void* ue_new(pj_log_func* logger,
      const char* server,
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
