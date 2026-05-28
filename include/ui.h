#ifndef AGBSEQ_UI_H
#define AGBSEQ_UI_H

#include <tonc.h>

#include "editor.h"
#include "sequencer.h"

void ui_init(void);
void ui_invalidate(void);
void ui_render(const Sequencer *sequencer, const Editor *editor);
void ui_commit(void);

#endif
