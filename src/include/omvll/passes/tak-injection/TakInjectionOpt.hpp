#ifndef OMVLL_TAKINJECTION_OPT_H
#define OMVLL_TAKINJECTION_OPT_H
#include <string>
#include <vector>

namespace omvll {
struct TakInjectionSkip {};

struct TakInjectionConfig {
  TakInjectionConfig() = delete;
  TakInjectionConfig(std::string TAKHeaderPath, unsigned intervalTime,
                     std::vector<std::string> excludedTargets)
      : TAKHeaderPath(std::move(TAKHeaderPath)), intervalTime(intervalTime),
        excludedTargets(std::move(excludedTargets)) {}

  std::string TAKHeaderPath;
  unsigned intervalTime;
  std::vector<std::string> excludedTargets;
};

using TakInjectionOpt = std::variant<TakInjectionSkip, TakInjectionConfig>;

} // namespace omvll

#endif
