#pragma once

#include "defines.h"

#include "layer.h"
#include <vector>
#include <algorithm>

class IC_API layer_stack
{
public:
    layer_stack();
    ~layer_stack();

    void push_layer(layer* layer);
    void push_overlay(layer* overlay);
    void pop_layer(layer* layer);
    void pop_overlay(layer* overlay);

    // TODO: use darray here instead
    std::vector<layer*>::iterator begin() { return m_layers.begin(); }
    std::vector<layer*>::iterator end() { return m_layers.end(); }

private:

    // TODO: use darray here instead
    std::vector<layer*> m_layers;
    std::vector<layer*>::iterator m_layer_insert;
};