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

    def __init__(self, realm, uri, username, password, outbound_proxy=b""):
        self._ueptr = lib.ue_new(logger_callback, realm, uri, username, password, outbound_proxy)
        if self._ueptr == ffi.NULL:
            raise Exception("Could not create UE")

    def register(self, expiry=300):
        rc = lib.ue_register(self._ueptr, expiry)
        _log.info("Got {} response to REGISTER".format(rc))

    def text(self, target, message):
        rc = lib.ue_send_message(self._ueptr, target, message)
        _log.info("Got {} response to MESSAGE".format(rc))
