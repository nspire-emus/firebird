/* Declarations for gui.c */

void gui_initialize();
void get_messages();
extern char target_folder[256];
void *gui_save_state(size_t *size);
void gui_reload_state(void *state);
