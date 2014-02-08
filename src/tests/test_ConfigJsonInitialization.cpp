#include "Mocks.hpp"

namespace blackhole {

namespace aux {

namespace conversion {

enum class integral {
    uint16,
    uint32,
    uint64,
    int16,
    int32,
    int64
};

} // namespace conversion

} // namespace aux

static std::map<std::string, aux::conversion::integral> convertion = {
    { "sink/files/rotation/backups", aux::conversion::integral::uint16 },
    { "sink/files/rotation/size", aux::conversion::integral::uint64 }
};

template<class Builder, typename IntegralType>
inline void convert(Builder& builder, const std::string& n, const std::string& full_name, IntegralType value) {
    using namespace aux::conversion;

    auto it = convertion.find(full_name + "/" + n);
    if (it == convertion.end()) {
        builder[n] = static_cast<int>(value);
        return;
    }

    const integral ic = it->second;
    switch (ic) {
    case integral::uint16:
        builder[n] = static_cast<std::uint16_t>(value);
        break;
    case integral::uint32:
        builder[n] = static_cast<std::uint32_t>(value);
        break;
    case integral::uint64:
        builder[n] = static_cast<std::uint64_t>(value);
        break;
    case integral::int16:
        builder[n] = static_cast<std::int16_t>(value);
        break;
    case integral::int32:
        builder[n] = static_cast<std::int32_t>(value);
        break;
    case integral::int64:
        builder[n] = static_cast<std::int64_t>(value);
        break;
    default:
        BOOST_ASSERT(false);
    }
}

template<typename T>
static void fill(T& builder, const rapidjson::Value& node, const std::string& full_name) {
    for (auto it = node.MemberBegin(); it != node.MemberEnd(); ++it) {
        const std::string& name = it->name.GetString();
        const rapidjson::Value& value = it->value;

        if (value.IsObject()) {
            auto nested_builder = builder[name];
            fill(nested_builder, value, full_name + "/" + name);
        } else {
            if (name == "type") {
                continue;
            }

            if (value.IsBool()) {
                builder[name] = value.GetBool();
            } else if (value.IsInt()) {
                convert(builder, name, full_name, value.GetInt());
            } else if (value.IsUint()) {
                convert(builder, name, full_name, value.GetUint());
            } else if (value.IsInt64()) {
                convert(builder, name, full_name, value.GetInt64());
            } else if (value.IsUint64()) {
                convert(builder, name, full_name, value.GetUint64());
            } else if (value.IsDouble()) {
                builder[name] = value.GetDouble();
            } else if (value.IsString()) {
                builder[name] = std::string(value.GetString());
            }
        }
    }
}

template<class T>
class parser_t;

template<>
class parser_t<repository::config::base_t> {
public:
    template<typename T>
    static T parse(const std::string& name, const rapidjson::Value& value) {
        const std::string& type = value["type"].GetString();
        T config(type);
        fill(config, value, name + "/" + type);
        return config;
    }
};

template<>
class parser_t<formatter_config_t> {
public:
    static formatter_config_t parse(const rapidjson::Value& value) {
        return parser_t<repository::config::base_t>::parse<formatter_config_t>("formatter", value);
    }
};

template<>
class parser_t<sink_config_t> {
public:
    static sink_config_t parse(const rapidjson::Value& value) {
        return parser_t<repository::config::base_t>::parse<sink_config_t>("sink", value);
    }
};

template<>
class parser_t<frontend_config_t> {
public:
    static frontend_config_t parse(const rapidjson::Value& value) {
        if (!(value.HasMember("formatter") && value.HasMember("sink"))) {
            throw blackhole::error_t("both 'formatter' and 'sink' section must be specified");
        }

        const formatter_config_t& formatter = parser_t<formatter_config_t>::parse(value["formatter"]);
        const sink_config_t& sink = parser_t<sink_config_t>::parse(value["sink"]);
        return frontend_config_t { formatter, sink };
    }
};

template<>
class parser_t<log_config_t> {
public:
    static log_config_t parse(const std::string& name, const rapidjson::Value& value) {
        log_config_t config;
        config.name = name;
        for (auto it = value.Begin(); it != value.End(); ++it) {
            const frontend_config_t& frontend = parser_t<frontend_config_t>::parse(*it);
            config.frontends.push_back(frontend);
        }

        return config;
    }
};

template<>
class parser_t<std::vector<log_config_t>> {
public:
    static std::vector<log_config_t> parse(const rapidjson::Value& root) {
        std::vector<log_config_t> configs;
        for (auto it = root.MemberBegin(); it != root.MemberEnd(); ++it) {
            const log_config_t& config = parser_t<log_config_t>::parse(it->name.GetString(), it->value);
            configs.push_back(config);
        }

        return configs;
    }
};

}

class parser_test_case_t : public Test {
protected:
    std::vector<log_config_t> configs;

    void SetUp() {
        std::ifstream stream("config/valid.json");
        std::string valid((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        rapidjson::Document doc;
        doc.Parse<0>(valid.c_str());
        ASSERT_FALSE(doc.HasParseError());
        configs = parser_t<std::vector<log_config_t>>::parse(doc);
    }
};

TEST_F(parser_test_case_t, NameParsing) {
    ASSERT_EQ(1, configs.size());
    EXPECT_EQ("root", configs.at(0).name);
}

TEST_F(parser_test_case_t, CheckFormatterConfigAfterParsing) {
    ASSERT_EQ(1, configs.size());
    ASSERT_EQ(1, configs.at(0).frontends.size());

    const formatter_config_t& fmt = configs.at(0).frontends.at(0).formatter;
    EXPECT_EQ("string", fmt.type);
    EXPECT_EQ("[%(level)s]: %(message)s", fmt["pattern"].to<std::string>());
}

TEST_F(parser_test_case_t, CheckSinkConfigAfterParsing) {
    ASSERT_EQ(1, configs.size());
    ASSERT_EQ(1, configs.at(0).frontends.size());

    const sink_config_t& sink = configs.at(0).frontends.at(0).sink;
    EXPECT_EQ("files", sink.type);
    EXPECT_EQ("test.log", sink["path"].to<std::string>());
    EXPECT_EQ("test.log.%N", sink["rotation"]["pattern"].to<std::string>());
    EXPECT_EQ(5, sink["rotation"]["backups"].to<std::uint16_t>());
    EXPECT_EQ(1000000, sink["rotation"]["size"].to<std::uint64_t>());
}

TEST(parser_t, ThrowsExceptionIfFormatterSectionIsAbsent) {
    std::ifstream stream("config/no-valid-absent-formatter.json");
    std::string nonvalid((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    doc.Parse<0>(nonvalid.c_str());
    ASSERT_FALSE(doc.HasParseError());
    EXPECT_THROW(parser_t<std::vector<log_config_t>>::parse(doc), blackhole::error_t);
}

TEST(parser_t, ThrowsExceptionIfSinkSectionIsAbsent) {
    std::ifstream stream("config/no-valid-absent-sink.json");
    std::string nonvalid((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    doc.Parse<0>(nonvalid.c_str());
    ASSERT_FALSE(doc.HasParseError());
    EXPECT_THROW(parser_t<std::vector<log_config_t>>::parse(doc), blackhole::error_t);
}

TEST(parser_t, MultipleFrontends) {
    std::ifstream stream("config/valid-multiple-frontends.json");
    std::string valid((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    doc.Parse<0>(valid.c_str());
    ASSERT_FALSE(doc.HasParseError());

    const std::vector<log_config_t>& configs = parser_t<std::vector<log_config_t>>::parse(doc);
    ASSERT_EQ(1, configs.size());
    EXPECT_EQ("root", configs.at(0).name);
    ASSERT_EQ(2, configs.at(0).frontends.size());

    const frontend_config_t& front1 = configs.at(0).frontends.at(0);
    ASSERT_EQ("string", front1.formatter.type);
    ASSERT_EQ("files", front1.sink.type);

    const frontend_config_t& front2 = configs.at(0).frontends.at(1);
    ASSERT_EQ("json", front2.formatter.type);
    ASSERT_EQ("tcp", front2.sink.type);

}