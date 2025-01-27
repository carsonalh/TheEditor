#pragma once

namespace ed
{

template<typename T>
class D2DRef
{
public:
    D2DRef(T* p = nullptr)
        : m_Resource(p)
    {
        if (p)
        {
            p->AddRef();
        }
    }

    D2DRef(const D2DRef& other)
        : m_Resource(other.m_Resource)
    {
        if (m_Resource)
        {
            m_Resource->AddRef();
        }
    }

    D2DRef(D2DRef&& other)
        : m_Resource(other.m_Resource)
    {
        other.m_Resource = nullptr;
    }

    ~D2DRef()
    {
        if (m_Resource)
        {
            m_Resource->Release();
            m_Resource = nullptr;
        }
    }

    D2DRef& operator=(const D2DRef& other)
    {
        this->~D2DRef();
        m_Resource = other.m_Resource;
        m_Resource->AddRef();
        return *this;
    }

    D2DRef& operator=(D2DRef&& other)
    {
        this->~D2DRef();
        m_Resource = other.m_Resource;
        return *this;
    }

    T* operator->() const
    {
        return m_Resource;
    }

    operator T*() const
    {
        return m_Resource;
    }

    bool empty() const
    {
        return m_Resource == nullptr;
    }
private:
    T* m_Resource;
};

} // namespace ed
