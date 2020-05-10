#include "functions.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <iostream>

void read_count(int fd, void *buf, size_t count)
{
    //bytes read per call
    ssize_t bytes;
    for (size_t i = 0; i < count; i += bytes) {
        errno = 0;
        bytes = read(fd, (void*)((char*)buf+i), count-i);
        if ((bytes == -1) || (bytes == 0 && errno > 0)) {
            std::cerr << "Read pipe encountered fatal error." << std::endl;
            zenity();
            exit(EXIT_FAILURE);
        }
    }
}

void write_count(int fd, void *buf, size_t count)
{
    //bytes written per call
    ssize_t bytes;
    for (size_t i = 0; i < count; i += bytes) {
        errno = 0;
        bytes = write(fd, (void*)((char*)buf+i), count-i);
        if ((bytes == -1) || (bytes == 0 && errno > 0)) {
            std::cerr << "Write pipe encountered fatal error." << std::endl;
            zenity();
            exit(EXIT_FAILURE);
        }
    }
}

bool file_exists(const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

bool strstri(const std::string & strHaystack, const std::string & strNeedle)
{
    auto it = std::search(
        strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(),   strNeedle.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return (it != strHaystack.end() );
}

bool digits_only(const std::string &str)
{
    return std::all_of(str.begin(), str.end(), ::isdigit) && str.length() > 0;
}

void mkdir_default(const char *pathname)
{
    int mkdir_error = mkdir(pathname, S_IRWXU | S_IRWXG | S_IROTH);
    if (mkdir_error != 0 && errno != EEXIST) {
        std::cerr << "Unable to create the folder " << pathname << ", errno " << errno << ")." << std::endl;
        zenity("Unable to create the folder " + std::string(pathname) + ", errno " + std::to_string(errno) + ".");
        exit(EXIT_FAILURE);
    }
}

std::string get_app_icon_path(std::string cache_folder, AppId_t app_id) {
    return cache_folder + "/" + std::to_string(app_id) + "/banner.jpg";
}

std::string get_achievement_icon_path(std::string cache_folder, AppId_t app_id, std::string id) {
    return cache_folder + "/" + std::to_string(app_id) + "/" + id + ".jpg";
}

void escape_html(std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(&data[pos], 1); break;
        }
    }
    data.swap(buffer);
}

int zenity(const std::string text, const std::string type) {
    return system( std::string("zenity " + type + " --text=\"" + text + "\" 2> /dev/null").c_str() );
}

bool convert_user_stat_value(UserStatType type, std::string buf, std::any* new_value) {
    bool valid_conversion = true;
    size_t idx = 0;

    try {
        // Cast these in their native steam implementation so
        // we don't truncate any user input precision,
        // then upcast to version used in JSON transport.
        //
        // We could also plumb in input validation for the
        // min/max/incremental values here
        if (type == UserStatType::Integer) {
            int a = std::stoi(buf, &idx);
            *new_value = static_cast<long long>(a);
        } else if (type == UserStatType::Float) {
            float a = std::stof(buf, &idx);
            *new_value = static_cast<double>(a);
        } else {
            std::cerr << "The stat value being converted has unrecognized type" << std::endl;
            valid_conversion = false;
        }
    } catch(std::exception& e) {
        // Invalid user input, but this is not fatal to the program
        valid_conversion = false;
    }

    // If the whole string hasn't been consumed, treat it as invalid
    if (buf[idx] != '\0') {
        valid_conversion = false;
    }

    return valid_conversion;
}

bool is_permission_protected(int permission) {
    return (permission & 3) != 0;
}