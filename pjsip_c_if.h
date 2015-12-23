#ifdef __cplusplus
extern "C" {
#endif
typedef void pj_log_func(int level, const char *data, int len);

int init_pjsip(pj_log_func* logger);
void pjsip_teardown();
#ifdef __cplusplus
}
#endif
