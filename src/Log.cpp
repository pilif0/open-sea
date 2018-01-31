/*
 * Logging implementation
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 */
#include <open-sea/Log.h>
#include <open-sea/config.h>

#include <fstream>
#include <iomanip>
#include <iostream>

// Boost shared pointer
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

// Boost logging
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
namespace logging   = ::boost::log;
namespace keywords  = ::boost::log::keywords;
namespace src       = ::boost::log::sources;
namespace triv      = ::boost::log::trivial;
namespace sinks     = ::boost::log::sinks;
namespace expr      = ::boost::log::expressions;

namespace open_sea::log {

    void init_file_sink() {
        // Prepare the sink
        typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
        boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
        boost::shared_ptr<std::ofstream> stream = boost::make_shared<std::ofstream>("log/main.log");
        sink->locked_backend()->add_stream(stream);

#if !defined(OPEN_SEA_DEBUG)
        // Filter out log records that are not at least warnings
        sink->set_filter(
                logging::trivial::severity >= logging::trivial::warning
        );
#endif

        // Enable auto flush
        sink->locked_backend()->auto_flush(true);

        // Set the formatter
        sink->set_formatter(
                expr::stream
                        << std::hex << std::setw(8) << std::setfill('0') << expr::attr<unsigned int>("LineID")
                        << ": <" << expr::attr<severity_level>("Severity")
                        << "> " << expr::smessage
        );

        // Add the sink
        logging::core::get()->add_sink(sink);
    }

    void init_logging() {
        // Initialize the file sink
        init_file_sink();

        // Add common logging attributes
        logging::add_common_attributes();

        // Note start of logging
        severity_logger lg;
        log(lg, info, "Logging initialized");
    }

    std::ostream &operator<<(std::ostream &os, severity_level lvl) {
        static const char* strings[] = {
                "trace",
                "debug",
                "info",
                "warning"
                "error",
                "fatal"
        };

        if (static_cast<std::size_t>(lvl) < sizeof(strings) / sizeof(*strings)) {
            os << strings[lvl];
        } else {
            os << static_cast<int>(lvl);
        }

        return os;
    }

    void log(severity_logger logger, severity_level lvl, std::string message) {
        logging::record rec = logger.open_record(keywords::severity = lvl);

        if (rec) {
            logging::record_ostream stream(rec);
            stream << message;
            stream.flush();
            logger.push_record(boost::move(rec));
        }
    }
}
