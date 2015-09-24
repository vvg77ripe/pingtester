
#ifndef _IP_H
#define _IP_H

#include <net/bridge.h>

/* IP Interface states */
#define IP_STATE_DOWN		0	/* Interface is down */
#define IP_STATE_STATIC		1	/* Up, static address */
#define IP_STATE_DHCPREQ	2	/* Requesting address from DHCP */
#define IP_STATE_DHCP		3	/* Up, dynamic address */

/* Defined IP protocols */
#define IP_PROTO_ICMP	1
#define IP_PROTO_TCP	6
#define IP_PROTO_UDP	17

/* Definition of an IP packet header */
typedef struct {
	uint8		version_ihl;
	uint8		service_type;
	uint16		total_length;
	uint16		identification;
	uint16		flags_frag_offset;
	uint8		ttl;
	uint8		protocol;
	uint16		checksum;
	uint32		source_addr;
	uint32		dest_addr;
	uint8		options;	/* actually an array of undetermined length */
} PACKED ip_frame_hdr;

/* UDP packet header */
typedef struct {
	uint16		sport;
	uint16		dport;
	uint16		length;
	uint16		checksum;
} PACKED udp_frame_hdr;

/* Macros for accessing an IP datagram.  */
#define IP_VERSION(a)	((a->version_ihl & 0x00F0) >> 4)
#define IP_IHL(a)		((a->version_ihl & 0x000F))
#define IP_DATA(a)		(&((uint8 *)a)[IP_IHL(a) * 4])


void ipInit(void);
void ipTimers(void);
void ipUpdateConfig(void);
void ipPacketHandler(unsigned char *packet, unsigned short size);
unsigned int ipGetAddress(void);
uint16 ip_chksum(uint16 csum, uint8 *data, int num);
void ipFillHeader(ip_frame_hdr *ip, unsigned int from, unsigned int to, unsigned char protocol);
int ipSendPacket(ip_frame_hdr *ip, pktbuf *data);

/* ICMP */

typedef void (*icmpHandler)(ip_frame_hdr *ip, unsigned char *data, unsigned short size);

void icmpRegisterHandler(icmpHandler newhandler);

/* UDP */

typedef void (*udpHandler)(ip_frame_hdr *ip, unsigned short sport, unsigned short dport,
						   unsigned char *data, unsigned short size);

void udpRegisterHandler(unsigned short port, udpHandler handler);
void udpSendPacket(ip_frame_hdr *ip, unsigned short pfrom, unsigned short pto, pktbuf *data);

/* Utilites */

char *inet_ntoa(char *buffer, unsigned char *ipad);
int inet_aton(unsigned char *buffer, char *ipad);

#endif
