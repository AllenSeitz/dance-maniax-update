// dwi_read.h reads a .DWI file, for older DMX simfiles
// source file created 1/18/12 by Catastrophe

#ifndef _DWI_READ_H_
#define _DWI_READ_H_

#include <vector>

#include "common.h"

int readDWI(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType);
// precondition: it is safe to overwrite chart and holds, and songID and chartType exist
// postcondition: will write new values for chart and holds, or leave them untouched if there is an error
// returns: the maximum score in dance points for this chart

int readDWI2P(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType);
// precondition: it is safe to overwrite chart and holds, and songID and chartType exist
// postcondition: calls readDWI, then moves columns 0-3 to 4-7
// returns: the maximum score in dance points for this chart

int readDWICenter(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType);
// precondition: it is safe to overwrite chart and holds, and songID and chartType exist
// postcondition: calls readDWI, then moves columns 0-3 to 4-7
// returns: the maximum score in dance points for this chart

#endif