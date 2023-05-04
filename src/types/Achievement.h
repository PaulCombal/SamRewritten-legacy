#pragma once

#include <cstdint>
#include <string>

// Mix them with bitwise operators
// NEXT_MOST_ACHIEVED = The next locked achievement achieved by the global player achievement rate
// ALMOST_FINISHED = The achievement progression is over 90%, if it has achievement progression, no implemnted yet
// RARE = Global player achievement rate is below 5%
enum eAchievementSpecial {
    ACHIEVEMENT_NORMAL = 0,
    ACHIEVEMENT_NEXT_MOST_ACHIEVED = 1,
    ACHIEVEMENT_ALMOST_FINISHED = 2,
    ACHIEVEMENT_RARE = 4
};

inline eAchievementSpecial operator|(eAchievementSpecial a, eAchievementSpecial b)
{
    return static_cast<eAchievementSpecial>(static_cast<int>(a) | static_cast<int>(b));
}

inline eAchievementSpecial operator|=(eAchievementSpecial& a, eAchievementSpecial b)
{
    return static_cast<eAchievementSpecial>((int&)(a) |= (int)(b));
}

/**
 * Achievement structure.
 * The special value is only used client-side for display purposes
 */
struct Achievement_t {
    std::string id;
    std::string name;
    std::string desc;
    std::string icon_name;
    float global_achieved_rate;
    bool achieved;
    bool hidden;
    int permission;
    eAchievementSpecial special;
};

typedef struct Achievement_t Achievement_t;

/**
 * AchievementChange structure
 * Minimum information needed to change an
 * achievement
 */
struct AchievementChange_t {
    std::string id;
    bool achieved;
    // Used for keeping track of the order an achievement modification
    // was put into the m_pending_ach_modifications map
    uint64_t selection_num;
};

typedef struct AchievementChange_t AchievementChange_t;