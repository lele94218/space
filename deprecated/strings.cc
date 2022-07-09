#include "strings.h"

namespace strings {

bool EndsWith(const std::string& from, const std::string& ending) {
  if (from.length() >= ending.length()) {
    return 0 == from.compare(from.length() - ending.length(), ending.length(), ending);
  }
  return false;
}

}  // namespace strings
