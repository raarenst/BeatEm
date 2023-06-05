/******************************************************************
 * Copyright (C) 2002-2007 Ra-Tek. All rights reserved.
 *               By Roger Aarenstrup
 *             www.rogeraarenstrup.com
 ******************************************************************
 */
#ifndef _DISPLAY_H
#define _DISPLAY_H

/* Debug modes
 */
#define DISPLAY_NO_DEBUG_INFO    ((int)1)
#define DISPLAY_SHOW_DEBUG_INFO  ((int)2)

extern void display_init(int debug_mode);
extern void display_register(void);
extern void display_source_added(char *application,
                                 char *variable_name,
                                 int variable_size,
                                 int client);
extern void display_remove_source(int client);
extern void display_sink_added(char *application,
                               char *variable_name,
                               int variable_size,
                               int client);
extern void display_remove_sink(int client);
extern void display_refuse_sink(int client);
extern void display_starting_simulation(void);
extern void display_server_stopped(void);
extern void display_list_complete(void);

/* Error displays
 */
extern void display_non_fatal_error(char *disp_msg);
extern void display_fatal_error(char *disp_msg);

/* Debug displays
 */
extern void display_dbg_before_select(void);
extern void display_dbg_after_select(int nr_eager);        
extern void display_dbg_info(char *info);

#endif /* _DISPLAY_H */
