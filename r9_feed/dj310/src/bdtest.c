#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>

#include "bdtest.h"
#include "dbg.h"

#define DBG_TAG	"BDTEST"

#define	ACCOUNT_PW_KEY		0x06282014
#define ACCOUNT_SERVER		"dasangng.co.kr"
#define ACCOUNT_RESOURCE	"raonix.gateway"
#define ACCOUNT_PREFIX		0x03

#define GMAC0_OFFSET	4
#define GMAC1_OFFSET	46

#define ID_OFFSET	(0x10000-512)
#define PW_OFFSET	(0x10000-384)
#define SRV_OFFSET	(0x10000-256)

#define otohl(x)  \
	({ unsigned int __x = (x);                      \
	 ((((__x) & 0xff000000) >> 24) | (((__x) & 0x00ff0000) >>  8) |  \
	  (((__x) & 0x0000ff00) <<  8) | (((__x) & 0x000000ff) << 24)); })    
#define otohs(x) \
	({ unsigned int __x = (x);                      \
	 ((((__x) & 0xff00) >>  8) | (((__x) & 0x00ff) << 8)); })

#define htool(x)  \
	({ unsigned int __x = (x);                      \
	 ((((__x) & 0xff000000) >> 24) | (((__x) & 0x00ff0000) >>  8) |  \
	  (((__x) & 0x0000ff00) <<  8) | (((__x) & 0x000000ff) << 24)); })    
#define htoos(x) \
	({ unsigned int __x = (x);                      \
	 ((((__x) & 0xff00) >>  8) | (((__x) & 0x00ff) << 8)); })

typedef unsigned char   U8;
typedef unsigned short  U16;
typedef unsigned int    U32;
typedef char            S8;
typedef short           S16;
typedef int             S32;

//display
#define DISP(format, args...)  {printf("\n[bdtest] "format, ## args); fflush(stdout); }

//global variable
int _current;
int _result;
int _allresult;
#define OK              (0)
#define FAIL    (1)

#define VER_STR                         "ver1.1"
// use reserved mtd for flash rd/wr test
#define MTD_TEST_NAME 	          "Config"
#define MTD_TEST_SIZE             (0x10000)

extern char *figlet[17][5];

char STRMAC[6][4];
U8 MAC[6];

static int _insert_mac_skip;

int loopback(char *eth);

//display single line with character
void display_line(U8 ch) {
	int i;
	U8 buf[120];

	for(i=0; i<70; i++)
		buf[i] = ch;
	buf[i] = '\0';
	DISP("%s", buf);

	return;
}

//show main title
void show_title(void) {
	DISP("");
	DISP("factory test. %s", VER_STR);
	DISP("");
	DISP("    for DJ310");
	DISP("");
	return;
}

//show current test result
void show_result()
{
	switch (_current)
	{
		case TEST_LAN:
			DISP("LAN TEST ");
			break;

		case INSERT_MAC:
			DISP("INSERT MAC ");
			break;

		case INSERT_ACCOUNT:
			DISP("INSERT ACCOUNT ");
			break;

		case FACTORY_RESET:
			DISP("FACTORY RESET ");
			break;

		default:
			DISP("unknown TEST ");
			break;
	}

	if(_result==FAIL) {
		DISP("    ----------> FAILED.");
		_allresult = FAIL;
	} else if(_result==OK) {
		DISP("    ----------> O K");
	} else {
		DISP("    ----------> not tested.");
	}

	_current = 0;
	_result = OK;
	return;
}

void display_ok_msg(void)
{
	DISP("");
	DISP("###########################");
	DISP("###########################");
	DISP("###                     ###");
	DISP("###         ***         ###");
	DISP("###       *     *       ###");
	DISP("###     *         *     ###");
	DISP("###    *           *    ###");
	DISP("###    *           *    ###");
	DISP("###     *         *     ###");
	DISP("###       *     *       ###");
	DISP("###         ***         ###");
	DISP("###                     ###");
	DISP("###########################");
	DISP("###########################");
	DISP("");
}

void display_fail_msg(void)
{
	DISP("");
	DISP("############################");
	DISP("############################");
	DISP("###                      ###");
	DISP("###    **          **    ###");
	DISP("###      **      **      ###");
	DISP("###        **  **        ###");
	DISP("###          **          ###");
	DISP("###        **  **        ###");
	DISP("###      **      **      ###");
	DISP("###    **          **    ###");
	DISP("###                      ###");
	DISP("############################");
	DISP("############################");
	DISP("");
}

void display_finish_msg(void)
{
	DISP("");
	DISP("###########################");
	DISP("###########################");
	DISP("###                     ###");
	DISP("###        #####        ###");
	DISP("###       *  #  *       ###");
	DISP("###     *    #    *     ###");
	DISP("###    *     #     *    ###");
	DISP("###    *     #     *    ###");
	DISP("###     *    #    *     ###");
	DISP("###       *  #  *       ###");
	DISP("###        #####        ###");
	DISP("###                     ###");
	DISP("###########################");
	DISP("###########################");
	DISP("");

}

void display_account(char *id, char *pw)
{
	unsigned char index_figlet[12];
	char buf[256];
	char ch;
	int slen;
	int i, j;
	char *p;

	slen = strnlen(id, 12);
	for (i=0; i<slen; i++)
	{
		ch = id[i];
		if (ch >= '0' && ch <= '9')
			index_figlet[i] = ch - '0';
		else
		{
			ch = toupper(ch);
			if (ch >= 'A' && ch <= 'Z')
				index_figlet[i] = ch - 'A' + 10;
		}
	}

	memset(buf, 0, sizeof(buf));
	for(i=0; i<5; i++)
	{
		p = buf;
		for(j=0; j<slen; j++) 
		{
			sprintf(p, "%s", figlet[index_figlet[j]][i]);
			p += strlen(figlet[index_figlet[j]][i]);
		}
		DISP("%s", buf);
	}

}

void show_total_result()
{
	DISP("");
	if(_allresult==FAIL) {
		display_fail_msg();
	} else if(_allresult==OK) {
		display_ok_msg();
	}
}

//show ending title
void show_ending(void) {
	display_finish_msg();
	return;
}

U8 get_checksum(U8* buf) {
	S32 i;
	U8      chk;

	chk = 0;
	for(i=0; i<MTD_TEST_SIZE; i++) {
		chk = chk | (chk ^ (*(buf+i)));
	}
	return chk;
}

void test_lan(void)
{
    int	rc;
	int i, errcnt = 0;

    _current = TEST_LAN;

	for (i=0; i<100; i++)
	{
		rc = loopback ("br-lan");
		if( rc < 0)
		{
			errcnt++;
		}
	}

	if (errcnt)
	{
		DISP("");
		DISP("LAN ERROR RATE : %d%%",  errcnt);
		DISP("");

    	_result  = FAIL;
	}
	else
		_result  = OK;

	return;
}

void insert_account(void)
{
	char strid[128], strpw[128], strsrv[128];
	char jid[128];
	unsigned long intpw, intid;
	char ch;
	int fgstart=0;
	int rc = 0;

	memset(strid, 0, sizeof(strid));
	memset(strpw, 0, sizeof(strpw));
	memset(strsrv, 0, sizeof(strsrv));
	memset(jid, 0, sizeof(jid));

	sprintf(strid, "%02x%02x%02x%02x", ACCOUNT_PREFIX, MAC[3], MAC[4], MAC[5]);
	sprintf(jid, "%s@%s/%s", strid, ACCOUNT_SERVER, ACCOUNT_RESOURCE);

	intid = strtol(&strid[1], NULL, 16);
	intpw = intid ^ ACCOUNT_PW_KEY;

	sprintf(strpw, "%08x", intpw);
	sprintf(strsrv, "%s", ACCOUNT_SERVER);

	DISP("");
	display_account(strid, strpw);
	DISP("");

	flash_write ("factory", jid, ID_OFFSET, sizeof(jid));
	flash_write ("factory", strpw, PW_OFFSET, sizeof(strpw));
	flash_write ("factory", strsrv, SRV_OFFSET, sizeof(strsrv));

	char tmpstr[128];
	flash_read  ("factory", tmpstr,  ID_OFFSET, sizeof (tmpstr));
	DISP("ID: %s\n", tmpstr);
	flash_read  ("factory", tmpstr,  PW_OFFSET, sizeof (tmpstr));
	DISP("PW: %s\n", tmpstr);
	flash_read  ("factory", tmpstr,  SRV_OFFSET, sizeof (tmpstr));
	DISP("DOMAIN: %s\n", tmpstr);

	_result = OK;

	_current = INSERT_ACCOUNT;
}

void factory_reset(void)
{
	char cmd[128];

	sprintf(cmd, "jffs2reset -y 2>/dev/null");
	system(cmd);
	sleep(1);

	_current = FACTORY_RESET;
	_result = OK;
}

//
// MAC address config
//
void display_macaddr(char *macaddr)
{
	unsigned char index_figlet[17];
	char buf[256];
	int slen;
	char ch, *p;
	int i, j;

	slen = strlen(macaddr);
	for (i=0; i<slen; i++)
	{
		ch = macaddr[i];
		if (ch == ':')
			index_figlet[i] = 36;
		else if (ch >= '0' && ch <= '9')
			index_figlet[i] = ch - '0';
		else
		{
			ch = toupper(ch);
			if (ch >= 'A' && ch <= 'Z')
				index_figlet[i] = ch - 'A' + 10;
		}
	}

	memset(buf, 0, sizeof(buf));
	for(i=0; i<5; i++)
	{
		p = buf;
		for(j=0; j<slen; j++)
		{
			sprintf(p, "%s", figlet[index_figlet[j]][i]);
			p += strlen(figlet[index_figlet[j]][i]);
		}
		DISP("%s", buf);
	}
}

void convert_macaddr(char *strmac)
{
	char *p;
	U32 idx;

	idx = 0;
	p = strtok(strmac, ":\n");
	while(p!=NULL) {
		sprintf(STRMAC[idx++], "%02x", (unsigned int)strtoul(p, NULL, 16));
		p = strtok(NULL, ":\n");
	}

	MAC[0] = strtol (STRMAC[0], NULL, 16);
	MAC[1] = strtol (STRMAC[1], NULL, 16);
	MAC[2] = strtol (STRMAC[2], NULL, 16);
	MAC[3] = strtol (STRMAC[3], NULL, 16);
	MAC[4] = strtol (STRMAC[4], NULL, 16);
	MAC[5] = strtol (STRMAC[5], NULL, 16);

	// recovery strmac
	sprintf(strmac, "%s:%s:%s:%s:%s:%s", STRMAC[0], STRMAC[1], STRMAC[2],
			STRMAC[3], STRMAC[4], STRMAC[5]);
}

void insert_mac(void)
{
	char ch;
	char *s;
	U8 chmac[12];
	int cnt;
	char buf[64];

	//init
	_current = INSERT_MAC;
	_result = FAIL;

restart:
	//insert MAC address
	cnt = 0;
	memset(&chmac[0], 0, sizeof(chmac));

	DISP("");
	DISP("Insert MAC address ");
	DISP("or Insert ENTER key to skip");
	DISP("");
	DISP("Insert MAC address or Enter => ");
	ch = 0;

	do {
		ch = getchar();
		if (ch == '\n' || ch == '\r') {
			DISP("MAC address was skipped.");
			_insert_mac_skip = 1;
			goto finish;
		}
		chmac[cnt] = ch;
		cnt++;
		if(cnt==12) {
			sprintf(&buf[0], "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
				chmac[0], chmac[1], chmac[2], chmac[3], chmac[4], chmac[5],
				chmac[6], chmac[7], chmac[8], chmac[9], chmac[10], chmac[11]);
			break;
		}
	} while(1);

	_insert_mac_skip = 0;

	convert_macaddr(buf);

	DISP("");
	display_macaddr(buf);
	DISP("");

	// Write chmac to flash
	flash_write ("factory", MAC, GMAC0_OFFSET, sizeof(MAC));
	flash_write ("factory", MAC, GMAC1_OFFSET, sizeof(MAC));

finish:

	_result = OK;

	return;
}

void sig_handler(int signo)
{
	exit(0);
}

//main Fn.
int main(void)
{
	char cmd[128];

	signal(SIGINT, (void *)sig_handler);

	_result = -1;
	_allresult = 0;

	display_line('=');
	show_title();

	/* perform test */
	// test_lan ();			show_result ();
	// display_line('=');

	// MAC Address
	insert_mac();			show_result();

	// XMPP Account
	if (!_insert_mac_skip)
	{
		insert_account();	show_result();
	}

	// factory reset
	display_line('=');
	factory_reset();		show_result();

	display_line('=');
	show_total_result();

	// display result to led
	if(_allresult==FAIL)
	{
		// TODO : 
	}
	else if(_allresult==OK)
	{
		// TODO : 
	}
	// show_ending();

	printf("End bdtest"); // for Teraterm Macro

	return 0;
}
