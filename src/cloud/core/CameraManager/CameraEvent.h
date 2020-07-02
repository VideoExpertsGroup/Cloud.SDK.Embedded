
#ifndef __CAMERA_EVENT_H__
#define __CAMERA_EVENT_H__

#include "../../../utils/utils.h"

struct CameraEvent
{
	std::string event; //"motion", "sound", "record", etc
	unsigned long timeUTC;
	std::string status;
	int data_size;
	int snapshot_width;
	int snapshot_height;
	std::string meta;

	void set_time(long ltime)
	{
		time_t rawtime = ltime;
		struct tm * timeinfo;
		char buffer[80];
		buffer[0] = 0;

		if(rawtime == 0)
			time(&rawtime);
		timeinfo = gmtime(&rawtime);

		strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeinfo);
		
		//Log.e("set_time  error camEvent.event.empty()",buffer);

		timeUTC = time(NULL);
	}


		void set_time1(const char* date_time)
		{		
				/*
				timeUTC = time(NULL);
				(void)date_time;
				*/
				
				
				// (2018-12-10T13:07:29+07:00) - Format from Hikvision
				printf("set_time1 (%s) \n",date_time);
				
			   struct tm ti={0};
		      if(sscanf(date_time, "%d-%d-%dT%d:%d:%d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday ,&ti.tm_hour , &ti.tm_min, &ti.tm_sec ) != 6)
		      {
		      	timeUTC = time(NULL);
					return ;
		      }


			 ti.tm_year-=1900;
	   	 ti.tm_mon-=1;
	   	 
	   	 timeUTC =  mktime(&ti);		

			 printf("<=set_time1 (%lu)\n",timeUTC);
			 

		    
		}

		void set_time2(unsigned long time_utc)
		{	
			if (time_utc)
				timeUTC = time_utc;
			else
				timeUTC = time(NULL);
		}
	 
	
};

#endif //__CAMERA_EVENT_H__
