
#include <board.h>
#include <stdio.h>
#include <string.h>
#include <aic/aic.h>

#include <drivers/display.h>
#include <net/bridge.h>
#include <registry.h>

#include "ethernet.h"


/* PHY device address */
#define ETH_PHY_ADDR		16

/* PHY interface pins */
#define ETH_PIN_MII			0x3FFFF;		/* Port B */
#define ETH_PIN_IRQ			(1 << 14);		/* Port A */
#define ETH_PIN_RESET		(1 << 15);		/* Port A */

#define ETH_RX_BUFCOUNT		32
#define ETH_RX_BUFSIZE		128
#define ETH_TX_BUFCOUNT		2
#define ETH_TX_BUFSIZE		1536

#define RXBUF_OWNERSHIP     0x00000001
#define RXBUF_WRAP          0x00000002
#define RXBUF_ADDRMASK      0xFFFFFFFC

#define RXS_BROADCAST_ADDR  0x80000000  /*!< \brief Broadcast address detected. */
#define RXS_MULTICAST_HASH  0x40000000  /*!< \brief Multicast hash match. */
#define RXS_UNICAST_HASH    0x20000000  /*!< \brief Unicast hash match. */
#define RXS_EXTERNAL_ADDR   0x10000000  /*!< \brief External address match. */
#define RXS_SA1_ADDR        0x04000000  /*!< \brief Specific address register 1 match. */
#define RXS_SA2_ADDR        0x02000000  /*!< \brief Specific address register 2 match. */
#define RXS_SA3_ADDR        0x01000000  /*!< \brief Specific address register 3 match. */
#define RXS_SA4_ADDR        0x00800000  /*!< \brief Specific address register 4 match. */
#define RXS_TYPE_ID         0x00400000  /*!< \brief Type ID match. */
#define RXS_VLAN_TAG        0x00200000  /*!< \brief VLAN tag detected. */
#define RXS_PRIORITY_TAG    0x00100000  /*!< \brief Priority tag detected. */
#define RXS_VLAN_PRIORITY   0x000E0000  /*!< \brief VLAN priority. */
#define RXS_CFI_IND         0x00010000  /*!< \brief Concatenation format indicator. */
#define RXS_EOF             0x00008000  /*!< \brief End of frame. */
#define RXS_SOF             0x00004000  /*!< \brief Start of frame. */
#define RXS_RBF_OFFSET      0x00003000  /*!< \brief Receive buffer offset mask. */
#define RXS_LENGTH_FRAME    0x000007FF  /*!< \brief Length of frame including FCS. */

#define TXS_USED            0x80000000  /*!< \brief Used buffer. */
#define TXS_WRAP            0x40000000  /*!< \brief Last descriptor. */
#define TXS_ERROR           0x20000000  /*!< \brief Retry limit exceeded. */
#define TXS_UNDERRUN        0x10000000  /*!< \brief Transmit underrun. */
#define TXS_NO_BUFFER       0x08000000  /*!< \brief Buffer exhausted. */
#define TXS_NO_CRC          0x00010000  /*!< \brief CRC not appended. */
#define TXS_LAST_BUFF       0x00008000  /*!< \brief Last buffer of frame. */


/* ===== Variables ===== */

typedef struct bufdescr {
	unsigned int	addr;
	unsigned int	stat;
} EthBuffer;

/* Receive buffers */
static EthBuffer rxlist[ETH_RX_BUFCOUNT];
static unsigned char rxbuf[ETH_RX_BUFCOUNT * ETH_RX_BUFSIZE] __attribute__ ((aligned(8)));
static unsigned char rxindex;
static pktbuf rxchunk[2];

/* Transmit buffers */
static EthBuffer txlist[ETH_TX_BUFCOUNT];
static unsigned char txbuf[ETH_TX_BUFCOUNT * ETH_TX_BUFSIZE] __attribute__ ((aligned(8)));
static unsigned char txindex;

/* ===== Internal functions ===== */

/* ethGetRecvFrame()
 *   Gets next received frame from EMAC rx buffers
 */
static int ethGetRecvFrame()
{
	unsigned char i;
	unsigned short flen;

	/* Search next frame start marker */
	while ((rxlist[rxindex].addr & RXBUF_OWNERSHIP) != 0 && (rxlist[rxindex].stat & RXS_SOF) == 0) {
		rxlist[rxindex].addr &= ~(RXBUF_OWNERSHIP);
		rxindex++;
		if (rxindex >= ETH_RX_BUFCOUNT) rxindex = 0;
	}

	/* Walk to end of the frame */
	i = rxindex;
	flen = 0;
	while (rxlist[i].addr & RXBUF_OWNERSHIP) {
		/* Unexpected SOF -- skip truncated frame */
		if (i != rxindex && (rxlist[i].stat & RXS_SOF) != 0) {
			do {
				rxlist[rxindex].addr &= ~(RXBUF_OWNERSHIP);
				rxindex++;
				if (rxindex >= ETH_RX_BUFCOUNT) rxindex = 0;
			} while ((rxlist[rxindex].addr & RXBUF_OWNERSHIP) != 0 && (rxlist[rxindex].stat & RXS_SOF) == 0);
			break;
		}

		/* Last buffer in the frame */
		if ((flen = rxlist[i].stat & RXS_LENGTH_FRAME) != 0) break;

		i++;
		if (i >= ETH_RX_BUFCOUNT) i = 0;
	}

	/* No valid frame found */
	if (!flen) return 0;

	if (i < rxindex) {
		/* Buffers wrapped */
		rxchunk[0].next = &rxchunk[1];
		rxchunk[0].data = &rxbuf[rxindex * ETH_RX_BUFSIZE];
		rxchunk[0].len = (ETH_RX_BUFCOUNT - rxindex) * ETH_RX_BUFSIZE;
		rxchunk[1].next = NULL;
		rxchunk[1].data = rxbuf;
		rxchunk[1].len = flen - rxchunk[0].len;
		briPacketRecv(BRI_IF_ETHERNET, rxchunk);
	} else {
		/* Unwrapped buffers */
		rxchunk[0].next = NULL;
		rxchunk[0].data = &rxbuf[rxindex * ETH_RX_BUFSIZE];
		rxchunk[0].len = flen;
		briPacketRecv(BRI_IF_ETHERNET, rxchunk);
	}

	/* Free frame buffers */
	while (1) {
		i = rxindex;
		rxlist[rxindex].addr &= ~(RXBUF_OWNERSHIP);
		rxindex++;
		if (rxindex >= ETH_RX_BUFCOUNT) rxindex = 0;

		if (rxlist[i].stat & RXS_EOF) break;
	}

	return 1;
}

/* ISR_Ethernet()
 *   EMAC interrupt handler
 */
static void ISR_Ethernet()
{
	unsigned int status = AT91C_BASE_EMAC->EMAC_ISR;

	/* Frame received */
	if (status & AT91C_EMAC_RCOMP) {

		/* Get all received frames */
		while (ethGetRecvFrame());

		AT91C_BASE_EMAC->EMAC_RSR = AT91C_EMAC_REC;
		return;
	}

	/* Frame received */
	if (status & AT91C_EMAC_RXUBR) {

		/* Get all received frames */
		while (ethGetRecvFrame());

		AT91C_BASE_EMAC->EMAC_RSR = AT91C_EMAC_BNA;
		return;
	}
}

static void ethSendFrame(pktbuf *packet)
{
	unsigned char *data;
	unsigned int size;
	pktbuf *buf;

	data = &txbuf[txindex * ETH_TX_BUFSIZE];
	size = 0;

	/* Copy packet to send buffer */
	for (buf = packet; buf; buf = buf->next) {
		/* Check buffer overflow */
		if ( (size + buf->len) > ETH_TX_BUFSIZE ) return;

		/* Copy data */
		memcpy(data, buf->data, buf->len);
		
		data += buf->len;
		size += buf->len;
	}

	size |= TXS_LAST_BUFF;
	if (txindex == (ETH_TX_BUFCOUNT-1)) size |= TXS_WRAP;

	/* Start TX */
	txlist[txindex].stat = size;
	txindex++;
	if (txindex >= ETH_TX_BUFCOUNT) txindex = 0;
	AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_TSTART;
}

/* ===== Interface functions ===== */

/* EthInit()
 *   Initializes Ethernet controller
 */
void EthInit()
{
	int i;
	unsigned char *v;

	// Configure interrupt pin
	AT91C_BASE_PIOA->PIO_PER = ETH_PIN_IRQ;
	AT91C_BASE_PIOA->PIO_ODR = ETH_PIN_IRQ;

	/* Disable Pull-UP for INT, RXDV and COL pins */
	AT91C_BASE_PIOA->PIO_PPUDR = ETH_PIN_IRQ;
	AT91C_BASE_PIOB->PIO_PPUDR = (1 << 15) | (1 << 16);

	// Enable PHY power
	AT91C_BASE_PIOA->PIO_PER = 0x8001;
	AT91C_BASE_PIOA->PIO_SODR = ETH_PIN_RESET;				/* Reset */
	AT91C_BASE_PIOA->PIO_SODR = 0x0001;						/* Power on */
	AT91C_BASE_PIOA->PIO_OER = 0x8001;

	/* Enable MII pins */
	AT91C_BASE_PIOB->PIO_PDR = ETH_PIN_MII;
	AT91C_BASE_PIOB->PIO_ASR = ETH_PIN_MII;

	// RESET pulse
	sleep(200);
	AT91C_BASE_PIOA->PIO_SODR = ETH_PIN_RESET;

	// Configure ethernet controller
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_EMAC) | (1 << AT91C_ID_PIOB);
	AT91C_BASE_EMAC->EMAC_NCFGR = AT91C_EMAC_SPD | AT91C_EMAC_FD | AT91C_EMAC_CLK_HCLK_16 | AT91C_EMAC_CAF;
	AT91C_BASE_EMAC->EMAC_NCR = AT91C_EMAC_MPE;

	/* Configure MAC address */
	v = regGetValue(SYS_REG_MAC_ADDRESS, NULL);
	if (v) {
		/* System MAC address */
		AT91C_BASE_EMAC->EMAC_SA1L = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
		AT91C_BASE_EMAC->EMAC_SA1H = (v[4] << 8) | v[5];
		/* ADM8511 MAC address */
		v[5] |= 1;
		AT91C_BASE_EMAC->EMAC_SA2L = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
		AT91C_BASE_EMAC->EMAC_SA2H = (v[4] << 8) | v[5];
	}

	/* Wait for hardware reset complete and configure PHY */
	sleep(100);
	EthPHYWrite(16, 0x0130);				/* Enable auto-crossover */
	EthPHYWrite(28, 0x0800);				/* MII mode, Copper */
	EthPHYWrite(0, 0xB100);					/* Software reset, ANEG enable */

	/* Wait for reset complete, continue PHY configuration */
	sleep(5);
	EthPHYWrite(22, 0x0EB5);				/* LED0: Link, LED1: Act, LED2: Interrupt */
	EthPHYWrite(25, 0x0000);

	/* Initialize buffer descriptors */
	for (i = 0; i < ETH_RX_BUFCOUNT; i++) {
		rxlist[i].addr = (unsigned int) &rxbuf[ETH_RX_BUFSIZE * i];
		if (i == (ETH_RX_BUFCOUNT-1)) rxlist[i].addr |= 2;
	}
	AT91C_BASE_EMAC->EMAC_RBQP = (unsigned int) rxlist;

	txlist[0].addr = (unsigned int)&txbuf[0];
	txlist[0].stat = TXS_USED;
	txlist[1].addr = (unsigned int)&txbuf[ETH_TX_BUFSIZE];
	txlist[1].stat = TXS_USED | TXS_WRAP;
	AT91C_BASE_EMAC->EMAC_TBQP = (unsigned int) txlist;

	/* Start controller */
	AT91C_BASE_EMAC->EMAC_RSR = 7;
	AT91C_BASE_EMAC->EMAC_USRIO = AT91C_EMAC_CLKEN;
	AT91C_BASE_EMAC->EMAC_NCR = AT91C_EMAC_MPE | AT91C_EMAC_RE | AT91C_EMAC_TE | AT91C_EMAC_WESTAT;

	/* Configure interrupts */
	AT91C_BASE_EMAC->EMAC_IER = AT91C_EMAC_RCOMP | AT91C_EMAC_RXUBR;
	AIC_ConfigureIT(AT91C_ID_EMAC, 0, ISR_Ethernet);
	AIC_EnableIT(AT91C_ID_EMAC);

	/* Register bridge interface */
	briIfRegister(BRI_IF_ETHERNET, "Ethernet interface", ethSendFrame, NULL);
}

void EthShutdown()
{
	/* Disable EMAC */
	AT91C_BASE_EMAC->EMAC_NCR = 0;

	/* Set all control lines low */
	AT91C_BASE_PIOA->PIO_CODR = ETH_PIN_RESET;
	AT91C_BASE_PIOB->PIO_CODR = ETH_PIN_MII;
	AT91C_BASE_PIOB->PIO_OER = ETH_PIN_MII;
	AT91C_BASE_PIOB->PIO_PER = ETH_PIN_MII;

	/* Power OFF */
	AT91C_BASE_PIOA->PIO_CODR = 1;
}

unsigned short EthPHYRead(unsigned char reg)
{
	/* Read command */
	AT91C_BASE_EMAC->EMAC_MAN = (1 << 30) | (1 << 17) | (1 << 29) |
		(ETH_PHY_ADDR << 23) | ((reg & 0x1F) << 18);

	/* Wait for complete */
	while ((AT91C_BASE_EMAC->EMAC_NSR & AT91C_EMAC_IDLE) == 0);

	return AT91C_BASE_EMAC->EMAC_MAN;
}

void EthPHYWrite(unsigned char reg, unsigned short val)
{
	/* Write command */
	AT91C_BASE_EMAC->EMAC_MAN = (1 << 30) | (1 << 17) | (1 << 28) |
		(ETH_PHY_ADDR << 23) | ((reg & 0x1F) << 18) | val;

	/* Wait for complete */
	while ((AT91C_BASE_EMAC->EMAC_NSR & AT91C_EMAC_IDLE) == 0);
}
