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
    """This resource supports the PUT method.
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
            token = csv[0]
            if token != self.sv_mgr._general_token:  # Check if token matches the private token in ServerManager
                raise ValueError("Invalid token")
        except (IndexError, ValueError):
            logging.warning("Invalid token or missing general token")
            return aiocoap.Message(code=aiocoap.UNAUTHORIZED)

        # Continue processing if the token is valid
        try:
            if csv[1] == "0":  # if it is OdourGuard
                self.sv_mgr.update_child_device_info(ipaddress.ip_address(re.sub(r"[\[\]]", "", client_ip)), csv)
            else:
                raise ValueError("Invalid payload")
        except (IndexError, ValueError):
            logging.warning("Invalid payload")
            return aiocoap.Message(code=aiocoap.BAD_REQUEST)

        user_handler_callback(ipaddress.ip_address(re.sub(r"[\[\]]", "", client_ip)), csv)
        return aiocoap.Message(code=aiocoap.CON)
