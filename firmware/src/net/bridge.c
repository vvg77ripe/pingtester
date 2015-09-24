
#include <config.h>
#include <string.h>

#include <gui.h>
#include <net/arp.h>
#include <net/ip.h>

#include "bridge.h"


/* ===== Bridge interfaces ===== */

#define MAX_INTERFACES		3

struct iface {
	char *			name;
	ifSendHandler	ifsend;
	unsigned char	ifindex;
	unsigned char	flags;
	unsigned char	macad[6];
	unsigned int	rxcnt;
	unsigned int	txcnt;
};

static struct iface iflist[MAX_INTERFACES];

/* ===== Local interface data ===== */

#define LOCAL_MTU			1518
#define LOCAL_QUEUE_SIZE	2

static unsigned char localRecvData[LOCAL_QUEUE_SIZE][LOCAL_MTU];
static unsigned short localRecvSizes[LOCAL_QUEUE_SIZE];
static unsigned char lqFirst;
static unsigned char lqLast;

#define lqNext(index) ( (index >= (LOCAL_QUEUE_SIZE-1)) ? 0 : index + 1 )

static unsigned char if_addr[6];


/* ===== Local interface functions ===== */

static void ifRecvPacket(pktbuf *packet)
{
	unsigned char next;
	unsigned char *data;
	unsigned short size;
	pktbuf *buf;

	/* Skip packet if queue is full */
	next = lqNext(lqLast);
	if (next == lqFirst) return;

	/* Copy packet to queue */
	data = localRecvData[lqLast];
	size = 0;
	for (buf = packet; buf; buf = buf->next) {
		/* Skip large packets */
		size += buf->len;
		if (size > LOCAL_MTU) return;

		/* Copy packet data */
		memcpy(data, buf->data, buf->len);
		data += buf->len;
	}
	localRecvSizes[lqLast] = size;

	/* Increment queue pointer */
	lqLast = next;
}

void ifRecvPoll()
{
	unsigned char *data;
	unsigned short size;
	unsigned short type;

	/* Process packets in queue */
	while (lqFirst != lqLast) {
		/* Get first packet from queue */
		data = localRecvData[lqFirst];
		size = localRecvSizes[lqFirst];

		do {
			/* Skip truncated packets */
			if (size < 14) break;

			type = (data[12] << 8) | data[13];

			/* Switch to appropriate protocol handler */
			switch (type) {
				case ETH_TYPE_ARP:
					arpPacketHandler(&data[14], size - 14);
					break;

				case ETH_TYPE_IP:
					ipPacketHandler(&data[14], size - 14);
					break;
			}
		} while (0);

		/* Move queue pointer */
		lqFirst = lqNext(lqFirst);
	}
}

void ifSendPacket(unsigned char *dest, unsigned short proto, pktbuf *packet)
{
	unsigned char ethhdr[14];
	pktbuf buf;

	/* Destination MAC address */
	memcpy(&ethhdr[0], dest, 6);

	/* Source MAC address */
	memcpy(&ethhdr[6], iflist[BRI_IF_LOCAL].macad, 6);

	/* Protocol ID */
	ethhdr[12] = proto >> 8;
	ethhdr[13] = proto;

	/* Send packet to bridge */
	buf.next = packet;
	buf.data = ethhdr;
	buf.len = sizeof(ethhdr);
	briPacketRecv(BRI_IF_LOCAL, &buf);
}

unsigned char *ifGetAddress()
{
	return iflist[BRI_IF_LOCAL].macad;
}

/* ===== Ethernet bridge functions ===== */

void briInit()
{
	/* Initialize variables */
	memset(iflist, 0, sizeof(iflist));

	/* Initialize protocol handlers */
	ipInit();

	/* Register local interface */
	if_addr[0] = 0x00;
	if_addr[1] = 0x52;
	if_addr[2] = 0x42;
	if_addr[3] = 0x03;
	if_addr[4] = 0x00;
	if_addr[5] = 0x02;
	briIfRegister(0, "Local Interface", ifRecvPacket, if_addr);
}

/* briPacketRecv()
 *   Handles incoming packets from interfaces
 */
void briPacketRecv(unsigned char iface, pktbuf *packet)
{
	int i;

	if (iface >= MAX_INTERFACES) return;

	iflist[iface].rxcnt++;

	/* Send packet to all other interfaces */
	for (i = 0; i < MAX_INTERFACES; i++) if (i != iface) {
		if (iflist[i].ifsend) {
			iflist[i].ifsend(packet);
			iflist[i].txcnt++;
		}
	}

	guiUpdateCounters(iflist[BRI_IF_LOCAL].txcnt, iflist[BRI_IF_LOCAL].rxcnt);
}

void briIfRegister(unsigned char ifindex, char *ifname, ifSendHandler ifsend, unsigned char *ifaddr)
{
	if (ifindex >= MAX_INTERFACES) return;

	iflist[ifindex].name = ifname;
	iflist[ifindex].ifindex = ifindex;
	iflist[ifindex].ifsend = ifsend;
	if (ifaddr) memcpy(iflist[ifindex].macad, ifaddr, 6);
}
