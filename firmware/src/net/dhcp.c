
#include <config.h>
#include <string.h>

#include <net/bridge.h>
#include <net/ip.h>
#include "dhcp.h"


static unsigned int dhcp_state;
static unsigned int dhcp_xid;
static unsigned int dhcp_retries;


/* ===== Private functions ===== */

static void dhcpSendRequest()
{
	ip_frame_hdr ip;
	dhcp_frame dhcp;
	int ol;
	pktbuf pkt;
	unsigned char *macad;

	/* Prepare ip header */
	ipFillHeader(&ip, 0, 0xFFFFFFFF, 0);

	/* Prepare dhcp frame */
	memset(&dhcp, 0, sizeof(dhcp));
	dhcp.op = 1;
	dhcp.htype = 1;
	dhcp.hlen = 6;
	dhcp.xid = htonl(dhcp_xid);

	/* Source MAC address */
	macad = ifGetAddress();
	memcpy(dhcp.chaddr, macad, 6);

	/* DHCP options */
	dhcp.options[0] = 99;
	dhcp.options[1] = 130;
	dhcp.options[2] = 83;
	dhcp.options[3] = 99;
	ol = 4;

	if (dhcp_state == DHCP_STATE_DISCOVER) {
		/* DHCP Message type */
		dhcp.options[ol++] = 53;
		dhcp.options[ol++] = 1;
		dhcp.options[ol++] = DHCP53_DHCPDISCOVER;
	}

	/* Send frame */
	pkt.next = NULL;
	pkt.data = (unsigned char *)&dhcp;
	pkt.len = sizeof(dhcp_frame) - DHCP_OPTIONS_SIZE + ol;
	udpSendPacket(&ip, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &pkt);
}

static void dhcpPacketHandler(ip_frame_hdr *ip, unsigned short sport, unsigned short dport,
						   unsigned char *data, unsigned short size)
{
	dhcp_frame *dhcp;

	/* Check frame size and source */
	if (size < (sizeof(dhcp_frame) - DHCP_OPTIONS_SIZE)) return;
	if (sport != DHCP_SERVER_PORT) return;
	if (dport != DHCP_CLIENT_PORT) return;

	/* Reply expected */
	dhcp = (dhcp_frame *)data;
	if (dhcp->op != 2) return;
}

/* ===== Exported functions ===== */

void dhcpStart()
{
	/* Listen on client port */
	udpRegisterHandler(DHCP_CLIENT_PORT, dhcpPacketHandler);

	/* Send initial request */
	dhcp_state = DHCP_STATE_DISCOVER;
	dhcp_xid = 1;
	dhcp_retries = 0;
	dhcpSendRequest();
}

void dhcpTimers()
{
}
