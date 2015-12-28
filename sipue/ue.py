from ._example import ffi, lib
import logging

__pdoc__ = {'ffi': None, 'lib': None}

_log = logging.getLogger(__name__) 

@ffi.callback("void(int, const char*, int)")
def logger_callback(level, data, length):

    log_levels = {1: logging.ERROR,
                  2: logging.WARNING,
                  3: logging.INFO,
                  4: logging.DEBUG}

    _log.log(log_levels.get(level, logging.DEBUG), ffi.string(data).decode('ascii'))

class UE(object):
    '''SIP user agent class, allowing emulation of a SIP softphone for testing.'''

    def __init__(self, realm: str, uri: str, username: str, password: str, outbound_proxy: str = ""):
        self._ueptr = lib.ue_new(logger_callback, realm.encode('utf-8'), uri.encode('utf-8'), username.encode('utf-8'), password.encode('utf-8'), outbound_proxy.encode('utf-8'))
        if self._ueptr == ffi.NULL:
            raise Exception("Could not create UE")

    def register(self, expiry: int = 300):
        rc = lib.ue_register(self._ueptr, expiry)
        _log.info("Got {} response to REGISTER".format(rc))
        return rc

    def text(self, target: str, message: str):
        rc = lib.ue_send_message(self._ueptr, target.encode('utf-8'), message.encode('utf-8'))
        _log.info("Got {} response to MESSAGE".format(rc))
        return rc
