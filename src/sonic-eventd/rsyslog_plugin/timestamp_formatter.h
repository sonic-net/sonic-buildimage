#ifndef TIMESTAMP_FORMATTER_H
#define TIMESTAMP_FORMATTER_H

#include <iostream>
#include <string>
#include <regex>
#include <ctime>
#include <vector>

using namespace std;

/***
 *
 * TimestampFormatter is responsible for formatting the timestamps received in syslog messages and to format them into the type needed by YANG model
 *
 */

class TimestampFormatter {
public:
    string changeTimestampFormat(vector<string> dateComponents);
    string m_storedTimestamp;
    string m_storedYear;
private:
    string getYear(string timestamp);
};

#endif
