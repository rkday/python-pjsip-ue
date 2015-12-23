from _example import ffi, lib
import logging

_log = logging.getLogger(__name__) 

log_levels = {1: logging.ERROR,
        2: logging.WARNING,
        3: logging.INFO,
        4: logging.DEBUG}

@ffi.callback("void(int, const char*, int)")
def logger_callback(level, data, length):
    _log.log(log_levels.get(level, logging.DEBUG), ffi.string(data))

class UE(object):
    def __init__(self, realm, uri, username, password, outbound_proxy=""):
        self.ueptr = lib.ue_new(logger_callback, realm, uri, username, password, outbound_proxy)
        if self.ueptr == ffi.NULL:
            raise Exception("Could not create UE")

    def register(self, expiry=300):
        rc = lib.ue_register(self.ueptr, expiry)
        print "Got {} response to REGISTER".format(rc)

    def text(self, target, message):
        rc = lib.ue_send_message(self.ueptr, target, message)
        print "Got {} response to MESSAGE".format(rc)
