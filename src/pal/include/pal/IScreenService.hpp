#pragma once

#include "pal/Types.hpp"

#include <vector>

namespace pal {

// Enumeration of physical monitors. Hot-plug notification and persistent
// monitor identity are introduced later (Fase 3); Fase 0 provides basic
// enumeration only.
class IScreenService {
public:
    virtual ~IScreenService() = default;

    virtual std::vector<ScreenInfo> screens() const = 0;
    virtual ScreenInfo primaryScreen() const = 0;
};

} // namespace pal
