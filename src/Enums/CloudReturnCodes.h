
#ifndef __CLOUDRETURNCODES_H__
#define __CLOUDRETURNCODES_H__

/**
 * Created by bleikher on 27.07.2017.
 */

enum CloudReturnCodes {
	RET_OK = 0,                          /* Success */
	RET_OK_COMPLETIONPENDING = 1,        /* Operation Pending */
	RET_ERROR_NOT_CONFIGURED = -2,       /* Object not configured */
	RET_ERROR_NOT_IMPLEMENTED = -1,      /* Function Not implemented*/
	RET_ERROR_NO_MEMORY = -12,           /* Out of memory */
	RET_ERROR_ACCESS_DENIED = -13,       /* Access denied */
	RET_ERROR_BADARGUMENT = -22,         /* Invalid argument */
	RET_ERROR_STREAM_UNREACHABLE = (-5049),
	RET_ERROR_EXPECTED_FILTER = (-5050),
	RET_ERROR_NO_CLOUD_CONNECTION = (-5051),
	RET_ERROR_WRONG_RESPONSE = (-5052),
	RET_ERROR_NOT_AUTHORIZED = (-5401),
	RET_ERROR_SOURCE_NOT_CONFIGURED = (-5053),
	RET_ERROR_INVALID_SOURCE = (-5054),
	RET_ERROR_RECORDS_NOT_FOUND = (-5055),
	RET_ERROR_FORBIDDEN = (-5403),
	RET_ERROR_NOT_FOUND = (-5404),
	RET_ERROR_TOO_MANY_REQUESTS = (-5429)
};

inline int reason_str_to_int(std::string reason)
{
	if (reason.empty())
		return RET_ERROR_NO_CLOUD_CONNECTION;
		
	if (!reason.compare("SYSTEM_ERROR")) {
		return RET_ERROR_INVALID_SOURCE;
	}
	if (!reason.compare("INVALID_USER")) {
		return RET_ERROR_NOT_AUTHORIZED;
	}
	if (!reason.compare("AUTH_FAILURE")) {
		return RET_ERROR_ACCESS_DENIED;
	}
	if (!reason.compare("CONN_CONFLICT")) {
		return RET_ERROR_TOO_MANY_REQUESTS;
	}
	if (!reason.compare("RECONNECT")) {
		return RET_ERROR_NO_CLOUD_CONNECTION;
	}
	if (!reason.compare("SHUTDOWN")) {
		return RET_ERROR_NO_CLOUD_CONNECTION;
	}
	if (!reason.compare("DELETED")) {
		return RET_ERROR_STREAM_UNREACHABLE;
	}

	return RET_ERROR_NO_CLOUD_CONNECTION;
}

#endif //__CLOUDRETURNCODES_H__