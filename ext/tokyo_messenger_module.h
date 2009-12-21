#ifndef RUBY_TOKYOMESSENGER_MODULE
#define RUBY_TOKYOMESSENGER_MODULE

#include "tokyo_messenger.h"

extern TCRDB *mTokyoMessenger_getdb(VALUE vself);
extern void mTokyoMessenger_exception(VALUE vself);
void init_mod();

#endif
