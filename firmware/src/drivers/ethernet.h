
#ifndef _ETHERNET_H
#define _ETHERNET_H

#define ETH_MTU					1514

#define PHY_REG17_MDIX			(1 << 6)
#define PHY_REG17_LINK			(1 << 10)
#define PHY_REG17_RESOLVED		(1 << 11)
#define PHY_REG17_FD			(1 << 13)
#define PHY_REG17_100			(1 << 14)

void EthInit(void);
void EthShutdown(void);

unsigned short EthPHYRead(unsigned char reg);
void EthPHYWrite(unsigned char reg, unsigned short val);

#endif
