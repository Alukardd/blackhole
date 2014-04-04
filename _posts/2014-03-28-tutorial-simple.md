## Deeper in configuration

This tutorial describes how to configure formatters and sinks more deeply. Also it will be shown how to use logger API directly and what's hidden under that macro's hood.

Let's start with the same steps we are done in the previous tutorial - include main header file and define severity enumeration.

{% highlight c++ %}
#include <blackhole/blackhole.hpp>

enum class level {
    debug
};

using namespace blackhole;
{% endhighlight %}

For this example we don't need multiple severity levels, so the single debug level is enough. But here the differences comes. In previous example we didn't have control for string formatter as like as stream sink. Now we want to control everything.

### Need more control

Let's specify own pattern for every log message, say, something like this:

`[%(timestamp)s] <%(severity)s>:: %(message)s`

At this point let's talk about string formatter pattern syntax. Consider pattern specified above. It has printf-like syntax with attribute names between `%(` and `)s` symbols and behaves just like you imagine it. If parser meets that attribute name, it will substitute real attribute value instead of it. For example if the log record attributes are:

    timestamp = 1396259922.000000
    severity  = 1
    message   = an intresting log message

formatter will produce the next string: `[1396259922.000000] <1>:: an intresting log message`.

To configure formatter in code we just create object of `formatter_config_t` class which has overloaded `operator[]` and provides `std::map` like interface giving ability to make hierarchical configuration.
{% highlight c++ %}
formatter_config_t formatter("string");
formatter["pattern"] = "[%(timestamp)s] <%(severity)s>:: %(message)s";
{% endhighlight %}

*Note that at this moment string formatter has no other options.*

*Why not using strongly-typed configuration objects? Mainly because one of the requirements to logger was to be able to initialize it through configuration file, string or whatever else. Internally that weakly and unsafe configuration objects are mapped to its actual formatter's or sink's settings. Intentionally that mappers must be written by developer who has written that formatter/sink. It opens possibility to have full control over which settings must be specified everything and which of them can be passed or initialized by default. Developer also controls error handling. If some required options are not provided or there is typo mistake an exception will be thrown at moment of creation logger object via repository.*

Stream sink can be configured the same way. Say, we want to write our messages into `stdout`. Stream sinks has only one option - output type, which can be either `stdout` or `stderr` at this moment. The code is:
{% highlight c++ %}
sink_config_t sink("stream");
sink["output"] = "stdout";
{% endhighlight %}

After customizing our future formatter and sink we must combine it into frontend and pass it (at last) to the logger config, not forgetting to specify logger's name. Later exactly by this name we can create new logger object with its configuration. All loggers config objects must be registered into the repository, our old friend, which we was discussed not so far. They will be there until program finished or until another logger config with the same name will be added. These steps in the code looks like:
{% highlight c++ %}
frontend_config_t frontend = { formatter, sink };
log_config_t config{ "root", { frontend } };

repository_t<level>::instance().add_config(config);
{% endhighlight %}

After that we have fully configured logger repository, which awaits until we create logger object itself (by name or by *special* name **root**, which doesn't means anything except cool The Mainest Logger name).

Usage pattern is the same as in previous example. We just create root logger object via repository and pass it through main logging macro. The function `init` just initializes logger configuration as described above.
{% highlight c++ %}
int main(int, char**) {
    init();
    verbose_logger_t<level> log = repository_t<level>::instance().root();

    BH_LOG(log, level::debug, "log message using macro API");    
    return 0;
}
{% endhighlight %}

### For those who despise macros
Sometimes it is useful to handle log object directly without using any magic macro. Shortly I'll describe it now. For more advanced documentation you should go to the logger object's reference.

Every log object has `open_record` method which accepts attributes set and returns `log_record_t` object if that attributes set passes filtering stage. If not - an invalid `log_record_t` object is returned. For that reason returned record object must be checked for validity using `record.valid()` method. For our case log record will be created anyway, because we don't have any filters registered now.

The next thing we must do after checking record's validity - is to add message attribute into the record, because our formatter requires it, otherwise we got an formatting exception, which will be eaten by the logger internally or passed to the fallback logger. Entire function is:
{% highlight c++ %}
template<typename Log>
void debug(Log& log, level lvl, const char* message) {    
    log::record_t record = log.open_record(lvl);
    if (record.valid()) {        
        record.attributes.insert({
            keyword::message() = message
        });
        log.push(std::move(record));
    }
}
{% endhighlight %}

And the usage is:
{% highlight c++ %}
debug(log, level::debug, "log message using log object directly");
{% endhighlight %}

The complete example code:
{% highlight c++ linenos%}
#include <blackhole/blackhole.hpp>

//! This example demonstrates the a bit extended blackhole logging library usage and its features.

/*! - detailed formatters and sinks configuration;
 *  - logger usage without macro.
 */

using namespace blackhole;

// As always specify severity enumeration.
enum class level {
    debug
};

// Here we are going to configure our string/stream frontend and to register it.
void init() {
    // Configure string formatter.
    // Pattern syntax behaves like usual substitution for placeholder. For example if attribute
    // named `severity` has value `2`, then pattern [%(severity)s] will produce: [2].
    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s] <%(severity)s>:: %(message)s";

    // Configure stream sink to write into stdout (also stderr can be configured).
    sink_config_t sink("stream");
    sink["output"] = "stdout";

    frontend_config_t frontend = { formatter, sink };
    log_config_t config{ "root", { frontend } };

    repository_t<level>::instance().add_config(config);
}

// Here it's an example how to create and handle log events without using any macros at all.
template<typename Log>
void debug(Log& log, level lvl, const char* message) {
    // Tries to create log record. Returns invalid record object if created log record couldn't
    // pass filtering stage.
    // For our case it will be created anyway, because we don't have any filters registered now.
    log::record_t record = log.open_record(lvl);
    if (record.valid()) {
        // Manually insert message attribute into log record attributes set using keyword API.
        // Formatter will break up if some attributes it needed don't exist where formatting
        // occures.
        record.attributes.insert({
            keyword::message() = message
        });
        log.push(std::move(record));
    }
}

int main(int, char**) {
    init();
    verbose_logger_t<level> log = repository_t<level>::instance().root();

    BH_LOG(log, level::debug, "log message using macro API");
    debug(log, level::debug, "log message using log object directly");

    return 0;
}
{% endhighlight %}

After executing this program the next log messages will be printed on the console:

    [1396271998.662163] <0>:: log message using macro API
    [1396271998.662336] <0>:: log message using log object directly

As you can see, output format is completely under our control and can be easily changed.

Next tutorials shows you:

* How to inject intermediate attributes mapping to have, for example ISO-8601 formatted date-time instead of raw timestamp;
* How to apply log record filtering;
* How to transport additional attributes (both registered and inlined) into the log record and how to attach attribute to the logger object;
* How to build more complex loggers an example of logstash frontend.