// xsq_read.h reads a .XSQ file, for older, original DMX chart data
// source file created 9/21/13 by Catastrophe

#ifndef _XSQ_READ_H_
#define _XSQ_READ_H_

#include <vector>

#include "../headers/common.h"

bool doesExistXSQ(int songID);
// precondition: songID is in the song database and is three digits or less
// postconditionL returns true if xxx.xsq exists on disk, false otherwise

int readXSQ(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType);
// precondition: it is safe to overwrite chart and holds, and songID and chartType exist
// postcondition: will write new values for chart and holds, or leave them untouched if there is an error
// returns: the maximum score in dance points for this chart

int readXSQ2P(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType);
// precondition: it is safe to overwrite chart and holds, and songID and chartType exist
// postcondition: calls readDWI, then moves columns 0-3 to 4-7
// returns: the maximum score in dance points for this chart

int readXSQCenter(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType);
// precondition: it is safe to overwrite chart and holds, and songID and chartType exist
// postcondition: calls readDWI, then moves columns 0-3 to 4-7
// returns: the maximum score in dance points for this chart


#endif