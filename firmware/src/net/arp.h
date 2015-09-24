
#ifndef _ARP_H
#define _ARP_H

#define ARP_EXPIRE_RECEIVED		5
#define ARP_EXPIRE_REPLIED		240

void arpTimers(void);
void arpPacketHandler(unsigned char *packet, unsigned short size);
void arpTableUpdate(unsigned int ipad, unsigned char *macad, unsigned char expire);
unsigned char *arpTableEntry(unsigned int ipad);
void arpSendRequest(unsigned int ipad);

#endif
