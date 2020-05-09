
#include "yajlHelpers.h"

#include <iostream>
#include <cstring>
#include "../types/Actions.h"
#include "../common/functions.h"

void 
yajl_gen_string_wrap(yajl_gen handle, const char * a) {
    if (yajl_gen_string(handle, (const unsigned char *)a, strlen(a)) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

void
encode_ack(yajl_gen handle) {
    yajl_gen_string_wrap(handle, SAM_ACK_STR);
    yajl_gen_string_wrap(handle, SAM_ACK_STR);
}

bool
decode_ack(std::string response) {
    yajl_val node = yajl_tree_parse(response.c_str(), NULL, 0);

    if (node == NULL) {
        std::cerr << "Parsing error. Data: " << std::endl;
        std::cerr << response << std::endl;
        zenity();
        exit(EXIT_FAILURE);
    }
    const char * path[] = { SAM_ACK_STR, (const char*)0 };

    yajl_val cur_val = yajl_tree_get(node, path, yajl_t_string);
    if (cur_val == NULL) {
        std::cerr << "parsing error" << std::endl;
    }

    std::string ack_value = YAJL_GET_STRING(cur_val);
    yajl_tree_free(node);
    
    return std::string(SAM_ACK_STR) == ack_value;
}

void 
encode_request(yajl_gen handle, const char * request) {
    yajl_gen_string_wrap(handle, SAM_ACTION_STR);
    yajl_gen_string_wrap(handle, request);
}

/**
 * Encode an individual achievement into a given YAJL handle
 */
void 
encode_achievement(yajl_gen handle, Achievement_t achievement) {

    yajl_gen_string_wrap(handle, ID_STR);
    yajl_gen_string_wrap(handle, achievement.id.c_str());

    yajl_gen_string_wrap(handle, NAME_STR);
    yajl_gen_string_wrap(handle, achievement.name.c_str());

    yajl_gen_string_wrap(handle, DESC_STR);
    yajl_gen_string_wrap(handle, achievement.desc.c_str());

    yajl_gen_string_wrap(handle, ICON_STR);
    yajl_gen_string_wrap(handle, achievement.icon_name.c_str());

    // https://github.com/lloyd/yajl/issues/222
    yajl_gen_string_wrap(handle, RATE_STR);
    if (yajl_gen_double(handle, (double)achievement.global_achieved_rate) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
    }

    yajl_gen_string_wrap(handle, ACHIEVED_STR);
    if (yajl_gen_bool(handle, achievement.achieved) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    yajl_gen_string_wrap(handle, HIDDEN_STR);
    if (yajl_gen_bool(handle, achievement.hidden) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    yajl_gen_string_wrap(handle, PERMISSION_STR);
    if (yajl_gen_integer(handle, achievement.permission) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

/**
 * Encode an individual stat into a given YAJL handle
 */
void 
encode_stat(yajl_gen handle, StatValue_t stat) {

    yajl_gen_string_wrap(handle, STAT_ID_STR);
    yajl_gen_string_wrap(handle, stat.id.c_str());

    yajl_gen_string_wrap(handle, STAT_DISPLAY_STR);
    yajl_gen_string_wrap(handle, stat.display_name.c_str());

    yajl_gen_string_wrap(handle, STAT_PERMISSION_STR);
    if (yajl_gen_integer(handle, stat.permission) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    yajl_gen_string_wrap(handle, STAT_TYPE_STR);
    if (yajl_gen_integer(handle, (int)stat.type) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    if (stat.type == UserStatType::Integer)
    {
        int value = std::any_cast<int>(stat.value);
        
        yajl_gen_string_wrap(handle, STAT_VALUE_STR);
        if (yajl_gen_integer(handle, value) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }
    }
    else if (stat.type == UserStatType::Float)
    {
        float value = std::any_cast<float>(stat.value);

        yajl_gen_string_wrap(handle, STAT_VALUE_STR);
        if (yajl_gen_double(handle, value) != yajl_gen_status_ok) {
                std::cerr << "failed to make json" << std::endl;
        }
    }

    yajl_gen_string_wrap(handle, STAT_INCREMENTONLY_STR);
    if (yajl_gen_bool(handle, stat.incrementonly) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

/**
 * Encode an achievement vector into a given YAJL handle
 */
void 
encode_achievements_and_stats(yajl_gen handle, std::vector<Achievement_t> achievements, std::vector<StatValue_t> stats) {

    // Achievements array
    yajl_gen_string_wrap(handle, ACHIEVEMENT_LIST_STR);

    if (yajl_gen_array_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    for (Achievement_t achievement : achievements) {
        if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }

        encode_achievement(handle, achievement);

        if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }
    }

    if (yajl_gen_array_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    // Stats array
    yajl_gen_string_wrap(handle, STAT_LIST_STR);

    if (yajl_gen_array_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    for (StatValue_t stat : stats) {
        if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }

        encode_stat(handle, stat);

        if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }
    }

    if (yajl_gen_array_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

std::vector<StatValue_t>
decode_stats(std::string response) {
    std::vector<StatValue_t> stats;
    char error_buffer[500];

    yajl_val node = yajl_tree_parse(response.c_str(), error_buffer, 500);

    if (node == NULL) {
        std::cerr << "Parsing error: " << error_buffer << std::endl;
        zenity();
        exit(EXIT_FAILURE);
    }

    const char * stat_path[] = { STAT_LIST_STR, (const char*)0 };
    const char * id_path[] = { STAT_ID_STR, (const char*)0 };
    const char * display_path[] = { STAT_DISPLAY_STR, (const char*)0 };
    const char * permission_path[] = { STAT_PERMISSION_STR, (const char*)0 };
    const char * type_path[] = { STAT_TYPE_STR, (const char*)0 };
    const char * value_path[] = { STAT_VALUE_STR, (const char*)0 };
    const char * incrementonly_path[] = { STAT_INCREMENTONLY_STR, (const char*)0 };

    yajl_val v = yajl_tree_get(node, stat_path, yajl_t_array);
    if (v == NULL) {
        std::cerr << "parsing error (no stat listing found)" << std::endl;
    }

    yajl_val *w = YAJL_GET_ARRAY(v)->values;
    size_t array_len = YAJL_GET_ARRAY(v)->len;

    stats.clear();
    stats.resize(array_len);

    for(unsigned i = 0; i < array_len; i++) {
        yajl_val cur_node = w[i];
        yajl_val cur_val;

        cur_val = yajl_tree_get(cur_node, id_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error (stat id)" << std::endl;
        }
        stats[i].id = YAJL_GET_STRING(cur_val);

        cur_val = yajl_tree_get(cur_node, display_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error (stat display)" << std::endl;
        }
        stats[i].display_name = YAJL_GET_STRING(cur_val);

        cur_val = yajl_tree_get(cur_node, permission_path, yajl_t_number);
        if (cur_val == NULL) {
            std::cerr << "parsing error (stat permission)" << std::endl;
        }
        stats[i].permission = YAJL_GET_INTEGER(cur_val);

        cur_val = yajl_tree_get(cur_node, type_path, yajl_t_number);
        if (cur_val == NULL) {
            std::cerr << "parsing error (stat type)" << std::endl;
        }
        stats[i].type = (UserStatType)YAJL_GET_INTEGER(cur_val);

        if (stats[i].type == UserStatType::Integer)
        {
            cur_val = yajl_tree_get(cur_node, value_path, yajl_t_number);
            if (cur_val == NULL) {
                std::cerr << "parsing error (stat value int)" << std::endl;
            }
            stats[i].value = YAJL_GET_INTEGER(cur_val);
        }
        else if (stats[i].type == UserStatType::Float) {
            cur_val = yajl_tree_get(cur_node, value_path, yajl_t_number);
            if (cur_val == NULL) {
                std::cerr << "parsing error (stat value float)" << std::endl;
            }
            stats[i].value = YAJL_GET_DOUBLE(cur_val);
        }
        else {
            std::cerr << "Unable to get stat value: Unsupported type. " << stats[i].id << std::endl;
        }

        cur_val = yajl_tree_get(cur_node, incrementonly_path, yajl_t_any);
        if (cur_val == NULL) {
            std::cerr << "parsing error (stat incrementonly)" << std::endl;
        }
        if (!YAJL_IS_TRUE(cur_val) && !YAJL_IS_FALSE(cur_val)) {
            std::cerr << "bool parsing error (stat incrementonly)" << std::endl;
        }
        stats[i].incrementonly = YAJL_IS_TRUE(cur_val);
    }

    return stats;
}

std::vector<Achievement_t> 
decode_achievements(std::string response) {
    std::vector<Achievement_t> achievements;
    char error_buffer[500];

    //parse response
    yajl_val node = yajl_tree_parse(response.c_str(), error_buffer, 500);

    if (node == NULL) {
        std::cerr << "Parsing error: " << error_buffer << std::endl;
        zenity();
        exit(EXIT_FAILURE);
    }

    // dumb defines for required interface for yajl_tree
    const char * list_path[] = { ACHIEVEMENT_LIST_STR, (const char*)0 };
    const char * id_path[] = { ID_STR, (const char*)0 };
    const char * name_path[] = { NAME_STR, (const char*)0 };
    const char * desc_path[] = { DESC_STR, (const char*)0 };
    const char * icon_path[] = { ICON_STR, (const char*)0 };
    const char * rate_path[] = { RATE_STR, (const char*)0 };
    const char * achieved_path[] = { ACHIEVED_STR, (const char*)0 };
    const char * hidden_path[] = { HIDDEN_STR, (const char*)0 };
    const char * permission_path[] = { PERMISSION_STR, (const char*)0 };

    yajl_val v = yajl_tree_get(node, list_path, yajl_t_array);
    if (v == NULL) {
        std::cerr << "parsing error" << std::endl;
    }

    yajl_val *w = YAJL_GET_ARRAY(v)->values;
    size_t array_len = YAJL_GET_ARRAY(v)->len;

    achievements.clear();
    achievements.resize(array_len);

    for(unsigned i = 0; i < array_len; i++) {
        yajl_val cur_node = w[i];
        yajl_val cur_val;

        // TODO: pull these out into a decode_achievement()?
        // verification is done via the type argument to yajl_tree_get
        // and via YAJL_IS_* checks if type alone isn't sufficient

        cur_val = yajl_tree_get(cur_node, id_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error (ach id)" << std::endl;
        }
        achievements[i].id = YAJL_GET_STRING(cur_val);

        cur_val = yajl_tree_get(cur_node, name_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error (ach name)" << std::endl;
        }
        achievements[i].name = YAJL_GET_STRING(cur_val);

        cur_val = yajl_tree_get(cur_node, desc_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error (ach desc)" << std::endl;
        }
        achievements[i].desc = YAJL_GET_STRING(cur_val);

        cur_val = yajl_tree_get(cur_node, icon_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error (ach icon)" << std::endl;
        }
        achievements[i].icon_name = YAJL_GET_STRING(cur_val);
        
        cur_val = yajl_tree_get(cur_node, rate_path, yajl_t_number);
        if (cur_val == NULL) {
            std::cerr << "parsing error (ach rate)" << std::endl;
        }
        if (!YAJL_IS_DOUBLE(cur_val)) {
            std::cerr << "double float parsing error" << std::endl;
        }
        achievements[i].global_achieved_rate = YAJL_GET_DOUBLE(cur_val);
        
        // why is bool parsing weird
        cur_val = yajl_tree_get(cur_node, achieved_path, yajl_t_any);
        if (cur_val == NULL) {
            std::cerr << "parsing error (ach achieved)" << std::endl;
        }
        if (!YAJL_IS_TRUE(cur_val) && !YAJL_IS_FALSE(cur_val)) {
            std::cerr << "bool parsing error" << std::endl;
        }
        achievements[i].achieved = YAJL_IS_TRUE(cur_val);

        cur_val = yajl_tree_get(cur_node, hidden_path, yajl_t_any);
        if (cur_val == NULL) {
            std::cerr << "parsing error (ach hidden)" << std::endl;
        }
        if (!YAJL_IS_TRUE(cur_val) && !YAJL_IS_FALSE(cur_val)) {
            std::cerr << "bool parsing error" << std::endl;
        }
        achievements[i].hidden = YAJL_IS_TRUE(cur_val);

        cur_val = yajl_tree_get(cur_node, permission_path, yajl_t_number);
        if (cur_val == NULL) {
            std::cerr << "parsing error (achievement permission)" << std::endl;
        }
        achievements[i].permission = YAJL_GET_INTEGER(cur_val);
    }

    yajl_tree_free(node);
    return achievements;
}

/**
 * Encode an individual achievement change into a given YAJL handle
 */
void 
encode_achievement_change(yajl_gen handle, AchievementChange_t change) {
    yajl_gen_string_wrap(handle, ID_STR);
    yajl_gen_string_wrap(handle, change.id.c_str());

    yajl_gen_string_wrap(handle, ACHIEVED_STR);
    if (yajl_gen_bool(handle, change.achieved) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

void
encode_achievement_changes(yajl_gen handle, std::vector<AchievementChange_t> changes) {

    yajl_gen_string_wrap(handle, ACHIEVEMENT_LIST_STR);

    if (yajl_gen_array_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    for (AchievementChange_t change : changes) {
        if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }

        encode_achievement_change(handle, change);

        if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }            
    }

    if (yajl_gen_array_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

/**
 * Encode an individual stat change into a given YAJL handle
 */
void 
encode_stat_change(yajl_gen handle, StatChange_t change) {
    yajl_gen_string_wrap(handle, STAT_ID_STR);
    yajl_gen_string_wrap(handle, change.id.c_str());

    yajl_gen_string_wrap(handle, STAT_TYPE_STR);
    if (yajl_gen_integer(handle, (int)change.type) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    if (change.type == UserStatType::Integer)
    {
        long long value = std::any_cast<long long>(change.new_value);
        
        yajl_gen_string_wrap(handle, STAT_VALUE_STR);
        if (yajl_gen_integer(handle, value) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }
    }
    else if (change.type == UserStatType::Float)
    {
        double value = std::any_cast<double>(change.new_value);

        yajl_gen_string_wrap(handle, STAT_VALUE_STR);
        if (yajl_gen_double(handle, value) != yajl_gen_status_ok) {
                std::cerr << "failed to make json" << std::endl;
        }
    }
}

void
encode_stat_changes(yajl_gen handle, std::vector<StatChange_t> changes) {

    yajl_gen_string_wrap(handle, STAT_LIST_STR);

    if (yajl_gen_array_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    for (StatChange_t change : changes) {

        if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }

        encode_stat_change(handle, change);

        if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }            
    }

    if (yajl_gen_array_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

std::string 
make_get_achievements_request_string() {
    const unsigned char * buf;
    size_t len;
    std::string ret;

    yajl_gen handle = yajl_gen_alloc(NULL);
    if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    encode_request(handle, GET_ACHIEVEMENTS_STR);

    if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    yajl_gen_get_buf(handle, &buf, &len);
    ret = std::string((const char*)buf);
    yajl_gen_free(handle);

    return ret;
}

std::string 
make_commit_changes_request_string(const std::vector<AchievementChange_t>& achievement_changes, const std::vector<StatChange_t>& stat_changes) {
    const unsigned char * buf; 
    size_t len;
    std::string ret;
    
    yajl_gen handle = yajl_gen_alloc(NULL); 
    if (handle == NULL) {
        std::cerr << "Failed to make handle" << std::endl;
    }

    if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    encode_request(handle, COMMIT_CHANGES_STR);
    encode_achievement_changes(handle, achievement_changes);
    encode_stat_changes(handle, stat_changes);

    if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    yajl_gen_get_buf(handle, &buf, &len);
    ret = std::string((const char*)buf);
    yajl_gen_free(handle);

    return ret;
}

std::string 
make_kill_server_request_string() {
    std::string ret;
    const unsigned char * buf; 
    size_t len;

    yajl_gen handle = yajl_gen_alloc(NULL);
    if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
    encode_request(handle, QUIT_GAME_STR);
    if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
    yajl_gen_get_buf(handle, &buf, &len);
    ret = std::string((const char*)buf);
    yajl_gen_free(handle);

    return ret;
}