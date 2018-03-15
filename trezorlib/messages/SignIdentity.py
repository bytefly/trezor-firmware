# Automatically generated by pb2py
from .. import protobuf as p
from .IdentityType import IdentityType


class SignIdentity(p.MessageType):
    FIELDS = {
        1: ('identity', IdentityType, 0),
        2: ('challenge_hidden', p.BytesType, 0),
        3: ('challenge_visual', p.UnicodeType, 0),
        4: ('ecdsa_curve_name', p.UnicodeType, 0),
    }
    MESSAGE_WIRE_TYPE = 53
