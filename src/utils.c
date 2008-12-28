/*
 * GPL Notice:
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Library General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Name:
 *
 *    alltray
 *
 *
 * Copyright:
 * 
 *    Jochen Baier, 2004 (email@Jochen-Baier.de)
 *
 *
 * Based on:
 *
 *    steal-xwin.c (acano@systec.com)
 *    xswallow (Caolan McNamara ?)
 *    kdocker (Girish Ramakrishnan)
 *    libwnck (Havoc Pennington <hp@redhat.com>)
 *    eggtrayicon (Anders Carlsson <andersca@gnu.org>)
 *    dsimple.c ("The Open Group")
 *    .....lot more, THANX !!!
 *    
*/

#include "gtray.h"
#include "utils.h"
#include "trayicon.h"
#include "inlinepixbufs.h"

char *event_names[] = {
   "",
   "",
   "KeyPress",
   "KeyRelease",
   "ButtonPress",
   "ButtonRelease",
   "MotionNotify",
   "EnterNotify",
   "LeaveNotify",
   "FocusIn",
   "FocusOut",
   "KeymapNotify",
   "Expose",
   "GraphicsExpose",
   "NoExpose",
   "VisibilityNotify",
   "CreateNotify",
   "DestroyNotify",
   "UnmapNotify",
   "MapNotify",
   "MapRequest",
   "ReparentNotify",
   "ConfigureNotify",
   "ConfigureRequest",
   "GravityNotify",
   "ResizeRequest",
   "CirculateNotify",
   "CirculateRequest",
   "PropertyNotify",
   "SelectionClear",
   "SelectionRequest",
   "SelectionNotify",
   "ColormapNotify",
   "ClientMessage",
   "MappingNotify"
};

gboolean assert_window (Window window)
{

  XWindowAttributes wa;
  
  gdk_error_trap_push();
  XGetWindowAttributes (GDK_DISPLAY(), window, &wa);
  
  if (gdk_error_trap_pop())
    return FALSE;
  
  return TRUE;
}

void atom_init (void)
{

  wm_name_atom = XInternAtom(GDK_DISPLAY(), "WM_NAME", False);
  wm_icon_atom = XInternAtom(GDK_DISPLAY(), "WM_ICON", False);
  net_wm_icon = XInternAtom(GDK_DISPLAY(), "_NET_WM_ICON", False);
  net_close_window=XInternAtom(GDK_DISPLAY(), "_NET_CLOSE_WINDOW", False);
  net_current_desktop = XInternAtom(GDK_DISPLAY(), "_NET_CURRENT_DESKTOP", False);
  wm_delete_window = XInternAtom (GDK_DISPLAY(), "WM_DELETE_WINDOW", False);
  wm_take_focus = XInternAtom (GDK_DISPLAY(), "WM_TAKE_FOCUS", False);
  net_wm_ping = XInternAtom (GDK_DISPLAY(), "_NET_WM_PING", False);
  net_wm_state_skip_pager= XInternAtom (GDK_DISPLAY(), "_NET_WM_STATE_SKIP_PAGER", False);
  net_wm_state_skip_taskbar=XInternAtom (GDK_DISPLAY(), "_NET_WM_STATE_SKIP_TASKBAR", False);
  net_wm_state = XInternAtom (GDK_DISPLAY(), "_NET_WM_STATE", False);
  wm_state = XInternAtom (GDK_DISPLAY(), "WM_STATE", False);
  net_wm_state_sticky= XInternAtom (GDK_DISPLAY(), "_NET_WM_STATE_STICKY", False);
  net_wm_desktop= XInternAtom (GDK_DISPLAY(), "_NET_WM_DESKTOP", False);
  net_active_window= XInternAtom(GDK_DISPLAY(), "_NET_ACTIVE_WINDOW", False);

  char temp[50];
  Screen *screen = XDefaultScreenOfDisplay(GDK_DISPLAY());

  sprintf(temp, "_NET_SYSTEM_TRAY_S%i", XScreenNumberOfScreen(screen));
  selection_atom = XInternAtom(GDK_DISPLAY(), temp, False);

  manager_atom = XInternAtom (GDK_DISPLAY(), "MANAGER", False);
  system_tray_opcode_atom = XInternAtom (GDK_DISPLAY(), 
      "_NET_SYSTEM_TRAY_OPCODE", False);

}

gboolean gtk_sleep_function (gpointer data)
{
  gtk_main_quit ();
  return FALSE;
}

void gtk_sleep (gint sec) 
{

  g_timeout_add (1000* (guint) sec, gtk_sleep_function, NULL);
  gtk_main();
  
}

Window get_active_window (void)
{
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *data;
  int result;
  Window win=None;
  
  type = None;
  result = XGetWindowProperty (GDK_DISPLAY(), GDK_ROOT_WINDOW(),
         net_active_window,
         0,  0x7fffffff, False,XA_WINDOW, &type, &format, &nitems,
         &bytes_after, &data); 
 
  if ( result != Success)
     return None;
        
  if (type != XA_WINDOW) {
      XFree (data);
      return None;
  }
  
  if (data) {
    win= *((Window *) data);
    if (debug) printf ("active window id %d\n", (int) win);
    XFree (data);
  }
  
  return win;
}

void skip_pager (Window window)
{
  
  XEvent xev;
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.window = window;
  xev.xclient.message_type = net_wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 1;
  xev.xclient.data.l[1] = net_wm_state_skip_pager;
  xev.xclient.data.l[2] = 0;
  xev.xclient.data.l[3] = 0;
  xev.xclient.data.l[4] = 0;
  
  XSendEvent (GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
     SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

void sticky (Window window)
{
  
  XEvent xev;
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.window = window;
  xev.xclient.message_type = net_wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 1;
  xev.xclient.data.l[1] = net_wm_state_sticky;
  xev.xclient.data.l[2] = 0;
  xev.xclient.data.l[3] = 0;
  xev.xclient.data.l[4] = 0;
  
  XSendEvent (GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
     SubstructureRedirectMask | SubstructureNotifyMask, &xev);

  /* Request desktop 0xFFFFFFFF */
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.window = window;
  xev.xclient.display = GDK_DISPLAY();
  xev.xclient.message_type =net_wm_desktop;
  xev.xclient.format = 32;
  
  xev.xclient.data.l[0] = 0xFFFFFFFF;
  xev.xclient.data.l[1] = 0;
  xev.xclient.data.l[2] = 0;
  xev.xclient.data.l[3] = 0;
  xev.xclient.data.l[4] = 0;
  
  XSendEvent (GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
            SubstructureRedirectMask | SubstructureNotifyMask, &xev);

}

static GdkFilterReturn parent_filter_map (GdkXEvent *xevent, 
    GdkEvent *event, gpointer user_data)
{
  XEvent *xev = (XEvent *)xevent;
   
  if (xev->xany.type == MapNotify) {
  
   if (debug) printf ("map notify\n");
       
   gtk_main_quit();
 
  }
  
  return GDK_FILTER_CONTINUE;
}

void show_hide_window (win_struct *win, gint force_state)
{

  gboolean show=TRUE;
  static gboolean first_click=TRUE;
  gint x, y;

  if (first_click) {

    if (debug) printf ("first click\n");
   
    gdk_window_add_filter(win->parent_gdk, parent_filter_map, (gpointer) win);
   
    XMapWindow (win->display, win->parent_xlib);
   
    gtk_main ();
    gdk_window_remove_filter(win->parent_gdk, parent_filter_map, (gpointer) win);
         
   sticky (win->parent_xlib);
   skip_pager(win->parent_xlib);
      
   first_click=FALSE;
   
   return;
    
}

  do {
  
    if (force_state == force_show) {
      show=TRUE;
      break;
    }
  
    if (force_state ==force_hide) {
      show=FALSE;
      break;
    }

    if (win->visibility != VisibilityUnobscured  &&  win->parent_xlib != get_active_window()) {
      show=TRUE;
      break;
    }
    
    show=(!xlib_window_is_viewable(win->parent_xlib));
  
   } while (0);

 
  if (show) {
  
    if (debug) printf ("show\n");

    skip_taskbar (win->parent_xlib, FALSE);  
    gdk_window_focus (win->parent_gdk, gtk_get_current_event_time());
  
    get_window_position (win->parent_xlib, &x, &y);
    gdk_window_move (win->parent_gdk, x, y); //force taskbar update

    
   } else {
   
    if (debug) printf ("hide\n");
       
    XIconifyWindow (GDK_DISPLAY(), win->parent_xlib, DefaultScreen(GDK_DISPLAY()));
    skip_taskbar (win->parent_xlib, TRUE);

   }
 
  gboolean *value;
  gboolean new_value=show;
   
  value=&g_array_index (win->workspace, gboolean, win->desktop);
  *value= new_value;

}

gboolean withdrawn (Window window)
{
  unsigned char *data;
  unsigned long nitems;
  unsigned long leftover;
  Atom actual_type;
  int actual_format;
  int status;

  gboolean return_value=FALSE;
  
  status = XGetWindowProperty (GDK_DISPLAY(), window,
    wm_state, 0L, 1, False, wm_state, &actual_type, &actual_format,
    &nitems, &leftover, &data);

  if (status != Success)
    return FALSE;
  
  if ((actual_type == wm_state) && (nitems == 1) && data) {
  
    gint state = *(gint *) data;
    if (state == WithdrawnState)
      return_value=TRUE;
    XFree (data);
    return return_value;
  } 

  if (actual_type == None)
    return TRUE;
  
  return FALSE;
}

/*for debug...*/
void show_pixbuf (GdkPixbuf *buf)
{

  GtkWidget *window1;
  GtkWidget *image1;
  
  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  image1=gtk_image_new_from_pixbuf(buf);
  gtk_widget_show (image1);
  gtk_container_add (GTK_CONTAINER (window1), image1);
  gtk_widget_show (window1);

}

void display_window_id(Display *display, Window window)
{
  char *win_name;
  static char *window_id_format = "0x%lx";
   
  if (!debug)
    return;
  
  if (!assert_window (window))
    return;
    
  printf(window_id_format, window);         /* print id # in hex/dec */
  
  if (!window) {
    printf(" (none)");
  } else {
  if (window == DefaultRootWindow (display)) {
    printf(" (the root window)");
  }

  if (!XFetchName(display, window, &win_name)) { /* Get window name if any */
    printf(" (has no name)");
  } else if (win_name) {
    printf(" \"%s\"", win_name);
    XFree(win_name);
  } else
    printf(" (has no name)");
  }
 
  printf ("window int: %d\n", (int) window);
  
  printf("\n");

}

gboolean parse_arguments(int argc, char **argv, gchar **string, gchar **icon,
    gchar  **rest, gboolean *show, gboolean *ignore_splash_window, 
    gboolean *hide_start)
{
  int i;
  char rest_buf[1024]="";
  
  if (argc == 1) {
    show_short_help();
    return FALSE;
  }  
  
  for (i = 1; i < argc; i++) {
  
    do {
    
      if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
        show_help();
        return FALSE;
      }
      
      if (!strcmp(argv[i], "--windowinfo") || !strcmp(argv[i], "-wi")) {
        get_string();
        return FALSE;
      }
      
      if (!strcmp(argv[i], "--splashwindow") || !strcmp(argv[i], "-sw")) {
       *ignore_splash_window=TRUE;
        break;
      }
      
      if (!strcmp(argv[i], "--show") || !strcmp(argv[i], "-s")) {
        *show=TRUE;
        break;
      }  
    
      if (!strcmp(argv[i], "--hide_window") || !strcmp(argv[i], "-hw")) {
        *hide_start=TRUE;
        break;
      }  
    
                 
      if (!strcmp(argv[i], "--hints") || !strcmp(argv[i], "-hi")) {
        if ((i+1) ==  argc) {
          show_help();
          return FALSE;
        }
        
        *string=g_strdup (argv[i+1]);
         i++;
         break;
      }
      
     if (!strcmp(argv[i], "--icon") || !strcmp(argv[i], "-i")) {
      if ((i+1) ==  argc) {
        show_help();
        return FALSE;
      }
      
      *icon=g_strdup (argv[i+1]);
       i++;
       break;
     }  
    
     if (strlen (rest_buf) > 0)
        strcat (rest_buf, " ");
      
      strcat (rest_buf, argv[i]);
      
    }while (0);
  
  }
  
  if (strlen (rest_buf) ==0) {
  show_help();
  return FALSE;
  }
  
  
  *rest=g_strdup (rest_buf);

  return TRUE;
}

gboolean parse_string (gchar *string,  win_struct *win)
{
  
  gchar **split;
  
  split=g_strsplit (string, ";",3);
  
  win->wm_res_class=g_strdup (split[0]);
  win->wm_res_name=g_strdup (split[1]);
  win->wm_name=g_strdup (split[2]);
  
  g_strfreev(split);

  return (win->wm_res_class && win->wm_res_name && win->wm_name);

}

gboolean string_match (Window w, win_struct *win)
{

  XClassHint class_hints;
  XTextProperty prop;
  gboolean match=FALSE;
  
  if (!XGetClassHint(GDK_DISPLAY(), w, &class_hints))
    return FALSE;
    
  if (debug) printf ("found strings: res_class: %s  res_name %s   \n", class_hints.res_class,
      class_hints.res_name);
  
  
  if ((!strcmp(class_hints.res_class, win->wm_res_class)) && 
      (!strcmp(class_hints.res_name, win->wm_res_name))) {
  
    if (!strcmp (win->wm_name, "null")) {
      match=TRUE;
    } else {
    
      if (XGetWMName(GDK_DISPLAY(), w, &prop)) {
    
        if (debug) printf ("found window prop: %s\n", prop.value);
      
        if (!strcmp(prop.value, win->wm_name))
        match=TRUE;
      
        XFree(prop.value); 
      }
    }
  
  }
  
  XFree(class_hints.res_class);
  XFree(class_hints.res_name);
 
  
  return match;
}

gboolean pid_match(Window w, GPid pid)
{
  
  Atom actual_type;
  int actual_format;
  unsigned long nitems, leftover;
  unsigned char *pid_return;

  if (XGetWindowProperty(GDK_DISPLAY(), w,
      XInternAtom(GDK_DISPLAY(), "_NET_WM_PID", False), 0,
      1, False, XA_CARDINAL, &actual_type,
      &actual_format, &nitems, &leftover, &pid_return) == Success) {
        
    if (pid_return) {
  
      if (debug) printf ("wm pid: %d\n", *(GPid *) pid_return);
      
      if (pid == *(GPid *) pid_return) {
        XFree(pid_return);
        return TRUE;
      }
    
      return FALSE;
    }
  }

  return FALSE;
}

gboolean window_match (Window window, win_struct *win)
{

  static gint match_count=0;
  gboolean match=FALSE;

  
  if (win->wm_res_class && win->wm_res_name && win->wm_name) {
  
    match=string_match( window, win);
    
  } else {
  
  if (pid_match (window, win->pid)) {
   
     match=TRUE;
     
     if (debug) printf ("pid match: with match_count: %d\n", match_count);
       
     match_count++;
     
     if (match_count ==1 &&   win->ignore_splash_window) {
       if (debug) printf ("splash window found\n");
        
       match=FALSE;
     }
   
    }
   
  }    
 
  return match;
}

/*from gdk-pixbuf-xlib-drawabel.c*/
gboolean xlib_window_is_viewable (Window w)
{
  XWindowAttributes wa;
   
  if (!assert_window (w))
    return FALSE;
  
  
  while (w != 0) {
    Window parent, root, *children;
    int nchildren;
    
    XGetWindowAttributes (GDK_DISPLAY(), w, &wa);
    
    if (wa.map_state != IsViewable) {
      return FALSE;
    }
     
    if (!XQueryTree (GDK_DISPLAY(), w, &root, &parent, &children, &nchildren))
      return 0;
    
    if (nchildren > 0)
      XFree (children);
    
    if ((parent == root) || (w == root))
      return TRUE;
    
    w = parent;
  }
  return TRUE;
}

void get_window_position (Window window, gint *x, gint *y)
{
 
  Window  root_return;
  unsigned int width_return, height_return, border, depth;

  Window our=one_under_root(GDK_DISPLAY(), window);

  if (debug) printf ("get_window_position: window normal: %d, window our: %d\n", 
    (int) window, (int)our);

  XGetGeometry (GDK_DISPLAY(), our,
    &root_return, x, y, &height_return, &width_return, &border, &depth);
 
}

GPid exec_command (gchar *command)
{

  gchar **child_vector=NULL;
  gint num;
  GError *error=NULL;
  GPid pid=0;
  gboolean success_parse;
  gboolean success_spawn;

  success_parse=g_shell_parse_argv (command, &num, &child_vector, NULL);
    
  if (success_parse) {
    success_spawn=g_spawn_async (NULL,child_vector, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL |
        G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, &pid, &error);
  
  if (!success_spawn)
    printf ("AllTray: %s\n", error->message);
    
    g_strfreev (child_vector);
  }
   
  return pid;

}

Window one_under_root (Display *display, Window window)
{
  
  Window root_return=None;
  Window parent=None;
  Window *kids;
  Window runner=None;
  
  int nkids;
  
  runner=window;

  if (debug) printf ("one under root function\n");

    while (1) {
    
      if (debug) printf ("current runner: %d\n", (int) runner);
   
       if (! XQueryTree (display, runner, &root_return, &parent, &kids, &nkids))
         {  continue;  }
       if (kids) XFree ((char *)kids);
         
       if (parent == DefaultRootWindow (display))
         break;
       
       runner=parent;
          
     }
    
  return runner;
}   

GdkColormap*
get_cmap (GdkPixmap *pixmap)
{
  GdkColormap *cmap;

  cmap = gdk_drawable_get_colormap (pixmap);
  if (cmap)
    g_object_ref (G_OBJECT (cmap));

  if (cmap == NULL)
    {
      if (gdk_drawable_get_depth (pixmap) == 1)
        {
          /* try null cmap */
          cmap = NULL;
        }
      else
        {
          /* Try system cmap */
          GdkScreen *screen = gdk_drawable_get_screen (GDK_DRAWABLE (pixmap));
          cmap = gdk_screen_get_system_colormap (screen);
          g_object_ref (G_OBJECT (cmap));
        }
    }

  /* Be sure we aren't going to blow up due to visual mismatch */
  if (cmap &&
      (gdk_colormap_get_visual (cmap)->depth !=
       gdk_drawable_get_depth (pixmap)))
    cmap = NULL;
  
  return cmap;
}

gboolean get_window_pixmap (Window window, Pixmap *pix, Pixmap *mask)
{
 
  XWMHints *wm_hints;
  
  gdk_error_trap_push();
  
  wm_hints= XGetWMHints(GDK_DISPLAY(), window);
  
  if (gdk_error_trap_pop()) {
    if (wm_hints)
      XFree (wm_hints);
      return FALSE;
  }
    
  if (wm_hints != NULL) {
    if (!(wm_hints->flags & IconMaskHint))
      wm_hints->icon_mask = None;
      
    if ((wm_hints->flags & IconPixmapHint) && (wm_hints->icon_pixmap)) {
      *pix=wm_hints->icon_pixmap;
      *mask=wm_hints->icon_mask;
    }
  
    XFree (wm_hints);
   return TRUE;
  }

  return FALSE;
}

static void free_pixels (guchar *pixels, gpointer data)
{
  g_free (pixels);
}

static void get_pixmap_geometry (Pixmap       pixmap,
                     int         *w,
                     int         *h,
                     int         *d)
{
  Window root_ignored;
  int x_ignored, y_ignored;
  guint width, height;
  guint border_width_ignored;
  guint depth;

  if (w)
    *w = 1;
  if (h)
    *h = 1;
  if (d)
    *d = 1;
  
  XGetGeometry (GDK_DISPLAY(),
                pixmap, &root_ignored, &x_ignored, &y_ignored,
                &width, &height, &border_width_ignored, &depth);

  if (w)
    *w = width;
  if (h)
    *h = height;
  if (d)
    *d = depth;
}

GdkPixbuf* _wnck_gdk_pixbuf_get_from_pixmap (GdkPixbuf   *dest,
                                  Pixmap       xpixmap,
                                  int          src_x,
                                  int          src_y,
                                  int          dest_x,
                                  int          dest_y,
                                  int          width,
                                  int          height)
{
  GdkDrawable *drawable;
  GdkPixbuf *retval;
  GdkColormap *cmap;
  
  retval = NULL;
  
  drawable = gdk_xid_table_lookup (xpixmap);

  if (drawable)
    g_object_ref (G_OBJECT (drawable));
  else
    drawable = gdk_pixmap_foreign_new (xpixmap);

  cmap = get_cmap (drawable);

  /* GDK is supposed to do this but doesn't in GTK 2.0.2,
   * fixed in 2.0.3
   */
  if (width < 0)
    gdk_drawable_get_size (drawable, &width, NULL);
  if (height < 0)
    gdk_drawable_get_size (drawable, NULL, &height);
  
  retval = gdk_pixbuf_get_from_drawable (dest,
                                         drawable,
                                         cmap,
                                         src_x, src_y,
                                         dest_x, dest_y,
                                         width, height);

  if (cmap)
    g_object_unref (G_OBJECT (cmap));
  g_object_unref (G_OBJECT (drawable));

  return retval;
}

static GdkPixbuf* apply_mask (GdkPixbuf *pixbuf,
            GdkPixbuf *mask)
{
  int w, h;
  int i, j;
  GdkPixbuf *with_alpha;
  guchar *src;
  guchar *dest;
  int src_stride;
  int dest_stride;
  
  w = MIN (gdk_pixbuf_get_width (mask), gdk_pixbuf_get_width (pixbuf));
  h = MIN (gdk_pixbuf_get_height (mask), gdk_pixbuf_get_height (pixbuf));
  
  with_alpha = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);

  dest = gdk_pixbuf_get_pixels (with_alpha);
  src = gdk_pixbuf_get_pixels (mask);

  dest_stride = gdk_pixbuf_get_rowstride (with_alpha);
  src_stride = gdk_pixbuf_get_rowstride (mask);
  
  i = 0;
  while (i < h)
    {
      j = 0;
      while (j < w)
        {
          guchar *s = src + i * src_stride + j * 3;
          guchar *d = dest + i * dest_stride + j * 4;
          
          /* s[0] == s[1] == s[2], they are 255 if the bit was set, 0
           * otherwise
           */
          if (s[0] == 0)
            d[3] = 0;   /* transparent */
          else
            d[3] = 255; /* opaque */
          
          ++j;
        }
      
      ++i;
    }

  return with_alpha;
}

static GdkPixbuf* scaled_from_pixdata (guchar *pixdata,
                     int     w,
                     int     h,
                     int     new_w,
                     int     new_h)
{
  GdkPixbuf *src;
  GdkPixbuf *dest;
  
  src = gdk_pixbuf_new_from_data (pixdata,
                                  GDK_COLORSPACE_RGB,
                                  TRUE,
                                  8,
                                  w, h, w * 4,
                                  free_pixels, 
                                  NULL);

  if (src == NULL)
    return NULL;

  if (w != h)
    {
      GdkPixbuf *tmp;
      int size;
      
      size = MAX (w, h);
      
      tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);
      
      if (tmp != NULL) {
       gdk_pixbuf_fill (tmp, 0);
       gdk_pixbuf_copy_area (src, 0, 0, w, h, tmp, (size - w) / 2, (size - h) / 2);
      
       g_object_unref (src);
       src = tmp;
      }
    }
  
  if (w != new_w || h != new_h)
    {
      dest = gdk_pixbuf_scale_simple (src, new_w, new_h, GDK_INTERP_BILINEAR);
      
      g_object_unref (G_OBJECT (src));
    }
  else
    {
      dest = src;
    }

  return dest;
}

gboolean try_wm_hints (Window xwindow, GdkPixbuf **iconp)
{
  GdkPixbuf *unscaled = NULL;
  GdkPixbuf *mask = NULL;
  int w, h;

  Pixmap src_pixmap=None;
  Pixmap src_mask=None;

  if (!get_window_pixmap (xwindow, &src_pixmap, &src_mask))
    return FALSE;

  if (src_pixmap==None) {
    if (debug) printf ("src_pixmap is none\n");
    return FALSE;
  }
  
            
  gdk_error_trap_push();

  get_pixmap_geometry (src_pixmap, &w, &h, NULL);
      
  unscaled = _wnck_gdk_pixbuf_get_from_pixmap (NULL,
                                               src_pixmap,
                                               0, 0, 0, 0,
                                               w, h);

  if (unscaled && src_mask != None) {
      get_pixmap_geometry (src_mask, &w, &h, NULL);
      mask = _wnck_gdk_pixbuf_get_from_pixmap (NULL,
                                               src_mask,
                                               0, 0, 0, 0,
                                               w, h);
    }
  
   if (gdk_error_trap_pop ()) {
       if (unscaled)
         g_object_unref (unscaled);
       
       if (mask)
         g_object_unref (mask);
       
       return FALSE;
    }

  if (mask) {
      GdkPixbuf *masked;
      
      masked = apply_mask (unscaled, mask);
      g_object_unref (G_OBJECT (unscaled));
      unscaled = masked;

      g_object_unref (G_OBJECT (mask));
      mask = NULL;
    }
  
  if (unscaled) {
      *iconp =
        gdk_pixbuf_scale_simple (unscaled,
                                 24,24, GDK_INTERP_HYPER);
      
      
      g_object_unref (G_OBJECT (unscaled));
      return TRUE;
    }
  else
    return FALSE;
}

/* The icon-reading code is copied
 * from metacity, please sync bugfixes
 */
static gboolean find_largest_sizes (gulong *data,
                    gulong  nitems,
                    int    *width,
                    int    *height)
{
  *width = 0;
  *height = 0;
  
  while (nitems > 0)
    {
      int w, h;
      gboolean replace;

      replace = FALSE;
      
      if (nitems < 3)
        return FALSE; /* no space for w, h */
      
      w = data[0];
      h = data[1];
      
      if (nitems < ((w * h) + 2))
        return FALSE; /* not enough data */

      *width = MAX (w, *width);
      *height = MAX (h, *height);
      
      data += (w * h) + 2;
      nitems -= (w * h) + 2;
    }

  return TRUE;
}

static gboolean find_best_size (gulong  *data,
                gulong   nitems,
                int      ideal_width,
                int      ideal_height,
                int     *width,
                int     *height,
                gulong **start)
{
  int best_w;
  int best_h;
  gulong *best_start;
  int max_width, max_height;
  
  *width = 0;
  *height = 0;
  *start = NULL;

  if (!find_largest_sizes (data, nitems, &max_width, &max_height))
    return FALSE;

  if (ideal_width < 0)
    ideal_width = max_width;
  if (ideal_height < 0)
    ideal_height = max_height;
  
  best_w = 0;
  best_h = 0;
  best_start = NULL;
  
  while (nitems > 0)
    {
      int w, h;
      gboolean replace;

      replace = FALSE;
      
      if (nitems < 3)
        return FALSE; /* no space for w, h */
      
      w = data[0];
      h = data[1];
      
      if (nitems < ((w * h) + 2))
        break; /* not enough data */

      if (best_start == NULL)
        {
          replace = TRUE;
        }
      else
        {
          /* work with averages */
          const int ideal_size = (ideal_width + ideal_height) / 2;
          int best_size = (best_w + best_h) / 2;
          int this_size = (w + h) / 2;
          
          /* larger than desired is always better than smaller */
          if (best_size < ideal_size &&
              this_size >= ideal_size)
            replace = TRUE;
          /* if we have too small, pick anything bigger */
          else if (best_size < ideal_size &&
                   this_size > best_size)
            replace = TRUE;
          /* if we have too large, pick anything smaller
           * but still >= the ideal
           */
          else if (best_size > ideal_size &&
                   this_size >= ideal_size &&
                   this_size < best_size)
            replace = TRUE;
        }

      if (replace)
        {
          best_start = data + 2;
          best_w = w;
          best_h = h;
        }

      data += (w * h) + 2;
      nitems -= (w * h) + 2;
    }

  if (best_start)
    {
      *start = best_start;
      *width = best_w;
      *height = best_h;
      return TRUE;
    }
  else
    return FALSE;
}

static void argbdata_to_pixdata (gulong *argb_data, int len, guchar **pixdata)
{
  guchar *p;
  int i;
  
  *pixdata = g_new (guchar, len * 4);
  p = *pixdata;

  /* One could speed this up a lot. */
  i = 0;
  while (i < len)
    {
      guint argb;
      guint rgba;
      
      argb = argb_data[i];
      rgba = (argb << 8) | (argb >> 24);
      
      *p = rgba >> 24;
      ++p;
      *p = (rgba >> 16) & 0xff;
      ++p;
      *p = (rgba >> 8) & 0xff;
      ++p;
      *p = rgba & 0xff;
      ++p;
      
      ++i;
    }
}

static gboolean read_rgb_icon (Window         xwindow,
               int            ideal_width,
               int            ideal_height,
               int           *width,
               int           *height,
               guchar       **pixdata)
         
{
  Atom type;
  int format;
  gulong nitems;
  gulong bytes_after;
  int result, err;
  unsigned char *data;
  gulong *best;
  int w, h;
   
  gdk_error_trap_push();
  type = None;
  data = NULL;
  result = XGetWindowProperty (GDK_DISPLAY(), xwindow,
    net_wm_icon, 0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
     &bytes_after, &data);
  
  err=gdk_error_trap_pop();
      
  if (result != Success || err || !data || type != XA_CARDINAL) {
    if (debug) printf ("ERROR in read_rgb_icon\n");
    return FALSE;
  }
 
  if (!find_best_size ((gulong*)data, nitems,
                       ideal_width, ideal_height,
                       &w, &h, &best)) {
      XFree (data);
      return FALSE;
  }
  
  *width = w;
  *height = h;

  argbdata_to_pixdata (best, w * h, pixdata);
 
  XFree (data);
  
  return TRUE;
}

GdkPixbuf *get_window_icon (Window xwindow)
{

  guchar *pixdata;
  int w, h;
  GdkPixbuf *icon=NULL;
  
  if (debug) printf ("get icon\n");
  
  pixdata = NULL;
  
  if (read_rgb_icon (xwindow, 24, 24, &w, &h, &pixdata))   {
  
    if (debug) printf ("read_rgb_icon  --> ok\n");
      
    icon= scaled_from_pixdata (pixdata, w, h, 24, 24);
  
    if (icon)
      return icon;
  }
  
  if (debug) printf ("rgb failed\n");
    
  if (try_wm_hints (xwindow, &icon)) {
    return icon;
  }
  
  if (debug) printf ("need fallback icon\n");
    
  GdkPixbuf *base;
  
  base = gdk_pixbuf_new_from_inline (-1, fallback_icon,
                                     FALSE,
                                     NULL);
  
  g_assert (base);

  return base;

}

void update_window_icon(win_struct *win)
{

  GdkPixbuf *icon_return; 
  static gboolean dont_update=FALSE;

  if (dont_update)
    return;

  if (win->user_icon) {
    icon_return=win->user_icon;
    dont_update=TRUE;
  }  else {
    icon_return =get_window_icon (win->child_xlib);
  }
  
  if (icon_return) {
  
    GList *icons=NULL;
  
    icons = g_list_append (icons, (gpointer) icon_return);
    gdk_window_set_icon_list (win->parent_gdk, icons);
    g_list_free(icons);
      
    tray_update_icon(win, icon_return);
    
  
    if (win->icon) {
      g_object_unref (win->icon);
    }
    
    win->icon=icon_return;
  }

}

void skip_taskbar (Window window, gboolean add)
{
  
  XEvent xev;
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.window = window;
  xev.xclient.message_type = net_wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
  xev.xclient.data.l[1] = net_wm_state_skip_taskbar;
  xev.xclient.data.l[2] = 0;
  xev.xclient.data.l[3] = 0;
  xev.xclient.data.l[4] = 0;
  
  XSendEvent (GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
     SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

void get_string(void)
{

  int status;
  Cursor cursor;
  XEvent event;
  Window target_win = None;
  Window XmuWindow;
  
  int buttons = 0;
  
  XClassHint class_hints;
  XTextProperty prop;
  char *win_name;

  Atom actual_type;
  int actual_format;
  unsigned long nitems, leftover;
  unsigned char *pid_return;

  printf ("\n\nAllTray: Please select the window about which you\n" \
                 "         would like information by clicking the mouse in that window....\n\n");

  /* Make the target cursor */
  cursor = XCreateFontCursor(GDK_DISPLAY(), XC_crosshair);
  
  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
  ButtonPressMask|ButtonReleaseMask, GrabModeSync,
  GrabModeAsync, GDK_ROOT_WINDOW(), cursor, CurrentTime);
  if (status != GrabSuccess)
  return;
  
  
  /* Let the user select a window... */
  while ((target_win == None) || (buttons != 0)) {
    /* allow one more event */
    XAllowEvents(GDK_DISPLAY(), SyncPointer, CurrentTime);
    XWindowEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), ButtonPressMask|ButtonReleaseMask, &event);
    switch (event.type) {
    case ButtonPress:
    if (target_win == None) {
    target_win = event.xbutton.subwindow; /* window selected */
    if (target_win == None) target_win = GDK_ROOT_WINDOW();
    }
    buttons++;
    break;
    case ButtonRelease:
    if (buttons > 0) /* there may have been some down before we started */
    buttons--;
    break;
    }
  } 
  
  XUngrabPointer(GDK_DISPLAY(), CurrentTime);      /* Done with pointer */
  
  if (target_win == None)
    return;
  
  XmuWindow=XmuClientWindow (GDK_DISPLAY(), target_win);
   
  if (!XFetchName(GDK_DISPLAY(), XmuWindow, &win_name)) { 
    //printf(" (has no name)\\nn");
  } else if (win_name) {
    //printf("%s\n\n", win_name);
    XFree(win_name);
  }
    
  if (XGetWindowProperty(GDK_DISPLAY(), XmuWindow,
      XInternAtom(GDK_DISPLAY(), "_NET_WM_PID", False), 0,
      1, False, XA_CARDINAL, &actual_type,
      &actual_format, &nitems, &leftover, &pid_return) == Success) {
  
      if (pid_return) {
      
     // if (debug) printf ("Pid: %d\n", *(GPid *) pid_return);
      
      }
   
  }
  
  
  if (XGetClassHint(GDK_DISPLAY(), XmuWindow, &class_hints)) {
  
    if (debug) {
      //printf ("class_hint.res_class: %s\n", class_hints.res_class);
      //printf ("class_hint.res_name: %s\n", class_hints.res_name);
    }
  }
    
  if (XGetWMName(GDK_DISPLAY(), XmuWindow, &prop)) {
  
    /*
    if (prop.value)
      if (debug) printf ("wm_name: %s\n", prop.value);
    else
     if (debug) printf ("wm_name: None\n");
    */
  
  }
  
  printf ("Alltray: Ok\n");

  if (prop.value)
      if (debug) printf ("AllTray: selected window: %s\n", prop.value);

  printf ("\nAllTray: Now close the selected window and \"tray\" the window\n         with the following steps."\
              "If one step fails, try next one.\n\n");


  printf ("AllTray:\n\n");
      
        
  printf (" Step 1:\n");
  if (pid_return) {
    printf ("   alltray <program_name>\n");
  } else
    printf ("   Step 1 not possible\n");

  printf (" Step 2:\n");

    
  printf ("   alltray --hints \"%s;%s;null\" <program_name> [parameter]\n", 
        class_hints.res_class, class_hints.res_name);

  printf (" Step 3:\n");

  printf ("   alltray --hints \"%s;%s;%s\" <program_name> [parameter]\n", 
      class_hints.res_class, class_hints.res_name, prop.value);
  
  

  show_tested_programs();
  
  if (class_hints.res_class)
    XFree(class_hints.res_class);
  if (class_hints.res_name)
    XFree(class_hints.res_name);
  if (prop.value)
    XFree (prop.value);
  if (pid_return)
    XFree(pid_return);
} 

void update_window_title(win_struct *win)
{

  char *title = NULL;
  gchar *title_string=NULL;
 
  if (debug) printf ("update window title\n");

  if (!assert_window(win->child_xlib))
    return;
 
  if (XFetchName(GDK_DISPLAY(), win->child_xlib, &title) && title) {
  
    title_string=g_strconcat (title, "  (AllTray)", NULL);
  
    gdk_window_set_title (win->parent_gdk, title_string);
    
    if (win->title)
      g_free (win->title);
    
    win->title=title_string;
     
    tray_update_tooltip (win);
        
    XFree(title);

  }

}

void  destroy_all_and_exit (win_struct *win, gboolean kill_child)
{

  XClientMessageEvent ev;
  
  gdk_window_remove_filter(win->parent_gdk, parent_window_filter, (gpointer) win);
  gdk_window_remove_filter (win->child_gdk, child_window_filter, (gpointer) win);
  gdk_window_remove_filter(win->root_gdk, root_filter_workspace, (gpointer) win);

  if (kill_child) {
   
     get_window_position (win->parent_xlib, &win->parent_window_x, &win->parent_window_y);
   
     XSelectInput(GDK_DISPLAY(), win->child_xlib, StructureNotifyMask);

     XReparentWindow (GDK_DISPLAY(), win->child_xlib, GDK_ROOT_WINDOW(), 0,0);
     XMoveWindow (win->display, win->child_xlib, win->parent_window_x, win->parent_window_y);
  
      for(;;) {
        XEvent e;
        XNextEvent(GDK_DISPLAY(), &e);
        if (e.type == ReparentNotify)
          break;
      }
       
             
      ev.type = ClientMessage;
      ev.window = win->child_xlib;
      ev.message_type =net_close_window;
      ev.format = 32;
      ev.data.l[0] = 0;
      ev.data.l[1] = 0;
      ev.data.l[2] = 0;
      ev.data.l[3] =0;
      ev.data.l[4] = 0;
      
      gdk_error_trap_push ();
      XSendEvent (win->display, win->root_xlib, False, 
          SubstructureNotifyMask | SubstructureRedirectMask, (XEvent *)&ev);
      
      XSync (win->display, False);
      gdk_error_trap_pop ();
    
  }


  tray_done(win);

  g_free (win->title);
  g_object_unref (win->icon);
  g_array_free (win->workspace, FALSE);
  
  XDestroyWindow(win->display, win->parent_xlib);
  g_free (win);

  gtk_main_quit ();

}

void show_tested_programs (void)
{
  printf ("\n\nExample programs:\n\n"\

             "  anjuta: Step 1, alltray anjuta -sw\n"\
             "  ding: Step 2, alltray --hints \"Ding;ding;null\" ding\n"\
             "  gcalctool: Step 1, alltray gcalctool\n"\
             "  gnome-terminal: Step 2 or Step 3, \n      alltray --hints \"Gnome-terminal;gnome-terminal;Terminal\" gnome-terminal\n"\
             "  kcalc: Step 1, alltray kcalc\n"\
             "  kdict: Step 2, alltray --hints \"Kdict;kdict;null\" kdict\n"\
             "  kmail: Step 2,  alltray --hints \"Kmail;kmail;null\" kmail\n"\
             "  konqueror: Step 1, alltray konqueror\n"\
             "  konsole: Step 1, alltray konsole\n"\
             "  nautilus: Step 1, alltray nautilus\n"\
             "  thunderbird: Step 2, alltray --hints \"Thunderbird-bin;Gecko;null\" thunderbird\n"\
             "  xchat: Step 1, alltray xchat\n"\
             "  valknut: Step 1, alltray -sw valknut --disable-tray\n"\
             "  xmms: Step 3, alltray --hints \"xmms;XMMS_Player;XMMS\" xmms\n"\
             );


}

void show_short_help (void)
{
  printf ("\nAllTray Version 0.22\n" \
            "\n\n    To tray new program run alltray with \"alltray --windowinfo\" \n    and follow the instructions.\n\n"\

             "    all options with \"alltray -h\"\n");
}

void show_help(void)
{
  printf ("\nAllTray Version 0.22\n\n" \
            "\n\nI recommend to start alltray with \"alltray --windowinfo\" and follow the instructions!\n\n\n"\

             "usage: alltray [options] <program_name> [program parameter]\n\n" \
             "where options include:\n"\
             "  --help; -h:  print this message\n" \
             "  --show; -s:  do not hide window after start\n"\
             "  --hide_window; -hw: hide window during startup (experimental, Gnome only)\n"\
             "  --hints; -hi \"WM_CLASS CLASS; WM_CLASS NAME; WM_NAME\"\n" \
             "  --icon; -i  <path to png>: use this icon\n" \
             "  --windowinfo; -wi: get hints string by clicking on window\n"\
             "  --splashwindow; -sw: ignore splash window\n\n");


  show_tested_programs();

  exit (0);
}
