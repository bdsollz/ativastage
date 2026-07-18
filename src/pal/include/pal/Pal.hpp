#pragma once

#include "pal/IPathService.hpp"
#include "pal/IScreenService.hpp"

#include <memory>

namespace pal {

// Factory returning the platform implementation selected at compile time.
// This is the ONLY place platform selection is allowed to leak into the API;
// no #ifdef Q_OS_* is permitted outside libpal (auditable rule, Seção 2.4).
std::unique_ptr<IPathService>   makePathService();
std::unique_ptr<IScreenService> makeScreenService();

} // namespace pal
