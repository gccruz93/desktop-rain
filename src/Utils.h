#pragma once

template <typename T>
struct ComDeleter
{
    void operator()(T *ptr) const
    {
        if (ptr)
            ptr->Release();
    }
};
template <typename T>
using ComPtr = std::unique_ptr<T, ComDeleter<T>>;
