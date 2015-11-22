#include "pjsip_c_if.h"

#include <pjsip.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjlib-util.h>
#include <pjlib.h>

pjsip_endpoint* get_global_endpoint();
pj_pool_factory* get_global_pool_factory();
pjsip_tpfactory* get_global_tcp_factory();
pjsip_module* ua_module();
