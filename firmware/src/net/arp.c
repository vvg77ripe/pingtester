
#include <config.h>
#include <string.h>
#include <stdio.h>

#include <net/bridge.h>
#include <net/ip.h>

#include "arp.h"


#define MAX_ARP_TABLE		16
#define MAX_ARP_RETRIES		4

#define ARP_FLAG_VALID		0x01
#define ARP_FLAG_REQUESTING	0x02

#define	ARP_REQUEST			1
#define	ARP_REPLY			2

/* ARP packet structure */
typedef struct {
	uint16		ar_hrd;
	uint16		ar_pro;
	uint8		ar_hln;		/* hardware address length */
	uint8		ar_pln;		/* protocol address length */
	uint16		opcode;
	uint8		ar_sha[6];	/* ethernet hw address */
	uint32		ar_spa;		/* ip address */
	uint8		ar_tha[6];	/* ethernet hw address */
	uint32		ar_tpa;		/* ip address */
} PACKED arp_frame_hdr;

/* ARP table entry structure */
typedef struct {
	unsigned char	flags;
	unsigned char	expire;
	unsigned char	retries;
	unsigned char	macad[6];
	unsigned int	ipad;
} arp_table_entry;

/* ARP table */
static arp_table_entry arptable[MAX_ARP_TABLE];


/* ===== Exported functions ===== */

void arpTimers()
{
	int i;

	for (i = 0; i < MAX_ARP_TABLE; i++) {
		/* Check time to live */
		if (arptable[i].flags & ARP_FLAG_VALID) {
			if (arptable[i].expire) {
				/* Decrement expire counter */
				arptable[i].expire--;
			} else {
				/* Delete expired entry */
				arptable[i].flags &= ~ARP_FLAG_VALID;
			}
		}

		/* Repeat arp requests */
		if (arptable[i].flags & ARP_FLAG_REQUESTING) {
			arptable[i].retries++;
			if (arptable[i].retries < MAX_ARP_RETRIES) {
				arpSendRequest(arptable[i].ipad);
			} else {
				arptable[i].flags &= ~ARP_FLAG_REQUESTING;
			}
		}
	}
}

void arpPacketHandler(unsigned char *packet, unsigned short size)
{
	arp_frame_hdr *arp;
	pktbuf buf;
	uint32 ipad;

	/* ARP header */
	arp = (arp_frame_hdr *)packet;

	/* Check target ip address */
	ipad = ntohl(arp->ar_tpa);
	if (ipad != ipGetAddress()) return;

	switch (ntohs(arp->opcode)) {
		case ARP_REQUEST:
			/* Construct ARP reply */
			arp->ar_tpa = arp->ar_spa;
			memcpy(arp->ar_tha, arp->ar_sha, 6);
			arp->opcode = htons(ARP_REPLY);

			/* Source MAC and IP address */
			memcpy(arp->ar_sha, ifGetAddress(), 6);
			arp->ar_spa = htonl(ipad);

			/* Send reply */
			buf.next = NULL;
			buf.data = (unsigned char *)arp;
			buf.len = sizeof(arp_frame_hdr);
			ifSendPacket(arp->ar_tha, ETH_TYPE_ARP, &buf);
			break;

		case ARP_REPLY:
			arpTableUpdate(ntohl(arp->ar_spa), arp->ar_sha, ARP_EXPIRE_REPLIED);
			break;
	}
}

void arpTableUpdate(unsigned int ipad, unsigned char *macad, unsigned char expire)
{
	int i, ii;

	/* Find existing entry or empty space */
	ii = -1;
	for (i = 0; i < MAX_ARP_TABLE; i++) {
		if (arptable[i].ipad == ipad) break;
		if ( (arptable[i].flags == 0) && (ii < 0) ) ii = i;
	}

	if (i >= MAX_ARP_TABLE) i = ii;
	if (i < 0) return;

	arptable[i].flags = ARP_FLAG_VALID;
	if (arptable[i].expire < expire) arptable[i].expire = expire;
	memcpy(arptable[i].macad, macad, 6);
}

unsigned char *arpTableEntry(unsigned int ipad)
{
	int i, ii;

	/* Find arp entry */
	ii = -1;
	for (i = 0; i < MAX_ARP_TABLE; i++) {
		if ( (arptable[i].ipad == ipad) && (arptable[i].flags) ) {
			/* If found valid entry -- return mac address */
			if (arptable[i].flags & ARP_FLAG_VALID) return arptable[i].macad;
			/* Else we already requesting arp, simply wait */
			return NULL;
		}
		if ( (arptable[i].flags == 0) && (ii < 0) ) ii = i;
	}

	/* If address is not found, attempt to request arp */
	if (ii < 0) return NULL;

	arptable[ii].ipad = ipad;
	arptable[ii].retries = 0;
	arptable[ii].flags = ARP_FLAG_REQUESTING;
	arpSendRequest(ipad);

	return NULL;
}

void arpSendRequest(unsigned int ipad)
{
	arp_frame_hdr arp;
	pktbuf buf;

	/* Clean */
	memset(&arp, 0, sizeof(arp));

	/* Hardware and protocol types */
	arp.ar_hrd = htons(0x0001);
	arp.ar_pro = htons(ETH_TYPE_IP);
	arp.ar_hln = 6;
	arp.ar_pln = 4;

	/* Source addresses */
	arp.opcode = htons(ARP_REQUEST);
	memcpy(arp.ar_sha, ifGetAddress(), 6);
	arp.ar_spa = htonl(ipGetAddress());

	/* Destination address */
	arp.ar_tpa = htonl(ipad);

	/* Send ARP request */
	buf.next = NULL;
	buf.data = (unsigned char *)&arp;
	buf.len = sizeof(arp);
	ifSendPacket((unsigned char *)"\xFF\xFF\xFF\xFF\xFF\xFF", ETH_TYPE_ARP, &buf);
}
