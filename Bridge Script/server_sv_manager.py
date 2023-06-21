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


import socket
from zeroconf import ServiceInfo
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
    """ This class manages the ot clients and associated information.New children are
    found when the client sends a message to a known resource on the server"""

    # create dictionary of clients accepting service - sensitivity list
    client_ip6 = dict[IPv6Address, OtDevice]()
    # CoAP server IPv6
    self_ip6 = ipaddress.IPv6Address

    # Queue for new children to be allocated a resource
    incoming_queue_res_child_ips = set[IPv6Address]()
    pend_queue_res_child_ips = set[IPv6Address]()

    def __init__(self, self_ip: IPv6Address):
        self.self_ip6 = self_ip

    def DNS_register_service(self, port):
        zeroconf = Zeroconf()

        # Define the service information
        service_name = "My CoAP Server"
        service_type = "_coap._udp.local."  # CoAP service type
        device_port = port

        # Create the service info object
        service_info = ServiceInfo(
            service_type,
            f"{service_name}.{service_type}",
            addresses=[socket.inet_pton(socket.AF_INET6, str(self.self_ip6))],
            port=device_port,
            properties={},
        )
        try:
            zeroconf.register_service(service_info) # Register the service
            logging.info("successful: Server Advertised")
            zeroconf.close()  # Close the Zeroconf instance

        except KeyError:
            logging.warning("unsuccessful: Server Not Advertised")
            raise KeyError

    def get_all_child_ips(self):
        """ Returns a dict of all children in the sensitivity list """
        return self.client_ip6

    def update_child_uri(self):
        """Add incoming clients to the resource tree"""
        while len(ServerManager.incoming_queue_res_child_ips) > 0:
            ip = ServerManager.incoming_queue_res_child_ips.pop()

            if ip not in ServerManager.client_ip6:
                try:
                    ServerManager.pend_queue_res_child_ips.add(ip)
                    logging.info(str(ip) + " added to child pending queue")
                    tmp_uri = ''.join(random.choices(string.ascii_letters + string.digits, k=16))
                    ServerManager.client_ip6[ip] = OtDevice(uri=tmp_uri)
                    logging.info(str(ip) + " updated in child sensitivity list with resource " + tmp_uri)
                    return ip

                except KeyError:
                    logging.warning("Unable to updated in child sensitivity list with resource ")
                    raise ValueError

    def update_child_device_info(self, ip: IPv6Address, ls: float , csv: list):
        """ Updates the sensitivity list with new information from the child """
        try:
            self.client_ip6[ip].last_seen = ls
            self.client_ip6[ip].timeout_cyc = OT_DEVICE_TIMEOUT_CYCLES
            """ work in progress...."""
            # Updates the  sensitivity list with new information from update odourguard PUT ....

        except KeyError:
            logging.warning("Child " + str(ip) + " not found in sensitivity list")
            raise ValueError
