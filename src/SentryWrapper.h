#ifdef WITH_SENTRY
    #include "sentry.h"
#endif

#include <string>

std::string makeExecutablePath(const std::string& dir, const std::string& name);
void        initSentry();