#ifndef FLYTO_REQUEST_MANAGER_H_
#define FLYTO_REQUEST_MANAGER_H_

#include "requestContextManager.h"

#include <string>
#include <unordered_map>
#include <functional>


class FlytoRequestManager : public RequestContextManager {
public:

    FlytoRequestManager();

    void handleFlytoResult(const std::string& msg);

private:
    std::unordered_map<std::string, std::string> m_resultMap;
};

#endif /* FLYTO_REQUEST_MANAGER_H_ */
