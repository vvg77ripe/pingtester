
#ifndef _DHCP_H
#define _DHCP_H


#define DHCP_STATE_INACTIVE		0	/* Off */
#define DHCP_STATE_DISCOVER		1	/* Discovering dhcp server */
#define DHCP_STATE_REQUEST		2	/* Requesting ip address */

#define DHCP_SERVER_PORT		67
#define DHCP_CLIENT_PORT		68

#define DHCP_OPTIONS_SIZE		128

#define DHCP53_DHCPDISCOVER		1
#define DHCP53_DHCPOFFER		2
#define DHCP53_DHCPREQUEST		3
#define DHCP53_DHCPDECLINE		4
#define DHCP53_DHCPACK			5
#define DHCP53_DHCPNAK			6
#define DHCP53_DHCPRELEASE		7

typedef struct {
	unsigned char	op;
	unsigned char	htype;
	unsigned char	hlen;
	unsigned char	hops;
	unsigned int	xid;
	unsigned short	secs;
	unsigned short	flags;
	unsigned int	ciaddr;			/* Client IP address */
	unsigned int	yiaddr;			/* Your Ip address */
	unsigned int	siaddr;			/* Server IP address */
	unsigned int	giaddr;			/* Relay IP address */
	unsigned char	chaddr[16];		/* MAC address */
	unsigned char	sname[64];
	unsigned char	file[128];
	unsigned char	options[DHCP_OPTIONS_SIZE];
} PACKED dhcp_frame;

void dhcpStart(void);
void dhcpTimers(void);

#endif
