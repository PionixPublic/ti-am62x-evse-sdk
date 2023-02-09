// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#include "TiAM625SDK.hpp"

namespace module {

void TiAM625SDK::init() {
    invoke_init(*p_board_support);
}

void TiAM625SDK::ready() {
    invoke_ready(*p_board_support);
}

} // namespace module
