//
//  Copyright © 2016 VXG Inc. All rights reserved.
//  Contact: https://www.videoexpertsgroup.com/contact-vxg/
//  This file is part of the demonstration of the VXG Cloud Platform.
//
//  Commercial License Usage
//  Licensees holding valid commercial VXG licenses may use this file in
//  accordance with the commercial license agreement provided with the
//  Software or, alternatively, in accordance with the terms contained in
//  a written agreement between you and VXG Inc. For further information
//  use the contact form at https://www.videoexpertsgroup.com/contact-vxg/
//

#ifndef __CLOUDAPIENDPOINTS_H__
#define __CLOUDAPIENDPOINTS_H__

#include "../../utils/utils.h"

namespace CloudAPIEndPoints
{
	static const std::string mCameras = "/api/v2/cameras/";
	static const std::string mSvcAuth = "/svcauth/";
	static const std::string mServerTime = "/api/v2/server/time/";
	static const std::string mCameraManagers = "/api/v2/cmngrs/";
	static const std::string mCameraSession = "/api/v2/camsess/";
	static const std::string mAccount = "/api/v2/account/";

	//STORAGE (CAMERA_RECORDS)
	static const std::string mStorage = "/api/v2/storage/";


	static const std::string LIVE_URLS = "/live_urls/";


	// Account end endpoints

	static const std::string ACCOUNT() {
		return mAccount;
	}

	static const std::string ACCOUNT_LOGOUT() {
		return mAccount + "logout/";
	}

	static const std::string ACCOUNT_API_TOKEN() {
		return mAccount + "token/api/";
	}

	static const std::string ACCOUNT_CAPABILITIES() {
		return mAccount + "capabilities/";
	}

	// Some server endpoints

	static const std::string SERVER_TIME() {
		return mServerTime;
	}

	static const std::string SVC_AUTH() { // TODO
		return mSvcAuth;
	}

	// Cameras endpoints

	static const std::string CAMERA_PREVIEW(long long cameraID) {
		return mCameras + fto_string(cameraID) + "/preview/";
	}

	static const std::string CAMERA_PREVIEW_UPDATE(long long cameraID) {
		return mCameras + fto_string(cameraID) + "/preview/update/";
	}

	static const std::string CAMERAS() {
		return mCameras;
	}

	static const std::string CAMERA(long long cameraID) {
		return mCameras + fto_string(cameraID) + "/";
	}

	static const std::string CAMERA_SHARING(long long cameraID) {
		return mCameras + fto_string(cameraID) + "/sharings/";
	}

	static const std::string CAMERA_RECORDS() {
		return mStorage + "data/";
	}
	static const std::string CAMERA_THUMBNAILS() {
		return mStorage + "thumbnails/";
	}

	static const std::string CAMERA_TIMELINE(long long cameraID) {
		return mStorage + "timeline/" + fto_string(cameraID) + "/";
	}

	static const std::string CAMERA_TIMELINE_DAYS() {
		return mStorage + "activity/";
	}

	static const std::string CAMERA_LIVE_URLS(long long cameraID) {
		return mCameras + fto_string(cameraID) + "/live_urls/";
	}

	// Camera manager endpoints
	static const std::string CMNGRS_RESET(long long cmngrsID) {
		return mCameraManagers + fto_string(cmngrsID) + "/reset/";
	}
	static const std::string CMNGRS(long long cmngrsID) {
		return mCameraManagers + fto_string(cmngrsID) + "/";
	}

	// Camera Session endpoints

	static const std::string CAMSESS_RECORDS(long long camsessID) {
		return mCameraSession + fto_string(camsessID) + "/records/";
	}

	static const std::string CAMSESS() {
		return mCameraSession;
	}

	static const std::string CAMSESS(long long camsessID) {
		return mCameraSession + fto_string(camsessID) + "/";
	}

	static const std::string CAMESESS_RECORDS_UPLOAD(long long camsessID) {
		return mCameraSession + fto_string(camsessID) + "/records/upload/";
	}

	static const std::string CAMESESS_PREVIEW_UPLOAD(long long camsessID) {
		return mCameraSession + fto_string(camsessID) + "/preview/upload/";
	}

	static const std::string CAMSESS_LIVE_STATS(long long camsessID) {
		return mCameraSession + fto_string(camsessID) + "/live_stats/";
	}

	static const std::string CAMSESS_CHAT_SEND_MESSAGE(long long camsessID) {
		return mCameraSession + fto_string(camsessID) + "/chat/send_message/";
	}

	static const std::string CAMSESS_PREVIEW_UPDATE(long long camsessID) { 
		return mCameraSession + fto_string(camsessID) + "/preview/update/";
	}

};
#endif //__CLOUDAPIENDPOINTS_H__