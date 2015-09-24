
#ifndef _BRIDGE_H
#define _BRIDGE_H

/* Interfaces index */
#define BRI_IF_LOCAL		0
#define BRI_IF_ETHERNET		1
#define BRI_IF_USB			2

/* Ethernet protocol types */
#define ETH_TYPE_IP			0x0800
#define ETH_TYPE_ARP		0x0806


/* Packet buffer descriptor */
typedef struct packetbuffer pktbuf;
struct packetbuffer {
	pktbuf *		next;
	unsigned char * data;
	unsigned short	len;
};

/* Byte swapping functions */

#define __byte_swap_long_constant(x)	\
	((((x) & 0xff000000) >> 24) |	\
	 (((x) & 0x00ff0000) >>  8) |	\
	 (((x) & 0x0000ff00) <<  8) |	\
	 (((x) & 0x000000ff) << 24))

#define	__byte_swap_word_constant(x)	\
	((((x) & 0xff00) >> 8) |	\
	 (((x) & 0x00ff) << 8))

#define __byte_swap_word(x) __byte_swap_word_constant(x)
#define __byte_swap_long(x) __byte_swap_long_constant(x)

#define ntohs(x) __byte_swap_word(x)
#define htons(x) __byte_swap_word(x)
#define ntohl(x) __byte_swap_long(x)
#define htonl(x) __byte_swap_long(x)


typedef void (*ifSendHandler)(pktbuf *packet);

void ifRecvPoll(void);
void ifSendPacket(unsigned char *dest, unsigned short proto, pktbuf *packet);
unsigned char *ifGetAddress(void);

void briInit(void);
void briPacketRecv(unsigned char iface, pktbuf *packet);
void briIfRegister(unsigned char ifindex, char *ifname, ifSendHandler ifsend, unsigned char *ifaddr);

#endif
