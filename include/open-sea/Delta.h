/** \file Delta.h
 * Delta time module
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_DELTA_H
#define OPEN_SEA_DELTA_H

//! Timekeeping functions
namespace open_sea::time {
    /**
     * \addtogroup Time
     * \brief Timekeeping functions
     *
     * Functions related to time and measuring its passage.
     * Mainly focused on calculating the frame delat time.
     *
     * @{
     */

    void start_delta();
    void update_delta();

    double get_delta();
    double get_fps_immediate();
    double get_fps_average();

    void debug_window(bool *open);

    /**
     * @}
     */
}

#endif //OPEN_SEA_DELTA_H
