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
import influx_sender
from server_sv_manager import ServerManager
from server_resource_handler import ResourceHandler
from user_handler import user_handler_init



# Declaring Global Variable
START_TASK_INFLUX_SENDER = False
POLL_NEW_CHILDREN_INTERVAL_S = 30

# Change accordingly to machine
COAP_UDP_DEFAULT_PORT = 5683
OT_DEFAULT_PREFIX = "fd74"
OT_DEFAULT_IFACE = "wpan0"

# resource.site imported from aiocoap.

def main(root_res: resource.Site):
    """Main function that starts the server"""

    # Resource tree creation
    addrs = netifaces.ifaddresses(OT_DEFAULT_IFACE)

    # Find addr
    iteration = 0
    try:
        for i in addrs[netifaces.AF_INET6]:
            if i["addr"].startswith(OT_DEFAULT_PREFIX):
                break
            iteration += 1

    except KeyError:
        # Handle the KeyError exception here
        logging.error("KeyError: The 'netifaces.AF_INET6' key is not present in 'addrs'")

    # Add an endpoint to advertise the CoAP service
    root_res.add_resource(
        ('.well-known', 'core'),
        resource.WKCResource(root_res.get_resources_as_linkheader)
    )

    """ asynchronously creates a server context for the CoAP server, which will listen
        for incoming requests on the specified IP address and port."""

    asyncio.create_task(
        aiocoap.Context.create_server_context(
            root_res,
            bind=(addrs[netifaces.AF_INET6][iteration]["addr"],COAP_UDP_DEFAULT_PORT)
        )
    )

    # log server is running
    logging.info("Server running")

    # create an instance of ServerManager() class
    sv_mgr = ServerManager()

    asyncio.get_event_loop().run_until_complete(

        # create a new coroutine that waits for the provided coroutines to complete concurrently.
        asyncio.gather(
            # Poll for children
            main_task(sv_mgr,root_res),

            # send data to an InfluxDB database if START_TASK_INFLUX_SENDER == TRUE
            #influx_sender.influx_task(ot_mgr) if START_TASK_INFLUX_SENDER else None
        )
    )

"""

not really sure if i should keep this 

async def main_task(ot_manager: ServerManager, root_res: resource.Site):
    #Main task that polls for new children and adds them to the resource tree every 30s

    while True:
        logging.info("Finding new children...")
        sv_manager.find_child_ips()
        ip = ot_manager.dequeue_child_ip()
        while ip is not None:
            try:
                root_res.add_resource(
                    (ot_manager.get_child_ips()[ip].uri,),
                    ResourceHandler(ot_manager.get_child_ips()[ip].uri, ot_manager)
                )
                logging.info(
                    "Added new child " + str(ip) + " with resource " + ot_manager.get_child_ips()[ip].uri
                )

            except KeyError:
                logging.info("Child " + str(ip) + " error")
                pass

            ip = ot_manager.dequeue_child_ip()

        await asyncio.sleep(POLL_NEW_CHILDREN_INTERVAL_S)
"""

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    coloredlogs.install(level="INFO")

    coap_root = resource.Site()
    logging.info("Startup success")
    try:
        main(coap_root)
    except KeyboardInterrupt:
        logging.error("Exiting")