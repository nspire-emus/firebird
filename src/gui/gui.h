/* Declarations for gui.c */

void gui_initialize();
void gui_redraw() __asm__("gui_redraw");
void gui_set_tittle(char *buf)  __asm__("gui_set_tittle");
void get_messages();
void *gui_save_state(size_t *size);
void gui_reload_state(void *state);
