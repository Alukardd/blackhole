# Main concepts

  * [Architecture](#architecture)
  * [Attributes](#attributes)
  * [Filtering](#filtering)

##Architecture
Blackhole was originally conceived as project with clearly separated entities, or modules. Mainly this decision was made because the project can be easily tested and expanded if it consists of independent entities than with the coupled ones.

Let's start our description with the fact that we represent Blackhole logger as the production line on which some cargos are transported and some work is done with them. This can be represented schematically as follows:

![architecture](images/architecture.png)

The main transport unit in our case - is log event.

### Frontend

Frontend consists of a simple pair of formatter-sink, and it is responsible for communication between them. If the standard contract between formatter and sink is not enough, you can always implement your own specialization of frontent's template class and use it. But it will be described later. For now let's look at the each frontend's blocks: formatters and sinks.

#### Formatter

Formatter module is responsible for translating log event into the string. Why string? Because for almost every case that you can imagine exactly string representation of log event is used.

* Writing log into files on the HDD? Definitely string!
* Syslog? Sure.
* NT events, sockets, streams? String again (event obfuscated or strongly formatted like jsoned, msgpacked or protobuffed string).

Thus, formatter's objects always produce strings.

Blackhole has some predefined formatters. You can read theirs description in the ["Formatters"](formatter.md) part of documentation.

After formatting we have log event's string representation, and (we are almost done with it) the last thing we can do with that string - is to send it into the final destination appointment.

#### Sink

Sink module is responsible for sending formatted message into its final destination. It can be file, socket, syslog or whatever else. You can find descriptions of implemented sinks in the ["Sinks"](sinks.md) part of documentation.

Here lifetime of log event ends.

### Repository.

You don't see it on the picture but it is a very important part of workflow. It is a singleton which is responsible for the registration of all possible types of required logger frontends, adding typical configurations and creating logger objects.

## Attributes

Internally Blackhole event is an attributes container. Event is considered valid if there is at least one attribute in it.

To describe some event comprehensively many info items should be collected. This items Blackhole treats as attributes of event. For example let's log an HTTP-request. We can get someting like this

```
[2014-03-30 15:30:00.000000] [INFO]: GET '/index/type/id' failed with 404: document not found
```

How you will process such logs? Of course you may use `grep` of similar instruments, but it doesn't efficient and also does not guarantee a positive result if the format changes with the time. Much better is to put your events into special typed document-oriented database, like [ElasticSearch](http://www.elasticsearch.org/) which provides out of the box almost instantly search and analyze. But to use this posibilities we should split event for items. For above log string we can get the next ones:

* Timestamp: 2014-03-30 15:30:00.000000
* Severity level: INFO
* HTTP method: GET
* URI: /index/type/id
* HTTP status code: 404
* HTTP error reason: document not found

Even in you don't want to use such instrument like Elasticsearch this attributes gives fully representation of event occurred. Such structure can be easily analyzed and indexed for further search.

So, in Blackhole everything that relates to log event - is an attribute actually. Attribute itself is represented by value-scale pair, where value has variant type (which isn't sound very surprisingly for C++), and the scale determines attribute's mapping to one of the following groups:

* Local: user's attributes;
* Event: attributes, that doesn't specified by user, like message or timestamp;
* Global: logger's object specific attributes, which can be attached to the logger object and which are transported with it;
* Thread: thread-specific attributes, like thread id;
* Universe: program global attributes, like process id.

Also every attribute has its name, which in summary gives a complete picture of the log event.

## Filtering

After sending attributes into logger object, they are pre-filtered. Since we have nothing but attributes in the log event, filtering performs on them. For example we can make filter, that only passes log events with severity level more than 3 (INFO) or by presence of some attribute (like urgent tag). Blackhole allows you to configure [filtering](messages-filtering.md) it by specifying some functional object which accepts attributes set and returns boolean value, determining if the event pass filtering or not.

Filtered events are sent into the main part of logger - frontends, which can be one or more.