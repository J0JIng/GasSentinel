import enum
import ipaddress
import random
import string
import subprocess
import asyncio
import time

from ipaddress import IPv6Address

from dataclasses import dataclass, field
import logging
import aiocoap
from aiocoap import *
from aiocoap.protocol import Request

from zeroconf import ServiceBrowser, ServiceListener, Zeroconf

OT_DEVICE_TIMEOUT_CYCLES = 5
OT_DEVICE_CHILD_TIMEOUT_S = 190
OT_DEVICE_CHILD_TIMEOUT_CYCLE_RATE = 1
OT_DEVICE_POLL_INTERVAL_S = 5

@dataclass
class OtDevice:
    """ Generic class for an OpenThread device. """
    device_type: OtDeviceType = field(default=OtDeviceType.UNKNOWN)
    eui64: int = field(default=0)
    uri: str = field(default="")
    last_seen: float = field(default=0)
    timeout_cyc: int = field(default=OT_DEVICE_TIMEOUT_CYCLES)
    ctr: int = field(default=0)

    device_flag: bool = field(default=False)
    device_conf: int = field(default=0)
    device_dist: int = field(default=0)
    opt_lux: int = field(default=0)
    vdd: int = field(default=0)
    rssi: int = field(default=0)

class ServerManager:
    """ This class manages the ot clients and associated information.
    New children are found via the client requesting for the server's
    service and is authenticated by a general token"""

    # create dictionary of clients accepting service - sensitivity list
    client_ip6 = dict[IPv6Address, OtDevice]()
    # CoAP server IPv6
    self_ip6 = ipaddress.IPv6Address

    def __init__(self, self_ip: IPv6Address):
        self.self_ip6 = self_ip
        logging.info("Registered self ip as " + str(self.self_ip6))
        self._general_token = "mygeneraltoken"

    def get_all_child_ips(self):
        """ Returns a dict of all children in the sensitivity list """
        return self.client_ip6

    def update_child_device_info(self, ip: IPv6Address, ls: float , csv: list):
        """ Updates the sensitivity list with new information from the child """
        try:
            self.client_ip6[ip].last_seen = ls
            self.child_ip6[ip].timeout_cyc = OT_DEVICE_TIMEOUT_CYCLES
            # Updates the  sensitivity list with new information from update odourguard PUT ....

        except KeyError:
            logging.warning("Child " + str(ip) + " not found in sensitivity list")
            raise ValueError