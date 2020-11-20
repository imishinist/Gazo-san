#include "gazosan.h"

namespace gazosan {

// private
std::string format_string(const TimeFormat& format_enum) {
  switch (format_enum) {
    case TimeFormat::YYYYMMDDHHMMSS:
      return "%Y%m%d%H%M%S";
    case TimeFormat::YYYYMMDD:
      return "%Y%m%d";
    case TimeFormat::HHMMSS:
      return "%H%M%S";
  }
}

// public
std::chrono::system_clock::time_point now() {
  return std::chrono::system_clock::now();
}

// public
std::string format_date(const std::chrono::system_clock::time_point& now,
                        const TimeFormat& format_enum) {
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  auto format = format_string(format_enum);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), format.c_str());
  return ss.str();
}
}  // namespace gazosan
