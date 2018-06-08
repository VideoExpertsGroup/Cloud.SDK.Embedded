
#ifndef __CAMERAMANAGERCOMMANDS_H__
#define __CAMERAMANAGERCOMMANDS_H__

namespace CameraManagerCommands {
	const char * HELLO = "hello";
	const char * CAM_UPDATE_PREVIEW = "cam_update_preview";
	const char * DONE = "done";
	const char * REGISTER = "register";
	const char * CONFIGURE = "configure";
	const char * BYE = "bye";
	const char * CAM_REGISTER = "cam_register";
	const char * CAM_HELLO = "cam_hello";
	const char * GET_CAM_STATUS = "get_cam_status";
	const char * CAM_STATUS = "cam_status";
	const char * GET_CAM_VIDEO_CONF = "get_cam_video_conf";
	const char * CAM_VIDEO_CONF = "cam_video_conf";
	const char * GET_CAM_AUDIO_CONF = "get_cam_audio_conf";
	const char * CAM_AUDIO_CONF = "cam_audio_conf";
	const char * GET_SUPPORTED_STREAMS = "get_supported_streams";
	const char * SUPPORTED_STREAMS = "supported_streams";
	const char * GET_CAM_EVENTS = "get_cam_events";
	const char * CAM_EVENTS_CONF = "cam_events_conf";
	const char * SET_CAM_EVENTS = "set_cam_events";
	const char * GET_STREAM_BY_EVENT = "get_stream_by_event";
	const char * STREAM_BY_EVENT_CONF = "stream_by_event_conf";
	const char * SET_CAM_VIDEO_CONF = "set_cam_video_conf";
	const char * SET_CAM_AUDIO_CONF = "set_cam_audio_conf";
	const char * STREAM_START = "stream_start";
	const char * STREAM_STOP = "stream_stop";
	const char * GET_STREAM_CAPS = "get_stream_caps";
	const char * STREAM_CAPS = "stream_caps";
	const char * GET_STREAM_CONFIG = "get_stream_config";
	const char * SET_STREAM_CONFIG = "set_stream_config";
	const char * STREAM_CONFIG = "stream_config";
	const char * CAM_EVENT = "cam_event";
	const char * GET_MOTION_DETECTION = "get_motion_detection";
	const char * MOTION_DETECTION_CONF = "motion_detection_conf";
	const char * SET_MOTION_DETECTION = "set_motion_detection";
	const char * GET_AUDIO_DETECTION = "get_audio_detection";
	const char * SET_AUDIO_DETECTION = "set_audio_detection";
	const char * GET_PTZ_CONF = "get_ptz_conf";

	//=>upload segments
	const char * GET_DIRECT_UPLOAD_URL = "get_direct_upload_url"; // CM =>
	const char * DIRECT_UPLOAD_URL = "direct_upload_url"; // <=SRV
	const char * CONFIRM_DIRECT_UPLOAD = "confirm_direct_upload"; // CM =>
																				//<=upload segments

}
#endif //__CAMERAMANAGERCOMMANDS_H__