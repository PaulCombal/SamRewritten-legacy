#pragma once

#include <string>

// Mix them with bitwise operators
// NEXT_MOST_ACHIEVED = The next locked achievement achieved by the global player achievement rate
// ALMOST_FINISHED = The achievement progression is over 90%, if it has achievement progression, no implemnted yet
// RARE = Global player achievement rate is below 5%
enum eAchievementSpecial {
    ACHIEVEMENT_NORMAL = 1,
    ACHIEVEMENT_NEXT_MOST_ACHIEVED = 2,
    ACHIEVEMENT_ALMOST_FINISHED = 4,
    ACHIEVEMENT_RARE = 8
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
    std::string name;
    std::string desc;
    std::string id;
    float global_achieved_rate;  
	bool achieved;
    bool hidden;
    eAchievementSpecial special;
};

typedef struct Achievement_t Achievement_t;

/**
 * AchievementChange structure
 * Minimum information needed to change an
 * achievement
 * TODO add stats
 */
struct AchievementChange_t {
    std::string id;
	bool achieved;
};

typedef struct AchievementChange_t AchievementChange_t;