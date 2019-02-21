
#ifndef __CLOUDHELPERS_H__
#define __CLOUDHELPERS_H__

#include "../../utils/utils.h"
#include <list>
#include <utility>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

namespace CloudHelpers
{
	static string url_encode(const string &value) {
		ostringstream escaped;
		escaped.fill('0');
		escaped << hex;

		for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
			string::value_type c = (*i);

			// Keep alphanumeric and other accepted characters intact
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
				escaped << c;
				continue;
			}

			// Any other characters are percent-encoded
			escaped << uppercase;
			escaped << '%' << setw(2) << int((unsigned char)c);
			escaped << nouppercase;
		}

		return escaped.str();
	}

	static string prepareHttpGetQuery( list< pair<string, string> > &params) 
	{
		string sQuery = "";
		int i = 0;
		//ArrayList<String> params2 = new ArrayList<>();
		
		list< pair<string, string> >::iterator it;
		for (it = params.begin(); it != params.end(); it++) {
			pair<string, string> &item = *it;

			string sKey = url_encode(item.first);
			string sValue = url_encode(item.second);

			if (sKey.length() > 0) {
				if (i != 0) {
					sQuery += "&";
				}
				sQuery += sKey + "=" + sValue;
				i++;
			}
		}
		return sQuery;
	}

};

#endif