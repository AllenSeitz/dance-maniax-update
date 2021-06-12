// ppp_read.cpp reads a .BIN file, which is extracted from Para Para Paradise
// source file created 6/2/2016 by Catastrophe
// NOTE: source file was created in a hurry as a proof of concept

#ifndef _PPP_READ_H_
#define _PPP_READ_H_

#include <vector>

#include "../headers/common.h"

bool doesExistPPP(int songID);
//
//

int readPPP(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType);
//
//

#endif