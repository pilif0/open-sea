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

//! Runtime profiler
namespace open_sea::profiler {
    /**
     * \addtogroup Profiler
     * \brief Runtime profiler
     *
     * Runtime profiler using the \see open_sea::data::Track "Track" data structure.
     * Has text and graphical display for ImGui.
     * All functions (except \c start()) can be safely called (with no effect) even when profiling has not been started.
     * Stores the last completed frame and the maximum overall duration frame.
     *
     * @{
     */

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

    /**
     * @}
     */
}

#endif //OPEN_SEA_PROFILER_H
