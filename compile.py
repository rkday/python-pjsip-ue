from cffi import FFI
ffi = FFI()

ffi.set_source("sipue._example",
    """ // passed to the real C compiler
        #include "ue_c_interface.h"
        #include "pjsip_c_if.h"
    """,
    libraries=["ue", "stdc++", 'ssl', 'crypto', 'm', 'rt', 'pthread'],
    library_dirs=["."],
    include_dirs=["."])   # or a list of libraries to link with
    # (more arguments like setup.py's Extension class:
    # include_dirs=[..], extra_objects=[..], and so on)

ffi.cdef("""     // some declarations from the man page
  void* ue_new(const char* server,
       const char* myurl,
       const char* username,
       const char* password,
       const char* proxy);

  void ue_delete(void* ue);

  int ue_register(void* ue, int expiry);
  int ue_send_message(void* ue, const char* dest, const char* content);

int init_pjsip();
void pjsip_teardown();
""")

if __name__ == "__main__":
    ffi.compile()
