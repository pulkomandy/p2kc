#ifndef P2KC_H
#define P2KC_H

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>
#include <glib/gstdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

enum
{
  ICON = 0,
  FILENAME,
  SIZE,
  MODIFIED,
  COLUMNS
};
enum
{
   SORTID_NAME = 0,
   SORTID_SIZE,
   SORTID_MODIFIED,
   SORTID_COLUMNS
};
enum
{
   KEYCMD_EDIT = 0,
   KEYCMD_VIEW,
   KEYCMD_ATTRIB,
   KEYCMD_COPY
};
/*****************************************************************
 *
 *   Prototypes
 *
 *****************************************************************/
char* getenv(char*);
void file_manager_error (gchar*);
void populate_tree_model1 (GtkWidget*);
void populate_tree_model2 (GtkWidget*);
static void setup_tree_view (GtkWidget*);
void logstat (gchar*);
GPid spawn_p2kcore ();
int displayP2kList (GtkTreeView*, char*);
int isP2kFolder(GtkTreeView*);
int system (const char * command );
int atoi(const char * str );
char *strptime(const char *s, const char *format, struct tm *tm);
 /*****************************************************************
 *
 *   Global variables
 *
 *****************************************************************/
gchar* version = "0.6";
gchar dummyfile[] = "/usr/local/share/p2kc/s5vi.url";
gchar latestversion[5];
const gchar *size_type[4];
GList *current_path1;
GList *current_path2;
GladeXML *xml;
guint context_id;

gchar* path_to_string1 ();
gchar* path_to_string2 ();

static guint8        *databuf;       /* NULL */
static gint           stdin_fd, stderr_fd, stdout_fd;
GPid     child_pid;

static int isP2k,isRight;

char p2kcommand[1000],actVol[32];
char FullPath[1000];
char FileListAtHome[1000];
char FileLatestAtHome[1000];
char ConfigFileAtHome[1000];
gint SBcontext_id;
GtkWidget *window,*statusbar,*entry1,*entry2,*cb1,*cb2,*treeview1,*treeview2,*pb;
GtkWidget *seemnum,*seemrec,*F7Entry;
GtkWidget *menu,*menuAttr;
GtkWidget *splashwindow;
GtkWidget *optentry1,*optentry2,*optentry3,*optentry4;
int keycmd;
/*****************************************************************
 *
 *   Global Config file variables
 *
 *****************************************************************/
 char OptEngineCmd[1000]="p2k-core -s";
 char OptExitCmd[1000]="mode a";
 char OptFilter[1000]="*";
 char OptOpenCmd[1000]="gnome-open";
#endif
