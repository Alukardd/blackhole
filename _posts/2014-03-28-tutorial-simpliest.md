# Tutorial

## The simpliest example

Let's try Blackhole in action by writing several examples from the simpliest one, where there is no need to configure anything, to the extended, but more agile one.

Suppose we want just to write logs without configuration and boring reading tons of manuals. By default, Blackhole initializes and configures logger called **root** with single frontend, configured like this:

* Formatter: string with pattern `[%(timestamp)s] [%(severity)s]: %(message)s`;
* Sink: stream with stdout output.

First of all include main Blackhole header file which gives you access to the main library features, like repository:
{% highlight c++ %}
#include <blackhole/blackhole.hpp>
{% endhighlight %}

Although, it's not necessary, it is highly recommended to define own severity enumeration. Blackhole supports both loggers with severity separation and without it. But, seriously, what is the logger without separated severity levels? For out example let's specify four levels:
{% highlight c++ %}
enum class level {
    debug,
    info,
    warning,
    error
};
{% endhighlight %}

Blackhole supports both strongly and weakly typed enumerations for severity and it's better to use strongly-typed one.

Actually all that is left to do - is to get logger object and use it. All logger objects in Blackhole are got from `repository_t` object. It is a singleton. Consider it as a large factory, that can register possible types of formatters and sinks, configure it different ways and to create logger objects using registered configurations. As I mentioned before, there is already registered configuration for **root** logger, and we are going to use it:
{% highlight c++ %}
int main(int, char**) {
    verbose_logger_t<level> log = repository_t::instance().root<level>();

    BH_LOG(log, level::debug,   "[%d] %s - done", 0, "debug");
    BH_LOG(log, level::info,    "[%d] %s - done", 1, "info");
    BH_LOG(log, level::warning, "[%d] %s - done", 2, "warning");
    BH_LOG(log, level::error,   "[%d] %s - done", 3, "error");
    return 0;
}
{% endhighlight %}

Having the logger object it can be passed to the main logging macro which has the next signature:
{% highlight c++ %}
BH_LOG(verbose_logger_t<Severity> logger, Severity severity, const char* message, Args...);
{% endhighlight %}
where the `Severity` - severity enumeration (`level` in our case) and `Args...` - are message's placeholder arguments for pretty formatting like `printf` style.

*Note that main logging macro is not the only interface the Blackhole offers to write logs. It is possible to handle your log records completely without using macros, which will be described later.*

The complete example code:

{% highlight c++ linenos%}
#include <blackhole/blackhole.hpp>

//! This example demonstrates the simpliest blackhole logging library usage.

/*! All that we need - is to define severity enum with preferred log levels.
 *
 *  Logger object `verbose_logger_t` is provided by `repository_t` class.
 */

using namespace blackhole;

enum class level {
    debug,
    info,
    warning,
    error
};

int main(int, char**) {
    verbose_logger_t<level> log = repository_t::instance().root<level>();

    BH_LOG(log, level::debug,   "[%d] %s - done", 0, "debug");
    BH_LOG(log, level::info,    "[%d] %s - done", 1, "info");
    BH_LOG(log, level::warning, "[%d] %s - done", 2, "warning");
    BH_LOG(log, level::error,   "[%d] %s - done", 3, "error");
    return 0;
}
{% endhighlight %}

As a result of this code, the next log messages will be printed on the console:

    [1396259922.290809] [0]: [0] debug - done
    [1396259922.306820] [1]: [1] info - done
    [1396259922.306926] [2]: [2] warning - done
    [1396259922.307020] [3]: [3] error - done

*Note that even timestamp attribute prints as raw `timeval` value, it is possible to easily format timesamp as you like using `strftime` specification. Severity levels can be also formatted. In general any attribute can be formatted by specifying intermediate attribute formatter which will be called every formatting cycle. This will be described later.*

See, the output is almost as if you are using standard `std::cout` streaming, but Blackhole offers a several advantages:

* **attributes**, which has not been covered in this example, but it is the powerful feature, that allows you to transport any other information you like with log records;
* log records filtering can be applied, which will be shown later;
* configurable logger's thread-safety, depending on your use-case.
