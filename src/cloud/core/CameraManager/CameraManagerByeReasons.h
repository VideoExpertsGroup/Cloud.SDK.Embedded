
#ifndef __CAMERAMANAGERBYEREASONS_H__
#define __CAMERAMANAGERBYEREASONS_H__

namespace CameraManagerByeReasons{
	const char *BYE_ERROR = "ERROR"; //general application error
	const char *SYSTEM_ERROR = "SYSTEM_ERROR"; // – system failure on peer
	const char *INVALID_USER = "INVALID_USER"; //  user not found
	const char *AUTH_FAILURE = "AUTH_FAILURE"; // authentication failure
	const char *CONN_CONFLICT = "CONN_CONFLICT"; //there is another alive connection from the CM
	const char *RECONNECT = "RECONNECT"; // no error but reconnection is required
	const char *SHUTDOWN = "SHUTDOWN"; //CM shutdown or reboot is requested

			  // CM has been deleted from account it belonged to. CM must stop all
			  // attempts to connect to server and forget all related data:
			  // account (user), password, server address and port.
	const char *DELETED = "DELETED";
}

#endif //__CAMERAMANAGERBYEREASONS_H__
