#pragma once

#define DECLARE_DELEGATE(Name, ...) TMultiCastDelegate<__VA_ARGS__> Name

template<typename... Args>
class TMultiCastDelegate
{
public:

private:


};