/** \file Log.h
 * Logging module
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_LOG_H
#define OPEN_SEA_LOG_H

#include <boost/log/sources/severity_logger.hpp>
#include <string>
#include <iosfwd>

namespace open_sea::log {
    //! Path to the log file
    constexpr const char* file_path = "log/main.log";
    //! Format string for the datetime
    constexpr const char* datetime_format = "%H:%M:%S.%f";

    /**
     * \brief Severity levels used when logging
     */
    enum severity_level
    {
        trace,
        debug,
        info,
        warning,
        error,
        fatal
    };

    //! Alias for the severity logger type
    typedef boost::log::sources::severity_logger<severity_level> severity_logger;

    void init_logging();
    bool add_file_sink();
    void add_console_sink();
    void log(severity_logger logger, severity_level lvl, std::string message);

    std::ostream& operator<<(std::ostream &os, severity_level lvl);
}

#endif //OPEN_SEA_LOG_H
