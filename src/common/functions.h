#pragma once

#include <string>
#include "../globals.h"
#include "../../steam/steam_api.h"

/**
 * Wrapper for fork()
 */
pid_t create_process();

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
 * Concatenates two C strings
 */
char* concat(const char *s1, const char *s2);

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
std::string get_app_icon_path(AppId_t app_id);

/**
 * Generate path to given app achievement icon
 */
std::string get_achievement_icon_path(AppId_t app_id, std::string id);

/**
 * Escape html characters inline a string.
 * From https://stackoverflow.com/a/5665377
 */
void escape_html(std::string& data);
