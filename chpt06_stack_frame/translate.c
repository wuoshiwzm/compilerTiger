#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "frame.h"
#include "escape.h"
#include "translate.h"

// F_access + Tr_level => Tr_access
struct Tr_access_ {Tr_level level; F_access access}
