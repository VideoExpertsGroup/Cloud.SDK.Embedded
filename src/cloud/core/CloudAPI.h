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

#ifndef __CLOUDAPI_H__
#define __CLOUDAPI_H__

#include "../../utils/utils.h"
#include "../../utils/_cunk.h"
#include "../../utils/MLog.h"
#include "CloudCommon/CloudRegToken.h"
#include "CloudCommon/CloudToken.h"
#include "CloudAPIEndPoints.h"
#include <list>
#include <utility>
#include "CloudHelpers.h"

using namespace std;

class CloudAPI : public CUnk {
//	const char *TAG = "CloudAPI";
//	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	std::string mProtocol;
	std::string mHost;
	std::string mPrefixPath;
	CloudToken mToken;
	std::string mShareToken;
	static const int readTimeout = 10000;
	static const int connectTimeout = 2000;

	std::string mCMAddress;

	std::string makeURL(std::string endPoint) {
		std::string url = mProtocol + "://" + mHost + mPrefixPath + endPoint;
		return url;
	}

public:
    CloudAPI() 
		:Log("CloudAPI", 2) 
	{


		mProtocol = "http";
		mHost = "";
		mPrefixPath = "";
		mShareToken = "";
		mCMAddress = "";

		Log.d("=>CloudAPI this=%p", this);
	}
	virtual ~CloudAPI() {
		Log.d("<=CloudAPI this=%p", this);
	}

    void setHost(std::string host){
        mHost = host;
    }

	std::string& getHost(){
        return mHost;
    }

    void setPrefixPath(std::string &prefixPath){
        mPrefixPath = prefixPath;
    }

	std::string& getPrefixPath(){
        return mPrefixPath;
    }

    void setProtocol(std::string &protocol){
        mProtocol = protocol;
    }

	std::string& getProtocol(){
        return mProtocol;
    }

    void setShareToken(std::string &share_token){
        mShareToken = share_token;
    }

    void setCMAddress(std::string &url) {
        mCMAddress = url;
    }
	std::string& getCMAddress(){
        return mCMAddress;
    }

    bool usedShareToken(){
        return mShareToken.length() > 0;
    }

    CloudRegToken resetCameraManager(long long cmngrsID){
        CloudRegToken regToken;
        if(mToken.isEmpty() && !usedShareToken()){
            return regToken;
        }

		pair<int, string> p = executePostRequest2(CloudAPIEndPoints::CMNGRS_RESET(cmngrsID), "");
		if(p.first != 200)
			return regToken;

		CloudRegToken regTokenOk(p.second);
        return regTokenOk;
    }

    /*
    ****************************************************************
    ***************************************************************
    * CAMERAS
    ***************************************************************
    */

	pair<int, string> getCamera(long long id){
		list< pair<string, string> > params;
		return executeGetRequest2(CloudAPIEndPoints::CAMERA(id), params);
    }

private:
    pair<int, string > executeGetRequest2(const string &endPoint, list< pair<string, string> > &params) {

        if (usedShareToken()){
            params.push_back( pair<string, string >("token", mShareToken));
        }

        string url = makeURL(endPoint + "?" + CloudHelpers::prepareHttpGetQuery(params));

        // HttpsURLConnection urlConnection = (HttpsURLConnection) url.openConnection();
        Log.d("GET %s", url.c_str());
		return pair<int, string>(402, "Not implemented");

        /*HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
        urlConnection.setReadTimeout(readTimeout);
        urlConnection.setConnectTimeout(connectTimeout);
        urlConnection.setRequestMethod("GET");

        if (!mToken.isEmpty()) {
            urlConnection.setRequestProperty("Authorization", "SkyVR " + mToken.getToken());
        }

        StringBuffer buffer = new StringBuffer();
        int code_response = urlConnection.getResponseCode();
        Log.d("GET ResponseCode: " + code_response + " for URL: " + url);

        boolean isError = code_response >= 400;
        InputStream inputStream = isError ? urlConnection.getErrorStream() : urlConnection.getInputStream();
        BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
        String line;
        while ((line = reader.readLine()) != null) {
            buffer.append(line);
        }
        return new Pair<>(code_response, buffer.toString());*/
    }

    pair<int, string> executePostRequest2(string endPoint,  string json_data) {
        list< pair< string, string> > params;
		if (usedShareToken()) {
			params.push_back(pair<string, string >("token", mShareToken));
		}

        string url = makeURL(endPoint + (params.size() ? "?" + CloudHelpers::prepareHttpGetQuery(params):""));
        // HttpsURLConnection urlConnection = (HttpsURLConnection) url.openConnection();
        Log.d("POST %s", url.c_str());

		return pair<int, string>(402, "Not implemented");

        /*HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
        urlConnection.setRequestMethod("POST");
        urlConnection.setReadTimeout(readTimeout);
        urlConnection.setConnectTimeout(connectTimeout);
        Log.i("readTimeout " + readTimeout);
        urlConnection.setDoInput(true);
        urlConnection.setUseCaches(false);

        if (!mToken.isEmpty()) {
            urlConnection.setRequestProperty("Authorization", "SkyVR " + mToken.getToken());
        }

        if(data != null) {
            urlConnection.setRequestProperty("Content-type", "application/json");
            OutputStreamWriter wr = new OutputStreamWriter(urlConnection.getOutputStream());
            wr.write(data.toString());
            wr.flush();
            wr.close();
        }

        StringBuffer buffer = new StringBuffer();

        Log.d("POST ResponseCode: " + urlConnection.getResponseCode() + " for URL: " + url);
        if (urlConnection.getResponseCode() == 401) {
            // throw new IOException(HTTP_EXCEPTION_INVALID_AUTH);
            // return
        }

        int code_response = urlConnection.getResponseCode();
        boolean isError = code_response >= 400;
        InputStream inputStream = isError ? urlConnection.getErrorStream() : urlConnection.getInputStream();
        BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
        String line;
        while ((line = reader.readLine()) != null) {
            buffer.append(line);
        }
        return new Pair<>(code_response, buffer.toString());*/
    }

    pair<int, string> executePutRequest2(string endPoint,  string json_data) {
		list< pair< string, string> > params;
		if (usedShareToken()) {
			params.push_back(pair<string, string >("token", mShareToken));
		}

		string url = makeURL(endPoint + (params.size() ? "?" + CloudHelpers::prepareHttpGetQuery(params) : ""));
        // HttpsURLConnection urlConnection = (HttpsURLConnection) url.openConnection();
        Log.d("PUT %s", url.c_str());

		return pair<int, string>(402, "Not implemented");

        /*HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
        urlConnection.setRequestMethod("PUT");
        urlConnection.setReadTimeout(readTimeout);
        urlConnection.setConnectTimeout(connectTimeout);
        Log.i("readTimeout " + readTimeout);
        urlConnection.setDoInput(true);
        urlConnection.setUseCaches(false);

        if (!mToken.isEmpty()) {
            urlConnection.setRequestProperty("Authorization", "SkyVR " + mToken.getToken());
        }

        if(data != null) {
            urlConnection.setRequestProperty("Content-type", "application/json");
            OutputStreamWriter wr = new OutputStreamWriter(urlConnection.getOutputStream());
            wr.write(data.toString());
            wr.flush();
            wr.close();
        }

        StringBuffer buffer = new StringBuffer();

        Log.d("PUT ResponseCode: " + urlConnection.getResponseCode() + " for URL: " + url);
        if (urlConnection.getResponseCode() == 401) {
            // throw new IOException(HTTP_EXCEPTION_INVALID_AUTH);
            // return
        }

        int code_response = urlConnection.getResponseCode();
        boolean isError = code_response >= 400;
        InputStream inputStream = isError ? urlConnection.getErrorStream() : urlConnection.getInputStream();
        BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
        String line;
        while ((line = reader.readLine()) != null) {
            buffer.append(line);
        }
        return new Pair<>(code_response, buffer.toString());*/
    }

};

#endif //__CLOUDAPI_H__