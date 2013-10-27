#include "lib_net.h"
#include "lib_log.h"

int lib_socket(int family, int type, int protocol)
{
	int val = socket(family, type, protocol);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "socket(%d, %d, %d) call failed. error[%d] info is %s.", family, type, protocol, errno, strerror(errno));
	}

	return val;
}
