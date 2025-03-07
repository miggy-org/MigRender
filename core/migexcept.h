#pragma once

#include <exception>
#include <string>
#include "defines.h"

#ifndef _NOEXCEPT
#define _NOEXCEPT noexcept
#endif  // _NOEXCEPT

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// mig_exception - base class for all MigRender exceptions
//-----------------------------------------------------------------------------

class mig_exception : public std::exception
{
private:
    std::string _msg;

public:
    using _Mybase = std::exception;

    explicit mig_exception(const std::string& _Message) : _Mybase()
    {
        _msg = _Message;
    }

    virtual const char* what() const _NOEXCEPT
    {
        return _msg.c_str();
    }
};

//-----------------------------------------------------------------------------
// derivations
//-----------------------------------------------------------------------------

class fileio_exception : public mig_exception
{
public:
    using _Mybase = mig_exception;

    explicit fileio_exception(const std::string& _Message) : _Mybase(_Message) {}
};

class model_exception : public mig_exception
{
public:
    using _Mybase = mig_exception;

    explicit model_exception(const std::string& _Message) : _Mybase(_Message) {}
};

class anim_exception : public mig_exception
{
public:
    using _Mybase = mig_exception;

    explicit anim_exception(const std::string& _Message) : _Mybase(_Message) {}
};

class render_exception : public mig_exception
{
public:
    using _Mybase = mig_exception;

    explicit render_exception(const std::string& _Message) : _Mybase(_Message) {}
};

class prerender_exception : public mig_exception
{
public:
    using _Mybase = mig_exception;

    explicit prerender_exception(const std::string& _Message) : _Mybase(_Message) {}
};

class postrender_exception : public mig_exception
{
public:
    using _Mybase = mig_exception;

    explicit postrender_exception(const std::string& _Message) : _Mybase(_Message) {}
};

_MIGRENDER_END
