
#ifndef _EEPROM_H
#define _EEPROM_H

void i2cInit(void);
int eepromBlockRead(unsigned char *buf, unsigned int addr, unsigned short size);
int eepromBlockWrite(unsigned char *buf, unsigned int addr, unsigned short size);

#endif
