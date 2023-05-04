/* Copyright (c) 2017 Rick (rick 'at' gibbed 'dot' us)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

// This code is ported to C++ from gibbed's original Steam Achievement Manager.
// The original SAM is available at https://github.com/gibbed/SteamAchievementManager
// To comply with copyright, the above license is included.

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <any>

enum class KeyValueType : unsigned char
{
    None = 0,
    String = 1,
    Int32 = 2,
    Float32 = 3,
    Pointer = 4,
    WideString = 5,
    Color = 6,
    UInt64 = 7,
    End = 8
};

class KeyValue {
private:
    static KeyValue* Invalid;

public:
    std::string name = "<root>";
    KeyValueType type = KeyValueType::None;
    std::any value;
    bool valid = false;
    std::vector<KeyValue*> children;

    //KeyValue* operator[](std::string key); // Would add spice?
    KeyValue* get(std::string key);
    KeyValue* get2(std::string key1, std::string key2);

    std::string as_string(std::string default_value);
    int as_integer(int default_value);
    float as_float(float default_value);
    bool as_boolean(bool default_value);
    // Other as_type function can be implemented as needed
    
    static KeyValue* load_as_binary(std::string path);
    bool read_as_binary(std::istream *is);

    KeyValue() { valid = true;};
    ~KeyValue() {
        for (auto child : children) {
            delete child;
        }
    };

};