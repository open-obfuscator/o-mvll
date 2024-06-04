#ifndef OMVLL_POSTCODING_H
#define OMVLL_POSTCODING_H

namespace omvll {

#define STRINGIZE(s) #s
#define EXPAND_AND_STRINGIZE(s) STRINGIZE(s)
#define __tak_injection lKeEc0yFCfkP2MlhnLkhRrPMjX_gGT37XzdnMcdllvg
#define TakInjectionFunctionName EXPAND_AND_STRINGIZE(__tak_injection)

static constexpr auto TakInjectionFunction = R"delim(
  extern "C" {
    #import <Foundation/Foundation.h>
    #import <dispatch/dispatch.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include "./iOS/C/include/tak.h"

    #define NO_NETWORK_RETRIES  5
    #define RETRY_WAIT_INTERVAL 10
    #define CHECK_WAIT_INTERVAL 10
    #define MAX_RETRY           10
    #define DQP_BACKGROUND DISPATCH_QUEUE_PRIORITY_BACKGROUND

    void )delim" TakInjectionFunctionName R"delim(() {
      dispatch_async(dispatch_get_global_queue(DQP_BACKGROUND, 0), ^{
        NSString *documentdir =
        [NSSearchPathForDirectoriesInDomains(
            NSDocumentDirectory, NSUserDomainMask, YES) lastObject];

        char* workingPath =
        (char*)[documentdir cStringUsingEncoding:NSUTF8StringEncoding];

        TAK_RETURN ret = TakLib_initialize(workingPath,
                                           "license.tak",
                                           NULL,
                                           NULL);

        if (ret != TAK_SUCCESS && ret != TAK_API_ALREADY_INITIALIZED
            && ret != TAK_LICENSE_ABOUT_TO_EXPIRE) {
            // Make it crash
          abort();
        }

        TAK_RETURN error;
        bool isRegistered = TakLib_isRegistered(&error);
        int retryNum = 0;
        if (!isRegistered) {
          do {
            if (retryNum > 0)
              sleep(RETRY_WAIT_INTERVAL);

            ret = TakLib_register(NULL);
            retryNum++;
          } while (retryNum < MAX_RETRY &&
                    (ret == TAK_NETWORK_TIMEOUT || ret == TAK_NETWORK_ERROR));
          if (ret != TAK_SUCCESS && ret != TAK_API_ALREADY_INITIALIZED
              && ret != TAK_LICENSE_ABOUT_TO_EXPIRE) {
                // Make it crash
                abort();
              }
        } else {
          do {
            ret = TakLib_checkIntegrity(NULL);
            retryNum++;
          } while (retryNum < MAX_RETRY &&
                    (ret == TAK_NETWORK_TIMEOUT || ret == TAK_NETWORK_ERROR));
          if (ret != TAK_SUCCESS && ret != TAK_RE_REGISTER_SUCCESS
              && ret != TAK_LICENSE_ABOUT_TO_EXPIRE && ret != TAK_NETWORK_TIMEOUT
              && ret != TAK_NETWORK_ERROR) {
            // Make it crash
            abort();
          }
        }

        ret = TakLib_createRuntimeCheckThread({{ TAK_INTERVAL_TIME }});
        if (ret != TAK_SUCCESS) {
          // Make it crash
          abort();
        }

        while (true) {
          sleep(CHECK_WAIT_INTERVAL);
          TakLib_isRuntimeThreadActive(true);
        }

        return;
      });
    }
  }
)delim";

} // namespace omvll

#endif
