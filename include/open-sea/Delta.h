/** \file Delta.h
 * Delta time module
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_DELTA_H
#define OPEN_SEA_DELTA_H

namespace open_sea::time {
    void start_delta();
    void update_delta();

    double get_delta();
    double get_FPS_immediate();
    double get_FPS_average();

    void debug_widget();
}

#endif //OPEN_SEA_DELTA_H
