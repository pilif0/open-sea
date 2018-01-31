/*
 * Test file
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 */

#include <sstream>

#include <open-sea/Test.h>
#include <open-sea/config.h>

std::string get_test_string(){
    std::ostringstream stringStream;
    stringStream << "Test - Open Sea (v" << OPEN_SEA_VERSION_FULL << ")";
    return stringStream.str();
}
