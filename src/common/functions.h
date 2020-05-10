#pragma once

#include <string>
#include <any>
#include "../globals.h"
#include "../../steam/steam_api.h"
#include "../types/UserStatType.h"

/**
 * Stats hold a value of type any, but can be either int or float.
 * This macro casts the value to the right type.
 * The cast types are the ones used by YAJL
 */
#define GET_STAT_VALUE(stat) (stat.type == UserStatType::Integer ? std::any_cast<long long>(stat.value) : std::any_cast<double>(stat.value))

/**
 * Wrapper for read/write to actually read/write count 
 * bytes instead of reading/writing up to count bytes,
 * or fail hard on error
 */
void read_count(int fd, void *buf, size_t count);
void write_count(int fd, void *buf, size_t count);

/**
 * Feed it a path to a file name, will return whether the file
 * exists or not on the current machine
 */
bool file_exists(const std::string& name);

/**
 * Insensitive "string in string"
 */
bool strstri(const std::string & strHaystack, const std::string & strNeedle);

/**
 * Returns true if str is only containing digits, and is not empty
 */
bool digits_only(const std::string& str);

/**
 * Make a directory with reasonable defaults
 * or fail hard on error
 */
void mkdir_default(const char *pathname);

/**
 * Generate path to given app icon
 */
std::string get_app_icon_path(std::string cache_folder, AppId_t app_id);

/**
 * Generate path to given app achievement icon
 */
std::string get_achievement_icon_path(std::string cache_folder, AppId_t app_id, std::string id);

/**
 * Escape html characters inline a string.
 * From https://stackoverflow.com/a/5665377
 */
void escape_html(std::string& data);

/**
 * Show a regular dialog box. Return value is ignored for now,
 * but feel free to add functionnlity to this
 */
int zenity(const std::string text = "An internal error occurred, please open a Github issue with the console output to get it fixed!", const std::string type = "--error --no-wrap");

/**
 * Convert a user stat value string buffer to the specified stat type
 * Directly modifies new_value
 * Returns whether the conversion was successful
 */
bool convert_user_stat_value(UserStatType type, std::string buf, std::any* new_value);

/**
 * Return whether an achievement permission is protected
 */
bool is_permission_protected(int permission);