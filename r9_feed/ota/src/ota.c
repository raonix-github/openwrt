#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <uci.h>
#include <openssl/md5.h>
#include <jansson.h>

#include "dbg.h"

#define	POLL_INTERVAL		5

#define	CMD_SZ	256
#define	OTA_INFO_FILE			"/tmp/ota.json"
#define	OTA_BIN_FILE			"/tmp/ota.bin"

#define	TAG_DESC				"description"
#define	TAG_HWVER				"sys_hwversion"
#define	TAG_SWVER				"sys_swversion"
#define	TAG_CODE				"sys_build_code"
#define	TAG_URL					"url"
#define	TAG_MD5					"md5"

#define BC_SYS_UPGRADE			"sys_upgrade"
// ram-nvram
#define BC_SYS_SW_VERSION		"SYS_SW_VERSION"
#define BC_SYS_BUILD_CODE		"SYS_BUILD_CODE"

#define BC_MTD_WRITE_PROGRESS	"MTD_WRITE_PROGRESS"
#define BC_MTD_WRITE_STATE		"MTD_WRITE_STATE"
#define BC_OTA_STATE			"OTA_STATE"
#define BC_OTA_PROGRESS			"OTA_PROGRESS"

#define DBG_TAG		"OTA"

enum 
{
	MTD_WRITE_WRITTING,
	MTD_WRITE_SUCCESS,
	MTD_WRITE_FAIL
};

FILE *logfp=NULL;

char usage[] = 	"ota url\n";

int getmd5(char *filename, char *md5str)
{
	MD5_CTX lctx;
	unsigned int i;
	int fd;
	struct stat status;
	char *data;
	unsigned char digest[16];

	fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		return -1;
	}

	fstat(fd, &status);
	data = (char *)malloc(status.st_size);
	read(fd, data, status.st_size);

	MD5_Init(&lctx);
	MD5_Update(&lctx, data, status.st_size);
	MD5_Final(digest, &lctx);

	for (i = 0; i < sizeof(digest); ++i)
	{
		sprintf(md5str+(i*2), "%02x", digest[i]);
	}
	free(data);
	close(fd);
	return 0;
}

enum
{
	IDLE=0,
	DOWNLOADINFO,
	DOWNLOADBIN,
	CHECK,
	WRITTING,
	SUCCESS,
	FAIL,
	LATEST,
	MAX_STATE
};

char *log_state[8] =
{
	"idle",
	"downloadinfo",
	"downloadbin",
	"check",
	"writting",
	"success",
	"fail",
	"latest"
};

static void _update_state(char *logst, int progress)
{
	char buf[5];

	sprintf(buf, "%d", progress);
//	bc_set_ram(BC_OTA_STATE, logst);
//	bc_set_ram(BC_OTA_PROGRESS, buf);
}

static void _get_mtd_state (int *mtdstate, int *progress)
{
	*mtdstate = -1;
	*progress = 0;
	char *strval;

//	strval =bc_get_ram (BC_MTD_WRITE_STATE);

	_dbg (DBG_TAG, "strval : %s\n", strval);

	if(!strncmp(strval, "success", 7))
		*mtdstate = 0;
	else if(!strncmp(strval, "fail", 4))
		*mtdstate = 1;
	else if(!strncmp(strval, "writting", 8))
		*mtdstate = 2;
	else
		*mtdstate = -1;

//	strval =bc_get_ram (BC_MTD_WRITE_PROGRESS);
	*progress = atoi(strval);

#if 0
	FILE *pf=NULL;

#define	PARSE_SZ	256
	char parse[PARSE_SZ];
	char *tag, *strval;
	int retry = 0;

	*mtdstate = -1;
	*progress = 0;

	pf = fopen( "/tmp/mtd_write.log", "r" );
	if(pf == NULL)
	{
		return;
	}

	while( !feof( pf ) )
	{
		memset( parse, 0x00, PARSE_SZ );
		fgets( parse, PARSE_SZ, pf );
		if( strncmp( parse, "mtdstate", 5) == 0 )
		{
			strval = strtok(&parse[6], "\t");
			if(!strncmp(strval, "success", 7))
				*mtdstate = 0;
			else if(!strncmp(strval, "fail", 4))
				*mtdstate = 1;
			else if(!strncmp(strval, "writting", 7))
				*mtdstate = 2;
			else
				*mtdstate = -1;
		}
		else if( strncmp( parse, "progress", 5) == 0 )
		{
			strval = strtok(&parse[9], "\t");
			*progress = atoi(strval);
		}
	}
	fclose( pf );
#endif
}

void start_mtd_write(void)
{
	char cmd[CMD_SZ];

	memset(cmd, 0x00, CMD_SZ);
//	sprintf(cmd, "killall -9 mtd_write; mtd_write -q -o 0 write %s Kernel &", OTA_BIN_FILE);
	sprintf(cmd, "killall -9 sysupgrade; sysupgrade -v %s&", OTA_BIN_FILE);
	system(cmd);
}

int check_process(char *procname)
{
	FILE *fd;
	char buffer[2048];
	int rc;

	rc = 0;
	strcpy(buffer, "ps");
	fd = popen(buffer, "r");
	while (fgets(buffer, sizeof(buffer), fd))
	{
		if (strstr(buffer, procname))
		{
			rc = 1;
			break;
		}
	}
	pclose(fd);

	return rc;
}

int get_buildcode (char *path)
{
	struct uci_context *c = NULL;
	struct uci_ptr ptr;
	int buildcode = 0;
	int rc;

	c = uci_alloc_context();

	if ((uci_lookup_ptr(c, &ptr, path, true) != UCI_OK)
			|| (ptr.o==NULL || ptr.o->v.string==NULL)) {
		uci_free_context(c);
		return -1;
	}

	buildcode = strtol (ptr.o->v.string, NULL, 10);

	uci_free_context (c);

	return buildcode;
}

int main(int argc, char *argv[])
{
	json_t *json;
	json_t *obj= NULL;
	json_error_t error;
	const char *desc, *hwver, *swver, *url, *md5;
	int newcode, curcode;
	char *bcval;

	desc = NULL;
	hwver = NULL;
	swver = NULL;
	url = NULL;
	md5 = NULL;

//	char infourl[256] ={0x00,};
    char buf[CMD_SZ];
	int rc;

	if (argc != 2) 
	{
		printf("Usage:\n%s\n", usage);
		return 1;
	}

	char path[] = "system.@system[0].firmware_buildcode";
	curcode = get_buildcode (path);
	_dbg (DBG_TAG, "curcode: %d\n", curcode);

	// download OTA information
	_update_state(log_state[DOWNLOADINFO], 0);

	memset(buf, 0x00, CMD_SZ);
	
	sprintf(buf, "rm %s %s",  OTA_INFO_FILE, OTA_BIN_FILE);
	system(buf);
	sprintf(buf, "wget %s -O %s", argv[1], OTA_INFO_FILE);
	system(buf);

	json = json_load_file(OTA_INFO_FILE, 0, &error);
	if(!json)
	{
		fprintf(stderr, "file not found upgrade info\n");
		rc = -1;
		goto ret;
	}

	obj = json_object_get(json, TAG_CODE);
	if(obj) newcode = json_integer_value(obj);

	if(newcode <= curcode)
	{
		rc = 1;
		goto ret;
	}

	obj = json_object_get(json, TAG_DESC);
	if(obj) desc = json_string_value(obj);

	obj = json_object_get(json, TAG_HWVER);
	if(obj) hwver = json_string_value(obj);

	obj = json_object_get(json, TAG_SWVER);
	if(obj) swver = json_string_value(obj);

	obj = json_object_get(json, TAG_URL);
	if(obj) url = json_string_value(obj);

	obj = json_object_get(json, TAG_MD5);
	if(obj) md5 = json_string_value(obj);

#if 0
   	_dbg (DBG_TAG, "%s: %s\n", TAG_DESC, desc);
   	_dbg (DBG_TAG, "%s: %s\n", TAG_HWVER, hwver);
   	_dbg (DBG_TAG, "%s: %s\n", TAG_HWVER, swver);
   	_dbg (DBG_TAG, "%s: %s\n", TAG_URL, url);
   	_dbg (DBG_TAG, "%s: %s\n", TAG_MD5, md5);
#endif

	// download OTA binary file
	_update_state(log_state[DOWNLOADBIN], 0);

	memset(buf, 0x00, CMD_SZ);
	sprintf(buf, "wget %s -O %s", url, OTA_BIN_FILE );
	system(buf);

	// check md5
	_update_state(log_state[CHECK], 0);

	char calmd5[256] = {0x00,};
	if(getmd5(OTA_BIN_FILE, calmd5) < 0)
	{
		fprintf(stderr, "fail to get S/W binary\n");
		rc = -1;
		goto ret;
	}

	if(strcmp(calmd5, md5)) 
	{
		fprintf(stderr, "MD5 is mismatch\n");
		rc = -1;
		goto ret;
	}

	// Write binary file to flash
	_update_state(log_state[WRITTING], 0);

	start_mtd_write();
	sleep(POLL_INTERVAL);

#if 0
	int mtdstate, curprog=0, oldprog=0;
	int retry=0, mtdretry=0;

	// writting on mtd with mtd_write cmd
	while(1)
	{
		_get_mtd_state (&mtdstate, &curprog);

		_dbg (DBG_TAG, "mtdstate : %d\n", mtdstate);
		switch(mtdstate)
		{
			case 0:		// mtd_write success
				memset(buf, 0x00, CMD_SZ);
				sprintf(buf, "%d", newcode);
//    			bc_set_ram (BC_SYS_SW_VERSION, (unsigned char *) swver);
//    			bc_set_ram (BC_SYS_BUILD_CODE, (unsigned char *) buf);
//				bc_set_rom (BC_SYS_UPGRADE, "1");
//				bc_commit_rom();
				rc = 0;
				goto ret;
			case 1:		// mtd_write fail
				fprintf(stderr,"fail to write binary code code %d\n", newcode);
				if(mtdretry++ < 3) 
				{
					_dbg (DBG_TAG, "mtdretry : %d\n", mtdretry);
					start_mtd_write();
				}
				else
				{
					rc  = -1;
					goto ret;
				}
				break;
			case 2:		// mtd_write writting
				if(oldprog == curprog)
				{
					if(retry++ > 3)
					{
						if(mtdretry++ < 3) 
						{
							start_mtd_write();
						}
						else
						{
							rc  = -1;
							goto ret;
						}
					}
				}
				else
				{
					retry == 0;
					oldprog = curprog;
					_update_state(log_state[WRITTING], curprog);
				}
				break;
			default:
				if(!check_process("sysupgrade"))
				{
					if(retry++ > 3)
					{
						if(mtdretry++ < 3) 
						{
							start_mtd_write();
							retry = 0;
						}
						else
						{
							rc  = -1;
							goto ret;
						}
					}
				}
				break;
		}
		sleep(POLL_INTERVAL);
	}
#endif

ret:
	if(logfp) fclose(logfp);

	if(rc < 0)
	{
		_dbg (DBG_TAG, "fail to update build code %d\n", newcode);
		_update_state(log_state[FAIL], 0);
	}
	else if(rc == 0)
	{
		_dbg (DBG_TAG, "success to write build-code %d\n", newcode);
		_update_state(log_state[SUCCESS], 100);
	}
	else if(rc == 1)
	{
		_dbg (DBG_TAG, "S/W is latest version\n");
		_update_state(log_state[LATEST], 100);
	}

	return rc;
}
