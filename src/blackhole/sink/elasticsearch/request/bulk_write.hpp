#pragma once

#include <string>

#include "blackhole/sink/elasticsearch/result.hpp"
#include "blackhole/sink/elasticsearch/response/bulk_write.hpp"
#include "blackhole/sink/elasticsearch/request/method.hpp"

namespace elasticsearch {

namespace actions {

class bulk_write_t {
public:
    typedef response::bulk_write_t response_type;
    typedef result_t<response_type>::type result_type;

    static const request::method_t method_value = request::method_t::post;

    static const char* name() {
        return "bulk_write";
    }

private:
    std::string data;

public:
    bulk_write_t(std::string&& data) :
        data(std::move(data))
    {}

    std::string path() const {
        return "/";
    }

    std::string body() const {
        return data;
    }
};

} // namespace actions

} // namespace elasticsearch