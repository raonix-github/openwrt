#ifndef _DBG_H_
#define _DBG_H_

#if defined(DEBUG)
#define _dbg(tag, fmt, args...) \
	do { printf("[%s %s:%d:%s] " fmt, tag, __FILE__, \
		__LINE__, __func__, ## args); fflush(stdout);} while (0)
#define _trc()		_dbg("trc", "%s\n", "")
#else
#define _dbg(tag, fmt, args...)		{}
#define _trc()						{}
#endif

#endif
