#ifndef _COMMON_H_
#define _COMMON_H_
#include <string>
#include <vector>

using bytes = std::vector<unsigned char>;

bytes to_bytes(const std::string &s);

#endif
