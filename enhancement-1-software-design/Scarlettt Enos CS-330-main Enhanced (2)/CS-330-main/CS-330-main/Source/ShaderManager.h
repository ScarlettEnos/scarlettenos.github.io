#pragma once

#include <string>

class ShaderManager
{
public:
    void setMat4Value(const std::string&, auto);
    void setVec3Value(const std::string&, float, float, float);
    void setVec3Value(const std::string&, auto);
    void setVec4Value(const std::string&, auto);
    void setVec2Value(const std::string&, auto);
    void setFloatValue(const std::string&, float);
    void setIntValue(const std::string&, int);
    void setBoolValue(const std::string&, bool);
    void setSampler2DValue(const std::string&, int);
};