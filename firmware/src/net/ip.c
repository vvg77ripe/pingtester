
#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <net/bridge.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <registry.h>

#include "ip.h"


#define ICMP_ECHO					8
#define ICMP_ECHO_REPLY				0

#define MAX_UDP_PORTS				16

typedef struct {
	unsigned short	port;
	udpHandler		handler;
} udp_handler;

/* ===== Variables ===== */

static int ip_state;
static unsigned int ip_addr;
static unsigned int ip_mask;
static unsigned int ip_gateway;

static icmpHandler icmp_handler;

static udp_handler udp_handlers[MAX_UDP_PORTS];

/* ===== ICMP protocol functions ===== */

static void icmpPacketHandler(ip_frame_hdr *ip, unsigned char *packet, unsigned short size)
{
	pktbuf buf;
	unsigned char type, code;
	unsigned short checksum;

	type = packet[0];
	code = packet[1];

	switch (type) {
		/* Echo request */
		case ICMP_ECHO:
			/* Simple rewrite icmp type and return packet to sender */
			packet[0] = ICMP_ECHO_REPLY;

			/* Calculate ICMP checksum */
			packet[2] = 0;
			packet[3] = 0;
			checksum = ip_chksum(0, packet, size);
			packet[2] = checksum >> 8;
			packet[3] = checksum & 0xFF;

			ipFillHeader(ip, htonl(ip->dest_addr), htonl(ip->source_addr), IP_PROTO_ICMP);
			buf.next = NULL;
			buf.data = packet;
			buf.len = size;
			ipSendPacket(ip, &buf);
			break;

		default:
			if (icmp_handler) icmp_handler(ip, packet, size);
			break;
	}
}

void icmpRegisterHandler(icmpHandler newhandler)
{
	icmp_handler = newhandler;
}

/* ===== UDP protocol functions ===== */

static void udpPacketHandler(ip_frame_hdr *ip, unsigned char *packet, unsigned short size)
{
	udp_frame_hdr *udp;
	int i;
	unsigned short sport, dport;

	if (size < sizeof(udp_frame_hdr)) return;

	udp = (udp_frame_hdr *)packet;
	sport = ntohs(udp->sport);
	dport = ntohs(udp->dport);

	for (i = 0; i < MAX_UDP_PORTS; i++) {
		if (udp_handlers[i].port == 0) break;
		if ( (udp_handlers[i].port == dport) && (udp_handlers[i].handler) ) {
			udp_handlers[i].handler(ip, sport, dport, &packet[sizeof(udp_frame_hdr)], size - sizeof(udp_frame_hdr));
		}
	}
}

void udpSendPacket(ip_frame_hdr *ip, unsigned short pfrom, unsigned short pto, pktbuf *data)
{
	udp_frame_hdr udp;
	pktbuf pkt;
	pktbuf *buf;
	unsigned short len;

	if (!ip) return;

	ip->protocol = IP_PROTO_UDP;

	/* Prepare udp header */
	memset(&udp, 0, sizeof(udp));
	udp.sport = htons(pfrom);
	udp.dport = htons(pto);

	/* Calculate total packet length */
	len = sizeof(udp);
	for (buf = data; buf; buf = buf->next) {
		len += buf->len;
	}
	udp.length = htons(len);

	/* Send packet */
	pkt.next = data;
	pkt.data = (unsigned char *)&udp;
	pkt.len = sizeof(udp);
	ipSendPacket(ip, &pkt);
}

void udpRegisterHandler(unsigned short port, udpHandler handler)
{
	int i;

	for (i = 0; i < MAX_UDP_PORTS; i++) {
		if ( (udp_handlers[i].port == port) || (udp_handlers[i].port == 0) ) {
			udp_handlers[i].port = port;
			udp_handlers[i].handler = handler;
			return;
		}
	}
}

/* ===== IP protocol functions ===== */

void ipInit()
{
	ipUpdateConfig();
}

void ipTimers()
{
	dhcpTimers();
	arpTimers();
}

void ipUpdateConfig()
{
	unsigned char *v;
	unsigned int x;

	ip_state = IP_STATE_STATIC;

	ip_addr = 0;
	v = regGetValue(SYS_REG_IP4_ADDRESS, (unsigned char *)&x);
	if (v) ip_addr = ntohl(x);

	ip_mask = 0;
	v = regGetValue(SYS_REG_IP4_MASK, (unsigned char *)&x);
	if (v) ip_mask = ntohl(x);

	ip_gateway = 0;
	v = regGetValue(SYS_REG_IP4_GATEWAY, (unsigned char *)&x);
	if (v) ip_gateway = ntohl(x);
}

void ipPacketHandler(unsigned char *packet, unsigned short size)
{
	ip_frame_hdr *ip;
	unsigned int ipad;
	uint8 *data;
	unsigned short len;

	ip = (ip_frame_hdr *)packet;

	/* Skip truncated packets */
	if (size < 20) return;

	/* Expect IPv4 */
	if ( (ip->version_ihl & 0xF0) != 0x40 ) return;

	/* Check destination IP */
	if (ntohl(ip->dest_addr) != ip_addr) return;

	/* Get payload pointer and size */
	data = IP_DATA(ip);
	len = ntohs(ip->total_length);
	if (size < len) return;

	/* Save sender MAC in ARP table */
	ipad = ntohl(ip->source_addr);
	if ( (ipad & ip_mask) == (ip_addr & ip_mask) ) {
		arpTableUpdate(ipad, packet - 8, ARP_EXPIRE_RECEIVED);
	}

	switch (ip->protocol) {
		case IP_PROTO_ICMP:
			icmpPacketHandler(ip, data, len - 20);
			break;

		case IP_PROTO_UDP:
			udpPacketHandler(ip, data, len - 20);
			break;
	}
}

unsigned int ipGetAddress()
{
	return ip_addr;
}

uint16 FASTCODE ip_chksum(uint16 csum, uint8 *data, int num)
{
	int chksum, ichksum;
	uint16 temp;

	chksum = csum;
	for (; num > 0; num-=2, data+=2)
	{
		temp = (*data << 8) | *(data+1);
		if (num == 1) temp &= 0xFF00;	/* Zero last byte if odd */
		ichksum = chksum + temp;
		ichksum = ichksum & 0x0000FFFF;
		if ((ichksum < temp) || (ichksum < chksum))
		{
			ichksum += 1;
			ichksum = ichksum & 0x0000FFFF;
		}
		chksum = ichksum;
	}
	return (uint16)~chksum;
}

void ipFillHeader(ip_frame_hdr *ip, unsigned int from, unsigned int to, unsigned char protocol)
{
	if (!ip) return;

	memset(ip, 0, 20);

	ip->version_ihl = 0x45;
	ip->ttl = 64;
	ip->protocol = protocol;
	ip->source_addr = htonl(from);
	ip->dest_addr = htonl(to);
}

int ipSendPacket(ip_frame_hdr *ip, pktbuf *data)
{
	unsigned char *macad;
	unsigned int ipad;
	pktbuf hdr;
	pktbuf *buf;
	unsigned short len;

	if (!ip) return 0;

	/* Route packet */
	ipad = ntohl(ip->dest_addr);
	if (ipad == 0xFFFFFFFF) {
		macad = (unsigned char *)"\xFF\xFF\xFF\xFF\xFF\xFF";
	} else {
		if ( (ipad & ip_mask) != (ip_addr & ip_mask) ) ipad = ip_gateway;
		macad = arpTableEntry(ipad);
		/* No arp entry found -- drop packet */
		if (!macad) return 0;
	}

	/* Calculate total packet length */
	len = 20;
	for (buf = data; buf; buf = buf->next) {
		len += buf->len;
	}
	ip->total_length = htons(len);

	/* Calculate header checksum */
	ip->checksum = 0;
	ip->checksum = htons(ip_chksum(0, (unsigned char *)ip, 20));

	/* Send packet */
	hdr.next = data;
	hdr.data = (unsigned char *)ip;
	hdr.len = 20;
	ifSendPacket(macad, ETH_TYPE_IP, &hdr);

	return 1;
}

/* ===== Utilites ===== */

char *inet_ntoa(char *buffer, unsigned char *ipad)
{
	if (!ipad) return NULL;
	if (!buffer) return NULL;

	sprintf(buffer, "%u.%u.%u.%u", ipad[0], ipad[1], ipad[2], ipad[3]);
	return buffer;
}

int inet_aton(unsigned char *buffer, char *ipad)
{
	char *s;
	int b1, b2, b3, b4;

	if (!ipad) return 0;
	if (!buffer) return 0;

	s = ipad;
	b1 = strtol(s, &s, 10);
	if ( (s[0] != '.') || (b1 < 0) || (b1 > 255) ) return 0;
	s++;

	b2 = strtol(s, &s, 10);
	if ( (s[0] != '.') || (b2 < 0) || (b2 > 255) ) return 0;
	s++;

	b3 = strtol(s, &s, 10);
	if ( (s[0] != '.') || (b3 < 0) || (b3 > 255) ) return 0;
	s++;

	b4 = strtol(s, &s, 10);
	if ( (b4 < 0) || (b4 > 255) ) return 0;

	buffer[0] = b1;
	buffer[1] = b2;
	buffer[2] = b3;
	buffer[3] = b4;

	return 1;
}
