# Helper Lib
import asyncio  # pylint: disable=import-error
import ipaddress
import logging
import coloredlogs

# CoAP lib / Network
import aiocoap
from aiocoap import resource
import aiocoap.numbers.constants
import netifaces

# imported modules
#import influx_sender
from server_sv_manager import ServerManager
from server_resource_handler import ResourceHandler

# Declaring Global Variable
START_TASK_INFLUX_SENDER = False
POLL_NEW_CHILDREN_INTERVAL_S = 30

# Change accordingly to machine
COAP_UDP_DEFAULT_PORT = 5683
OT_DEFAULT_PREFIX = "fd74"
OT_DEFAULT_IFACE = "wlan0"

def get_ipv6_address(interface_name, address_prefix):
    """
    Get the IPv6 address of a specific network interface with a given address prefix.
    Returns None if no matching address is found.
    """
    try:
        addresses = netifaces.ifaddresses(interface_name)
        iteration = 0
        for address_info in addresses[netifaces.AF_INET6]:
            if address_info["addresses"].startswith(address_prefix):
                # return ipv6 addr as a string
                return addresses[netifaces.AF_INET6][iteration]["addresses"]
            iteration += 1

    except KeyError:
        logging.error(f"KeyError: The '{netifaces.AF_INET6}' key is not present")

    return None

def main(root_res: resource.Site):
    """Main function that starts the server"""
    # Resource tree creation
    server_ipv6_address = get_ipv6_address(OT_DEFAULT_IFACE,OT_DEFAULT_PREFIX)
    if server_ipv6_address:
        logging.info(f"Server running. IPv6 address: {server_ipv6_address}")
    else:
        logging.error("Failed to retrieve IPv6 address")

    logging.info("Server running")

    # Creates a server context for the CoAP server
    asyncio.create_task(
        aiocoap.Context.create_server_context(
            root_res,bind=(server_ipv6_address,COAP_UDP_DEFAULT_PORT)
        )
    )
    # create an instance of ServerManager() class
    sv_mgr = ServerManager(ipaddress.ip_address(server_ipv6_address))
    logging.info("Advertising Server...")
    # Advertise server via DNS-SD
    sv_mgr.DNS_register_service(COAP_UDP_DEFAULT_PORT)

    asyncio.get_event_loop().run_until_complete(
        # create a new coroutine that waits for the provided coroutines to complete concurrently.
        asyncio.gather(
            main_task(sv_mgr,root_res),
            # send data to an InfluxDB database if START_TASK_INFLUX_SENDER == TRUE
            #influx_sender.influx_task(ot_mgr) if START_TASK_INFLUX_SENDER else None
        )
    )

async def main_task(sv_manager: ServerManager, root_res: resource.Site):
    """Add clients to resource tree"""
    while True:
        logging.info("inviting new children...")
        ip = sv_manager.pend_queue_res_child_ips.pop()
        while ip is not None:
            try:
                root_res.add_resource(
                    (sv_manager.get_all_child_ips()[ip].uri,),
                    ResourceHandler(sv_manager.get_all_child_ips()[ip].uri, sv_manager),
                )
                logging.info(
                    "Added new child " + str(ip) + " with resource " + sv_manager.get_all_child_ips()[ip].uri
                )
            except KeyError:
                logging.info("Child " + str(ip) + " error")
                pass

        await asyncio.sleep(POLL_NEW_CHILDREN_INTERVAL_S)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    coloredlogs.install(level="INFO")

    coap_root = resource.Site()
    logging.info("Startup success")
    try:
        main(coap_root)
    except KeyboardInterrupt:
        logging.error("Exiting")
