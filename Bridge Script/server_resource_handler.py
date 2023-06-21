import ipaddress
import time
import re
import logging
from aiocoap import resource
import aiocoap
import tinytuya

from server_sv_manager import ServerManager
from user_handler import user_handler_callback

class ResourceHandler(resource.Resource):
    """This resource supports the PUT methods.
    PUT: Update state of alarm."""

    def __init__(self, uri, sv_mgr: ServerManager):
        super().__init__()
        self.coap_payload = None
        self.path = uri
        self.sv_mgr = sv_mgr
        logging.info("Registered resource " + str(uri))

    async def render_put(self, request):
        """ Handles PUT requests, updates info, and calls functions to trigger actions. """

        client_ip = request.remote.hostinfo
        self.coap_payload = request.payload.decode("utf-8")
        csv = self.coap_payload.split(",")
        logging.info("Received PUT request from " + str(client_ip) + " with payload " + str(csv))
        logging.warning(csv)
        try:
            # Place them into a queue to be added into resource tree
            ServerManager.incoming_queue_res_child_ips.add(ipaddress.ip_address(re.sub(r"[\[\]]", "", client_ip)))

            # Update the resource tree with client information
            self.sv_mgr.update_child_uri()

            """Work in progress"""
            # Update the information on Client
            self.sv_mgr.update_child_device_info(ipaddress.ip_address(re.sub(r"[\[\]]", "", client_ip)), csv)
        except ValueError:
            logging.warning("Invalid payload")
        user_handler_callback(ipaddress.ip_address(re.sub(r"[\[\]]", "", client_ip)), csv)
