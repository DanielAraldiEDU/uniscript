#ifndef ANALYSIS_ERROR_H
#define ANALYSIS_ERROR_H

#include <string>

class AnalysisError
{
public:

    AnalysisError(const std::string &msg, int position = -1, int length = 1)
      : message(msg), position(position), length(length) { }

    const char *getMessage() const { return message.c_str(); }
    int getPosition() const { return position; }
    int getLength() const { return length; }

private:
    std::string message;
    int position;
    int length;
};

#endif
