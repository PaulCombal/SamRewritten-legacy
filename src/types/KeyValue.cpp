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

#include "KeyValue.h"
#include <strings.h>

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
    char buf[256];

    // Even in unicode, NULL terminator will terminate the string.
    // C++ actually automatically interprets an array of bytes
    // as unicode if there are unicode encodings in it,
    // so no special modification is needed.
    // Eat the string and NULL terminator with getline.
    is->getline( buf, 256, L'\0');

    // NULL is automatically appended
    return std::string(buf);
}

// returns NULL if no children or children not found
//KeyValue* KeyValue::operator[](std::string key) {
KeyValue* KeyValue::get(std::string key) {
    if (this->valid == false) {
        return NULL;
    }

    KeyValue* select_child = NULL;

    for (auto child : this->children) {
        if (strcasecmp(child->name.c_str(), key.c_str()) == 0) {
            select_child = child;
        }
    }
    return select_child;
}

KeyValue* KeyValue::get2(std::string key1, std::string key2) {
    auto a = this->get(key1);
    if (a == NULL) {
        return NULL;
    }
    auto b = a->get(key2);
    if (b == NULL) {
        return NULL;
    }
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
            // eh exception handling
            try {
                return std::stoi(std::any_cast<std::string>(this->value));
            } catch (std::exception& e) {
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

        case KeyValueType::Color:
        case KeyValueType::Pointer:
        {
            break;
        }

        default:
        {
            std::cout << "Invalid type referenced in stats parser!" << std::endl;
            return default_value;
        }
    }

    return default_value;
}

KeyValue* KeyValue::load_as_binary(std::string path) {
    std::filebuf fb;
    if (!fb.open(path, std::ios::in)) {
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
                std::cout << "Stats parser encountered invalid type!" << std::endl;
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
