
#ifndef _REGISTRY_H
#define _REGISTRY_H

/* Basic configuration */
#define SYS_REG_MAC_ADDRESS		1
#define SYS_REG_IP4_TYPE		2
#define SYS_REG_IP4_ADDRESS		3
#define SYS_REG_IP4_MASK		4
#define SYS_REG_IP4_GATEWAY		5
#define SYS_REG_IP4_DNS			6
#define SYS_REG_MODEL_NAME		7
#define SYS_REG_USB_DEVICE		8

void regInit(void);
void regSave(void);
unsigned char *regGetValue(unsigned char key, unsigned char *out);
int regWriteValue(unsigned char key, unsigned char *value, unsigned char len);

#endif
