
#ifndef __CAMERAMANAGERDONESTATUS_H__
#define __CAMERAMANAGERDONESTATUS_H__

namespace CameraManagerDoneStatus {
	const char *OK = "OK"; // Success is “OK”. Predefined values are:
	const char *CM_ERROR = "ERROR"; // general error
	const char *SYSTEM_ERROR = "SYSTEM_ERROR"; // system failure
	const char *NOT_SUPPORTED = "NOT_SUPPORTED"; // functionality is not supported
	const char *INVALID_PARAM = "INVALID_PARAM"; // some parameter in packet is invalid
	const char *MISSED_PARAM = "MISSED_PARAM"; // mandatory parameter is missed in the packet
	const char *TOO_MANY = "TOO_MANY"; // list contains too many elements
	const char *RETRY = "RETRY"; // peer is busy, retry later
}

#endif //__CAMERAMANAGERDONESTATUS_H__
