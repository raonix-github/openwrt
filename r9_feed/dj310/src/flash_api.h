#ifndef __FLASH_API
#define __FLASH_API

// 2014.06.29 bygomma : libnvram
// int flash_read(char *buf, off_t from, size_t len);
// int flash_write(char *buf, off_t to, size_t len);
int flash_read(char *mtd, char *buf, off_t from, size_t len);
int flash_write(char *mtd, char *buf, off_t to, size_t len);
// 2014.06.29 bygomma : end

#endif
