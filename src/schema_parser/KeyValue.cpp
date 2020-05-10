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

#include "../common/functions.h"
#include "KeyValue.h"
#include <strings.h>

// https://stackoverflow.com/questions/8267847/why-should-i-initialize-static-class-variables-in-c
KeyValue* KeyValue::Invalid = new KeyValue();

// Stream helper functions

int8_t read_value_u8(std::istream * is) {
    return static_cast<int8_t>(is->get());
}

int32_t read_value_s32(std::istream * is) {
    char buf[4];
    is->read(buf, 4);
    return *reinterpret_cast<int32_t*>(buf);
}

uint32_t read_value_u32(std::istream * is) {
    char buf[4];
    is->read(buf, 4);
    return *reinterpret_cast<uint32_t*>(buf);
}

uint64_t read_value_u64(std::istream * is) {
    char buf[8];
    is->read(buf, 8);
    return *reinterpret_cast<uint64_t*>(buf);
}

float read_value_f32(std::istream * is) {
    char buf[4];
    is->read(buf, 4);
    return *reinterpret_cast<float*>(buf);
}

std::string read_string(std::istream * is) {
    char buf[512];

    // Even in unicode, NULL terminator will terminate the string.
    // C++ actually automatically interprets an array of bytes
    // as unicode if there are unicode encodings in it,
    // so no special modification is needed.
    // Eat the string and NULL terminator with getline.
    if( !is->getline( buf, 512, L'\0') ) {
        std::cerr << "Failed to read a KeyValue. Increasing buffer size might help." << std::endl;
    }

    // NULL is automatically appended
    return std::string(buf);
}

// returns KeyValue::Invalid if no children or children not found
//KeyValue* KeyValue::operator[](std::string key) {
KeyValue* KeyValue::get(std::string key) {
    if (this->valid == false) {
        return KeyValue::Invalid;
    }

    KeyValue* select_child = NULL;

    for (auto child : this->children) {
        if (strcasecmp(child->name.c_str(), key.c_str()) == 0) {
            select_child = child;
        }
    }

    return select_child ?: KeyValue::Invalid;
}

KeyValue* KeyValue::get2(std::string key1, std::string key2) {
    auto a = this->get(key1);
    if (!a->valid) {
        return a;
    }
    auto b = a->get(key2);
    return b;
}


std::string KeyValue::as_string(std::string default_value) {
    if (this->valid == false) {
        return default_value;
    }

    if (!this->value.has_value()) {
        return default_value;
    }

    // I don't think support for any other types is needed right now,
    // but if it is, fail hard to avoid complications
    if (this->value.type() != typeid(std::string)) {
        std::cout << "Stats parser encountered fatal error!" << std::endl;
        std::cout << "as_string attempted on non-string type" << std::endl;
        std::cout << "exiting now to avoid complications" << std::endl;
        zenity();
        exit(EXIT_FAILURE);
    }

    return std::any_cast<std::string>(this->value);
}

int KeyValue::as_integer(int default_value) {
    if (this->valid == false) {
        return default_value;
    }

    if (!this->value.has_value()) {
        return default_value;
    }

    switch (this->type) {
        case KeyValueType::String:
        case KeyValueType::WideString:
        {
            try {
                return std::stoi(std::any_cast<std::string>(this->value));
            } catch (std::exception&) {
                return default_value;
            }
        }

        case KeyValueType::Int32:
        {
            return std::any_cast<int32_t>(this->value);
        }

        case KeyValueType::UInt64:
        {
            return std::any_cast<uint64_t>(this->value);
        }

        case KeyValueType::Float32:
        {
            return std::any_cast<float>(this->value);
            break;
        }

        default: {}
    }

    return default_value;
}

float KeyValue::as_float(float default_value) {
    if (this->valid == false) {
        return default_value;
    }

    if (!this->value.has_value()) { // This check is not part of original C# SAM
        return default_value;
    }

    switch (this->type) {
        case KeyValueType::String:
        case KeyValueType::WideString:
        {
            try {
                return std::stof(std::any_cast<std::string>(this->value));
            } catch (std::exception&) {
                return default_value;
            }
        }

        case KeyValueType::Int32:
        {
            return std::any_cast<int32_t>(this->value);
        }

        case KeyValueType::UInt64:
        {
            return std::any_cast<uint64_t>(this->value);
        }

        case KeyValueType::Float32:
        {
            return std::any_cast<float>(this->value);
        }

        default: {}
    }

    return default_value;
}

bool KeyValue::as_boolean(bool default_value) {
    if (this->valid == false)
    {
        return default_value;
    }

    switch (this->type)
    {
        case KeyValueType::String:
        case KeyValueType::WideString:
        {
            try {
                int pr = std::stoi(std::any_cast<std::string>(this->value));
                return pr != 0;
            } catch (std::exception&) {
                return default_value;
            }
        }
        
        case KeyValueType::Int32:
        {
            return std::any_cast<int32_t>(this->value) != 0;
        }

        case KeyValueType::UInt64:
        {
            return std::any_cast<uint64_t>(this->value) != 0;
        }

        case KeyValueType::Float32:
        {
            return std::any_cast<float>(this->value) != 0;
        }

        default: {}
    }

    return default_value;
}

KeyValue* KeyValue::load_as_binary(std::string path) {
    #ifdef DEBUG_CERR
    std::cerr << "Parsing schema file " << path << std::endl;
    #endif

    std::filebuf fb;
    if (!fb.open(path, std::ios::in)) {
        std::cerr << "Unable to open schema file: " << path << std::endl;
        return NULL;
    }

    std::istream is(&fb);

    auto kv = new KeyValue();

    if (!kv->read_as_binary(&is)) {
        delete kv;
        kv = NULL;
    }

    fb.close();

    return kv;
};

bool KeyValue::read_as_binary(std::istream* is) {
    while (true) {

        KeyValueType type = static_cast<KeyValueType>(is->get());

        if (type == KeyValueType::End) {
            break;
        }

        auto current = new KeyValue();
        current->type = type;
        current->name = read_string(is);

        switch (type)
        {
            case KeyValueType::None:
            {
                current->read_as_binary(is);
                break;
            }

            case KeyValueType::String:
            {
                current->valid = true;
                current->value = read_string(is);
                break;
            }

            case KeyValueType::WideString:
            {
                std::cout << "Stats parser encountered invalid unsupported wide string!" << std::endl;
                delete current;
                return false;
            }

            case KeyValueType::Int32:
            {
                current->valid = true;
                current->value = read_value_s32(is);
                break;
            }

            case KeyValueType::UInt64:
            {
                current->valid = true;
                current->value = read_value_u64(is);
                break;
            }

            case KeyValueType::Float32:
            {
                current->valid = true;
                current->value = read_value_f32(is);
                break;
            }

            case KeyValueType::Color:
            {
                current->valid = true;
                current->value = read_value_u32(is);
                break;
            }

            case KeyValueType::Pointer:
            {
                current->valid = true;
                current->value = read_value_u32(is);
                break;
            }

            default:
            {
                std::cout << "Stats parser encountered invalid type: " << static_cast<unsigned>(type) << " at offset " << is->tellg() << std::endl;
                delete current;
                return false;
            }
        }
        
        this->children.push_back(current);
    }

    this->valid = true;

    // Make sure the stream is ok before reading for EOF
    if (!is->good()) {
        return false;
    }

    // Then check that we're at the end
    if (is->peek() != std::ifstream::traits_type::eof()) {
        return false;
    }

    return true;
}
