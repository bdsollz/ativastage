#include "pal/Pal.hpp"

namespace pal {

// Forward declarations of the per-platform factories (defined in mac/ or win/).
std::unique_ptr<IPathService>   makePathServiceMac();
std::unique_ptr<IPathService>   makePathServiceWin();
std::unique_ptr<IScreenService> makeScreenServiceMac();
std::unique_ptr<IScreenService> makeScreenServiceWin();

std::unique_ptr<IPathService> makePathService() {
#if defined(_WIN32)
    return makePathServiceWin();
#else
    return makePathServiceMac();
#endif
}

std::unique_ptr<IScreenService> makeScreenService() {
#if defined(_WIN32)
    return makeScreenServiceWin();
#else
    return makeScreenServiceMac();
#endif
}

} // namespace pal
