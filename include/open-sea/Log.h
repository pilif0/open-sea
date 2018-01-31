/*
 * Logging header
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 */
#ifndef OPEN_SEA_LOG_H
#define OPEN_SEA_LOG_H

#include <boost/log/sources/severity_logger.hpp>
#include <string>
#include <iosfwd>

namespace open_sea::log {
    // Initialize logging with a text sink to "log/main.log"
    void init_logging();

    // Initialize just the file sink with path "log/main.log"
    void init_file_sink();

    // Severity levels
    enum severity_level
    {
        trace,
        debug,
        info,
        warning,
        error,
        fatal
    };

    // Text representation of severity levels for streams
    std::ostream& operator<<(std::ostream &os, severity_level lvl);

    // Alias for the severity logger type
    typedef boost::log::sources::severity_logger<severity_level> severity_logger;

    // Log a message through a logger with a certain severity
    void log(severity_logger logger, severity_level lvl, std::string message);
}

#endif //OPEN_SEA_LOG_H
