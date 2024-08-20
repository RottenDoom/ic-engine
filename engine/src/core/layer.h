#pragma once

#include "defines.h"
#include <string>

class IC_API layer
{
public:
    layer(const std::string& name = "layer");
    virtual ~layer();

    virtual void on_attach() {}
    virtual void on_detach() {}
    virtual void on_update() {}
    virtual void on_event() {}

    inline const std::string get_name() const { return m_DebugName; }

private:
    std::string m_DebugName;
};