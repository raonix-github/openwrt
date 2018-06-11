/* vi: set sw=4 ts=4: */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>

#if	0
#define DEBUG(format, args...) { printf("[LOOPBACK]"format, ## args); }
#else
#define DEBUG(format, args...) { }
#endif

#define DEBUG_TRC()		{printf("[TRC %s:%d]\n", __FUNCTION__, __LINE__);}

// #define ETH_P_LOOP		0x9000

static struct in_addr src;
static struct in_addr dst;
static struct sockaddr_ll me;
static struct sockaddr_ll he;
static unsigned last;

static int sock;
static unsigned count = 2;
static unsigned timeout_us=7000000;
static unsigned sent;

struct loopbackhdr {
	unsigned int magic;
};

static int send_pack(struct in_addr *src_addr,
			struct in_addr *dst_addr, struct sockaddr_ll *ME,
			struct sockaddr_ll *HE)
{
	int err;
	unsigned char buf[256];
	struct loopbackhdr *h = (struct loopbackhdr *) buf;
	unsigned char *p = (unsigned char *) (h + 1);

	h->magic = htonl(0x12345678);

	err = sendto(sock, buf, p - buf, 0, (struct sockaddr *) HE, sizeof(*HE));
	if (err == p - buf)
		return 0;

	return err;
}

static int recv_pack(unsigned char *buf, int len, struct sockaddr_ll *FROM)
{
	struct loopbackhdr *h = (struct loopbackhdr *) buf;

	if(ntohl(h->magic) == 0x12345678)
	{
		DEBUG("Receive loopback packet !!!\n");
		return 0;
	}
	else 
		DEBUG("Receive unkown packet !!!\n");

	return 1;
}

int loopback(char *eth)
{
	int ifindex;
	unsigned char *rxpkt;
	int rxlen;
	struct sockaddr_ll from;
	socklen_t alen = sizeof(from);

	fd_set recvset;
	struct timeval tv;
	int state, ret=0;

	sock = socket(PF_PACKET, SOCK_DGRAM, 0);
	if(sock < 0)
	{
		printf("socket(AF_PACKET) is error\n");
		return -1;
	}

	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, eth, IFNAMSIZ - 1);
	if(ioctl(sock, SIOCGIFINDEX, (char *)&ifr) < 0)
	{
		printf("interface %s not found\n", eth);
		ret= -1;
		goto end;
	}
	ifindex = ifr.ifr_ifindex;

	if(ioctl(sock, SIOCGIFFLAGS, (char *) &ifr) < 0)
	{
		printf("ioctl %#x failed", SIOCGIFFLAGS);
		ret= -1;
		goto end;
	}

	if (!(ifr.ifr_flags & IFF_UP)) {
//		printf("interface %s is down", eth);
		ret= -1;
		goto end;
	}

	me.sll_family = AF_PACKET;
	me.sll_ifindex = ifindex;
//	me.sll_protocol = htons(ETH_P_LOOP);
	me.sll_protocol = htons(0x0806); // RT2880 drops 0x9000

	if(bind(sock, (struct sockaddr *) &me, sizeof(me)))
	{
		printf("bind error\n");
		ret= -1;
		goto end;
	}

	{
		socklen_t alen = sizeof(me);

		if (getsockname(sock, (struct sockaddr *) &me, &alen) == -1)
		{
			printf("getsockname error");
			ret= -1;
			goto end;
		}
	}

	if (me.sll_halen == 0)
	{
		printf("interface \"%s\" is no ll address", eth);
		ret= -1;
		goto end;
	}

	he = me;
	// memset(he.sll_addr, -1, he.sll_halen);
	he.sll_addr[0] = 0xff;
	he.sll_addr[1] = 0xff;
	he.sll_addr[2] = 0xff;
	he.sll_addr[3] = 0xff;
	he.sll_addr[4] = 0xff;
	he.sll_addr[5] = 0xff;

	send_pack(&src, &dst, &me, &he);

	rxpkt = malloc(4096);
	if(rxpkt == NULL)
	{
		printf("fail to malloc !!!\n");
		ret= -1;
		goto end;
	}

	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	FD_ZERO(&recvset);
	FD_SET(sock, &recvset);

	state=select(sock+1, &recvset, NULL, NULL, &tv);
	if(state)
	{
		rxlen = recvfrom(sock, rxpkt, 4096, 0, (struct sockaddr *) &from, &alen);
	}
	else
	{
		DEBUG("loopback is timeout in select()\n");
		ret= -1;
		goto end;
	}

	if(recv_pack(rxpkt, rxlen, &from))
		ret= -1;

end:
	close(sock);
	return ret;
}
