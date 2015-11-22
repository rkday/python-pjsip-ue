from _example import ffi, lib

class UE(object):
    def __init__(self, realm, uri, username, password, outbound_proxy=""):
        self.ueptr = lib.ue_new(realm, uri, username, password, outbound_proxy)

    def register(self, expiry=300):
        rc = lib.ue_register(self.ueptr, expiry)
        print "Got {} response to REGISTER".format(rc)

    def text(self, target, message):
        rc = lib.ue_send_message(self.ueptr, target, message)
        print "Got {} response to MESSAGE".format(rc)
