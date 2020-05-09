#pragma once

#include <string>
#include <any>

#include "UserStatType.h"

struct StatValue_t {
    std::string id;
    std::string display_name;
    int permission;
    UserStatType type;
    std::any value;
    bool incrementonly;
};

typedef struct StatValue_t StatValue_t;

/**
 * StatChange structure
 * Minimum information needed to change a stat
 */
struct StatChange_t {
    UserStatType type;
    std::string id;
    std::any new_value;
};

typedef struct StatChange_t StatChange_t;