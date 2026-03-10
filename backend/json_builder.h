#pragma once
// ============================================================
//  Minimal JSON builder — no external dependency
//  Supports: string, number, bool, null, array, object
// ============================================================
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <variant>
#include <map>
#include <memory>

class Json {
public:
    enum Type { Null, Bool, Int, Double, String, Array, Object };

    Json() : type_(Null) {}
    Json(bool v) : type_(Bool), b_(v) {}
    Json(int v)  : type_(Int),  i_(v) {}
    Json(double v): type_(Double), d_(v) {}
    Json(const std::string& v) : type_(String), s_(v) {}
    Json(const char* v) : type_(String), s_(v) {}

    static Json array() { Json j; j.type_ = Array; return j; }
    static Json object() { Json j; j.type_ = Object; return j; }

    void push(const Json& v) { arr_.push_back(v); }
    void set(const std::string& k, const Json& v) { obj_[k] = v; keys_.push_back(k); }

    std::string dump() const {
        switch (type_) {
            case Null:   return "null";
            case Bool:   return b_ ? "true" : "false";
            case Int:    return std::to_string(i_);
            case Double: {
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(1) << d_;
                return ss.str();
            }
            case String: return "\"" + escape(s_) + "\"";
            case Array: {
                std::string r = "[";
                for (size_t i = 0; i < arr_.size(); i++) {
                    if (i) r += ",";
                    r += arr_[i].dump();
                }
                return r + "]";
            }
            case Object: {
                std::string r = "{";
                bool first = true;
                for (auto& k : keys_) {
                    if (!first) r += ",";
                    r += "\"" + k + "\":" + obj_.at(k).dump();
                    first = false;
                }
                return r + "}";
            }
        }
        return "null";
    }

private:
    Type type_;
    bool b_ = false;
    int  i_ = 0;
    double d_ = 0;
    std::string s_;
    std::vector<Json> arr_;
    std::map<std::string, Json> obj_;
    std::vector<std::string> keys_; // preserve insertion order

    static std::string escape(const std::string& s) {
        std::string r;
        for (char c : s) {
            if (c == '"')  r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else if (c == '\r') r += "\\r";
            else if (c == '\t') r += "\\t";
            else r += c;
        }
        return r;
    }
};
