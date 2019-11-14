
#include "yajlHelpers.h"

#include <iostream>
#include <cstring>
#include "../types/Actions.h"

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

    yajl_gen_string_wrap(handle, NAME_STR);
    yajl_gen_string_wrap(handle, achievement.name.c_str());

    yajl_gen_string_wrap(handle, DESC_STR);
    yajl_gen_string_wrap(handle, achievement.desc.c_str());

    yajl_gen_string_wrap(handle, ID_STR);
    yajl_gen_string_wrap(handle, achievement.id.c_str());

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
}

/**
 * Encode an achievement vector into a given YAJL handle
 */
void 
encode_achievements(yajl_gen handle, std::vector<Achievement_t> achievements) {

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
}

//parsing the array inline would not be nice, so just extract them all here

std::vector<Achievement_t> 
decode_achievements(std::string response) {
    std::vector<Achievement_t> achievements;

    //parse response
    yajl_val node = yajl_tree_parse(response.c_str(), NULL, 0);

    if (node == NULL) {
        std::cerr << "parsing error";
        exit(EXIT_FAILURE);
    }

    // dumb defines for required interface for yajl_tree
    const char * list_path[] = { ACHIEVEMENT_LIST_STR, (const char*)0 };
    const char * name_path[] = { NAME_STR, (const char*)0 };
    const char * desc_path[] = { DESC_STR, (const char*)0 };
    const char * id_path[] = { ID_STR, (const char*)0 };
    const char * rate_path[] = { RATE_STR, (const char*)0 };
    const char * achieved_path[] = { ACHIEVED_STR, (const char*)0 };
    const char * hidden_path[] = { HIDDEN_STR, (const char*)0 };
    
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
        cur_val = yajl_tree_get(cur_node, name_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error" << std::endl;
        }
        achievements[i].name = YAJL_GET_STRING(cur_val);

        cur_val = yajl_tree_get(cur_node, desc_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error" << std::endl;
        }
        achievements[i].desc = YAJL_GET_STRING(cur_val);
        
        cur_val = yajl_tree_get(cur_node, id_path, yajl_t_string);
        if (cur_val == NULL) {
            std::cerr << "parsing error" << std::endl;
        }
        achievements[i].id = YAJL_GET_STRING(cur_val);
        
        cur_val = yajl_tree_get(cur_node, rate_path, yajl_t_number);
        if (cur_val == NULL) {
            std::cerr << "parsing error" << std::endl;
        }
        if (!YAJL_IS_DOUBLE(cur_val)) {
            std::cerr << "double float parsing error" << std::endl;
        }
        achievements[i].global_achieved_rate = YAJL_GET_DOUBLE(cur_val);
        
        // why is bool parsing weird
        cur_val = yajl_tree_get(cur_node, achieved_path, yajl_t_any);
        if (cur_val == NULL) {
            std::cerr << "parsing error" << std::endl;
        }
        if (!YAJL_IS_TRUE(cur_val) && !YAJL_IS_FALSE(cur_val)) {
            std::cerr << "bool parsing error" << std::endl;
        }
        achievements[i].achieved = YAJL_IS_TRUE(cur_val);

        cur_val = yajl_tree_get(cur_node, hidden_path, yajl_t_any);
        if (cur_val == NULL) {
            std::cerr << "parsing error" << std::endl;
        }
        if (!YAJL_IS_TRUE(cur_val) && !YAJL_IS_FALSE(cur_val)) {
            std::cerr << "bool parsing error" << std::endl;
        }
        achievements[i].hidden = YAJL_IS_TRUE(cur_val);
    }

    yajl_tree_free(node);
    return achievements;
}

/**
 * Encode an individual achievement change into a given YAJL handle
 */
void 
encode_change(yajl_gen handle, AchievementChange_t change) {
    yajl_gen_string_wrap(handle, ID_STR);
    yajl_gen_string_wrap(handle, change.id.c_str());

    yajl_gen_string_wrap(handle, ACHIEVED_STR);
    if (yajl_gen_bool(handle, change.achieved) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

void
encode_changes(yajl_gen handle, std::vector<AchievementChange_t> changes) {

    yajl_gen_string_wrap(handle, ACHIEVEMENT_LIST_STR);

    if (yajl_gen_array_open(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }

    for (AchievementChange_t change : changes) {
        if (yajl_gen_map_open(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }

        encode_change(handle, change);

        if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
            std::cerr << "failed to make json" << std::endl;
        }            
    }

    if (yajl_gen_array_close(handle) != yajl_gen_status_ok) {
        std::cerr << "failed to make json" << std::endl;
    }
}

std::string 
make_get_achivements_request_string() {
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
make_store_achivements_request_string(const std::vector<AchievementChange_t>& changes) {
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

    encode_request(handle, STORE_ACHIEVEMENTS_STR);
    encode_changes(handle, changes);

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