# Automatically generated by pb2py
from .. import protobuf as p


class ResetDevice(p.MessageType):
    FIELDS = {
        1: ('display_random', p.BoolType, 0),
        2: ('strength', p.UVarintType, 0),  # default=256
        3: ('passphrase_protection', p.BoolType, 0),
        4: ('pin_protection', p.BoolType, 0),
        5: ('language', p.UnicodeType, 0),  # default='english'
        6: ('label', p.UnicodeType, 0),
        7: ('u2f_counter', p.UVarintType, 0),
        8: ('skip_backup', p.BoolType, 0),
    }
    MESSAGE_WIRE_TYPE = 14
