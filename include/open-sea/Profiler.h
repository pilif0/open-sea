/** \file Profiler.h
 * Runtime profiler
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_PROFILER_H
#define OPEN_SEA_PROFILER_H

#include <open-sea/Track.h>

#include <string>
#include <vector>
#include <memory>
#include <ostream>

namespace open_sea::profiler {
    // Notes:
    //  - Result of profiling is a tree of runtime information nodes for code blocks.
    //  - Time in a node is start time while executing to save space (not needed after duration is known --> can be replaced).

    /** \struct Info
     * \brief Information in each element of the frame track
     * Information in each element of the frame track.
     * During execution, \c time is the start time of the execution.
     * After execution, \c time is the duration of the execution.
     */
    struct Info {
        //! Label
        std::string label;
        //! Execution duration in second (only when finished, start time while executing)
        double time;

        explicit Info(const std::string &label);
    };
    std::ostream& operator<<(std::ostream &os, const Info &info);

    //! Frame track type
    typedef data::Track<Info> track;

    void start();
    void finish();

    void push(const std::string &label);
    void pop();

    std::shared_ptr<track> get_last();
    std::shared_ptr<track> get_maximum();
    void clear_maximum();

    void show_text();
    void show_graphical();
}

#endif //OPEN_SEA_PROFILER_H
