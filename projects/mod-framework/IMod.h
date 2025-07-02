#pragma once

class IMod {
public:
    virtual ~IMod() = default;
    virtual const char* GetName() const = 0;
    virtual void OnLoad() = 0;
    virtual void OnUnload() = 0;
};
