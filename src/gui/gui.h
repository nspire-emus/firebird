/* Declarations for gui.c */

void gui_initialize();
void gui_redraw();
void gui_set_tittle(char *buf);
void get_messages();
void *gui_save_state(size_t *size);
void gui_reload_state(void *state);
