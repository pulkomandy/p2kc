/*---------------------------------------------------------------------------
 *      p2kc.c  aka P2kCommander for linux
 *
 *      V_0.6    2008/jan/25
 *
 *      Copyright 2007 Sandor Otvos <s5vi.hu@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */
#include "p2kc.h"
#define BUFSIZE 1024
int on_tim1 ()
{
  gtk_widget_destroy(GTK_WIDGET(splashwindow));
  return 0;
}
/*****************************************************************
 *
 *   m a i n   start here
 *
 *****************************************************************/
int main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);

  //GtkWidget *textview;
  current_path1 = NULL;
  current_path2 = NULL;
  GtkListStore *CBstore;
  GtkTreeIter CBiter;
  GtkCellRenderer *CBrenderer;
  GdkPixbuf *pixbuf_p2k, *pixbuf_hdd;
  char       currpath[PATH_MAX];
  gsize      len = 0;
  gint       tim1;
  GKeyFile *keyfile;
  gchar *configdata;
  FILE *hFile;
  GtkWidget *dialog;

//
// get widget IDs
//
  if(access("p2kc.glade",R_OK)==0) {
  	xml = glade_xml_new ("p2kc.glade", NULL, NULL);
	pixbuf_p2k = gdk_pixbuf_new_from_file ("pixmaps/p2kc.png", NULL);
	pixbuf_hdd = gdk_pixbuf_new_from_file ("pixmaps/hdd.png", NULL);
	splashwindow = glade_xml_get_widget (glade_xml_new ("splash.glade", NULL, NULL), "splashdlg");
	strcpy(dummyfile,"s5vi.url");
  }
  else {
  	xml = glade_xml_new ("/usr/local/share/p2kc/p2kc.glade", NULL, NULL);
	pixbuf_p2k = gdk_pixbuf_new_from_file ("/usr/local/share/p2kc/pixmaps/p2kc.png", NULL);
	pixbuf_hdd = gdk_pixbuf_new_from_file ("/usr/local/share/p2kc/pixmaps/hdd.png", NULL);
	splashwindow = glade_xml_get_widget (glade_xml_new ("/usr/local/share/p2kc/splash.glade", NULL, NULL), "splashdlg");
  }
  window = glade_xml_get_widget (xml, "window");
  treeview1 = glade_xml_get_widget (xml, "treeview1");
  treeview2 = glade_xml_get_widget (xml, "treeview2");
  //textview = glade_xml_get_widget (xml, "textview4");
  statusbar = glade_xml_get_widget (xml, "statusbar1");
  cb1 = glade_xml_get_widget (xml, "combobox1");
  cb2 = glade_xml_get_widget (xml, "combobox2");
  pb = glade_xml_get_widget (xml, "progressbar4");
  entry1 = glade_xml_get_widget (xml, "enry1");
  entry2 = glade_xml_get_widget (xml, "enry2");
  menu = glade_xml_get_widget (xml, "cmd_menu");
  menuAttr = glade_xml_get_widget (xml, "attrib");

  glade_xml_signal_autoconnect (xml);
//
// Initial widget setup
//
  setup_tree_view (treeview1);
  populate_tree_model1 (treeview1);
  setup_tree_view (treeview2);
  populate_tree_model2 (treeview2);
  SBcontext_id = gtk_statusbar_get_context_id( GTK_STATUSBAR(statusbar), "Statusbar");
//
// set hdd selector combo boxes
//
	CBstore = gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	gtk_combo_box_set_model (GTK_COMBO_BOX(cb1),GTK_TREE_MODEL (CBstore));
	gtk_combo_box_set_model (GTK_COMBO_BOX(cb2),GTK_TREE_MODEL (CBstore));

	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, "/", 1, pixbuf_hdd, -1);
	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, getenv("HOME"), 1, pixbuf_hdd, -1);
	getcwd(currpath,PATH_MAX);
	g_utf8_validate(currpath,len,NULL);
	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, currpath, 1, pixbuf_hdd, -1);
	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, "p2k: /a", 1, pixbuf_p2k, -1);
	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, "p2k: /b", 1, NULL, -1);
	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, "p2k: /c", 1, pixbuf_p2k, -1);
	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, "p2k: /e", 1, pixbuf_p2k, -1);
	gtk_list_store_append (CBstore, &CBiter);
	gtk_list_store_set (CBstore, &CBiter, 0, "p2k: /seems", 1, pixbuf_p2k, -1);

	CBrenderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb1), CBrenderer, FALSE);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb2), CBrenderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb1), CBrenderer, "pixbuf", 1,NULL);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb2), CBrenderer, "pixbuf", 1,NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb1),0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb2),2);
	gtk_widget_grab_focus (GTK_WIDGET(treeview1));
//
// init globals
//
	isRight=0;isP2k=0;
	sprintf(FileListAtHome,"%s/P2KFILELIST",getenv("HOME"));
	sprintf(ConfigFileAtHome,"%s/p2kc.conf",getenv("HOME"));
	sprintf(FileLatestAtHome,"%s/P2KCLATEST",getenv("HOME"));
//	sprintf(FileLatestAtHome,"%s/P2KCLATEST",g_get_home_dir());
	g_remove(FileListAtHome);
	//if (access("FILELIST",R_OK)==0) logstat ("Previously loaded FILELIST found.\nIf you want refresh FILELIST pls press Re-Read button.\n");
//
// set menu and buttons state
//
	gtk_widget_set_sensitive(menuAttr,FALSE);
//
// splash image
//
  gtk_widget_show_all (window);
  gtk_widget_show(GTK_WIDGET(splashwindow));
  tim1=gtk_timeout_add(2000,on_tim1,NULL);
//
// keyfile
//
  keyfile = g_key_file_new ();
  if (!g_key_file_load_from_file (keyfile, ConfigFileAtHome, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL)) {
  	// no config file present, create a new one.
  	g_key_file_set_string(keyfile,"P2k-core settings","Engine command",OptEngineCmd);
  	g_key_file_set_string(keyfile,"P2k-core settings","Command before exit",OptExitCmd);
  	g_key_file_set_string(keyfile,"P2k-core settings","Filelist filter",OptFilter);
  	g_key_file_set_string(keyfile,"P2k-core settings","File open command",OptOpenCmd);
  	configdata=g_key_file_to_data(keyfile,NULL,NULL);
	hFile=fopen(ConfigFileAtHome,"wb+");
	fwrite(configdata,1,strlen(configdata),hFile);
	fclose(hFile);
	g_free(configdata);
  }
  else {
  	// parse config file.
  	g_stpcpy(OptEngineCmd,g_key_file_get_string(keyfile,"P2k-core settings","Engine command",NULL));
  	g_stpcpy(OptExitCmd,g_key_file_get_string(keyfile,"P2k-core settings","Command before exit",NULL));
  	g_stpcpy(OptFilter,g_key_file_get_string(keyfile,"P2k-core settings","Filelist filter",NULL));
  	g_stpcpy(OptOpenCmd,g_key_file_get_string(keyfile,"P2k-core settings","File open command",NULL));
  }
//
// start p2k-core
//
  child_pid = spawn_p2kcore ();
//
// check update released?
//
	logstat("Looking for update.\n");
	logstat("Starting engine.\n");
	g_remove(FileLatestAtHome);
	strcpy(p2kcommand,"wget -nv -O");
	strcat(p2kcommand,FileLatestAtHome);
	strcat(p2kcommand," http://www.el-co.hu/dl/p2kc/Latest\n");
	if (!system(p2kcommand)) {
		//write(stdin_fd,p2kcommand,strlen(p2kcommand));
		hFile=fopen(FileLatestAtHome,"r");
		fgets(latestversion, 4, hFile);
		fclose(hFile);
		if (strcmp(version,latestversion)!=0) {
			dialog=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,"Are you sure to get update?");
			gtk_window_set_title(GTK_WINDOW(dialog),"New version available.");
			if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_YES) system("$BROWSER http://www.el-co.hu/smf/index.php?board=33.0");
			gtk_widget_destroy(GTK_WIDGET(dialog));
		}
	}
	else  logstat("Cannot check, no connection available.\n");
//
// main gtk loop
//
  gtk_main ();
  return 0;
}
/*****************************************************************
 *
 *   Display an error message to the user using GtkMessageDialog.
 *
 *****************************************************************/
void file_manager_error (gchar *message)
{
  GtkWidget *dialog, *window;

  window = glade_xml_get_widget (xml, "window");
  dialog = gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                   "File Manager Error");
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
/*****************************************************************
 *
 *   sort compare function
 *
 *****************************************************************/
gint sort_iter_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
  {
    gint sortcol = GPOINTER_TO_INT(userdata);
    gint ret = 0;
    gchar *name1, *name2, *name3, *name4;
    struct tm loctime1;
    struct tm loctime2;

    switch (sortcol) {
      case SORTID_NAME: {
        gtk_tree_model_get(model, a, FILENAME, &name1, -1);
        gtk_tree_model_get(model, b, FILENAME, &name2, -1);
        gtk_tree_model_get(model, a, SIZE, &name3, -1);
        gtk_tree_model_get(model, b, SIZE, &name4, -1);

        if (name1 == NULL || name2 == NULL) {
          if (name1 == NULL && name2 == NULL) break; /* both equal => ret = 0 */
          ret = (name1 == NULL) ? -1 : 1;
        }
        else {
		 if (g_utf8_collate(name3,"< DIR >")==0 && g_utf8_collate(name4,"< DIR >")==0) ;
		 else  {
		   if (g_utf8_collate(name3,"< DIR >")==0 || g_utf8_collate(name4,"< DIR >")==0) {
			//ret =(g_utf8_collate(name3,"< DIR >")) ? 1 : -1; break;
			ret =(strcmp(name3,"< DIR >")) ? 1 : -1; break;
		   }
		 }
         //ret = g_utf8_collate(name1,name2);
         ret = strcmp(name1,name2);
        }
      }
      break;
      case SORTID_SIZE: {
        gtk_tree_model_get(model, a, SIZE, &name1, -1);
        gtk_tree_model_get(model, b, SIZE, &name2, -1);
        if (name1 == NULL || name2 == NULL) {
          if (name1 == NULL && name2 == NULL) break; /* both equal => ret = 0 */
          ret = (name1 == NULL) ? -1 : 1;
        }
        else {
        	ret = (atoi(name1)<=atoi(name2)) ? -1 : 1;
        }
      }
      break;
      case SORTID_MODIFIED: {
        gtk_tree_model_get(model, a, MODIFIED, &name1, -1);
        gtk_tree_model_get(model, b, MODIFIED, &name2, -1);
        if (name1 == NULL || name2 == NULL) {
          if (name1 == NULL && name2 == NULL) break; /* both equal => ret = 0 */
          ret = (name1 == NULL) ? -1 : 1;
        }
        else {
        	if (isP2k==0) {
	        	strptime(name1,"%x %X",&loctime1);
    	    	strptime(name2,"%x %X",&loctime2);
        		ret = (mktime(&loctime1)<=mktime(&loctime2)) ? -1 : 1;
        	}
        	else ret = strcmp(name1,name2);
        }
	  }
      break;
      default:
        g_return_val_if_reached(0);
    g_free(name1); g_free(name2); g_free(name3); g_free(name4);
    }
    return ret;
  }
/*****************************************************************
 *
 *   Setup the columns in the tree view. These include a column for the GdkPixbuf,
 *   file name, file size, and last modification time/date.
 *
 *****************************************************************/
static void setup_tree_view (GtkWidget *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *store;
  GtkTreeSortable *sortable;
  GtkTreeSelection *selection;

  /* Create a tree view column with an icon and file name. */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, " Filename");

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", ICON, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  //columnsize
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 240);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_clickable (column,TRUE);
  gtk_tree_view_column_set_resizable (column,TRUE);
  //
  gtk_tree_view_column_set_attributes (column, renderer, "text", FILENAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* Insert a second tree view column that displays the file size. */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (" Size", renderer, "text", SIZE, NULL);
  gtk_tree_view_column_set_clickable (column,TRUE);
  gtk_tree_view_column_set_resizable (column,TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* Insert a third tree view column that displays the last modified time/date. */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (" Last Modified", renderer, "text", MODIFIED, NULL);
  gtk_tree_view_column_set_clickable (column,TRUE);
  gtk_tree_view_column_set_resizable (column,TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
//
// setup model
//
  store = gtk_list_store_new (COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
// sort
  sortable = GTK_TREE_SORTABLE(store);
  gtk_tree_sortable_set_sort_func(sortable, SORTID_NAME, sort_iter_compare_func, GINT_TO_POINTER(SORTID_NAME), NULL);
  gtk_tree_sortable_set_sort_func(sortable, SORTID_SIZE, sort_iter_compare_func, GINT_TO_POINTER(SORTID_SIZE), NULL);
  gtk_tree_sortable_set_sort_func(sortable, SORTID_MODIFIED, sort_iter_compare_func, GINT_TO_POINTER(SORTID_MODIFIED), NULL);
  gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),0),SORTID_NAME);
  gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),1),SORTID_SIZE);
  gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),2),SORTID_MODIFIED);
  gtk_tree_sortable_set_sort_column_id(sortable, SORTID_NAME, GTK_SORT_ASCENDING);
// end sort
  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode (selection,GTK_SELECTION_SINGLE);
//  gtk_tree_selection_set_mode (selection,GTK_SELECTION_MULTIPLE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));
  g_object_unref (store);
}
/*****************************************************************
 *
 *   Convert the GList path into a character string.
 *
 *****************************************************************/
gchar* path_to_string1 ()
{
  gchar *location1;
  GList *temp;

  /* If there is no content, use the root directory. */
  if (g_list_length (current_path1) == 0)
    location1 = G_DIR_SEPARATOR_S;
  /* Otherwise, build the path out of the GList content. */
  else {
    temp = current_path1;
    location1 = g_strconcat (G_DIR_SEPARATOR_S, (gchar*) temp->data, NULL);
    temp = temp->next;
      while (temp != NULL) {
      location1 = g_strconcat (location1, G_DIR_SEPARATOR_S, (gchar*) temp->data, NULL);
      temp = temp->next;
    }
  }
  return location1;
}
gchar* path_to_string2 ()
{
  gchar *location2;
  GList *temp;

  /* If there is no content, use the root directory. */
  if (g_list_length (current_path2) == 0) location2 = G_DIR_SEPARATOR_S;
  /* Otherwise, build the path out of the GList content. */
  else {
    temp = current_path2;
    location2 = g_strconcat (G_DIR_SEPARATOR_S, (gchar*) temp->data, NULL);
    temp = temp->next;
      while (temp != NULL) {
        location2 = g_strconcat (location2, G_DIR_SEPARATOR_S, (gchar*) temp->data, NULL);
        temp = temp->next;
      }
  }
  return location2;
}
/*****************************************************************
 *
 *   Setup the content of the tree model.
 *
 *****************************************************************/
void populate_tree_model1 (GtkWidget *treeview)
{
  GdkPixbuf *pixbuf_file, *pixbuf_dir;
  GtkListStore *store;
  GtkTreeIter iter;
  GdkPixbuf *directory;
  struct stat st;
  gchar *location1, *file;
  gfloat size;
  gint i, items = 0;;
  GDir *dir;
  struct tm *loctime;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));
  //selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  //model=gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
  gtk_list_store_clear (store);
  location1 = path_to_string1 ();

  /* If the current location is not the root directory, add the '..' entry. */
  if (g_list_length (current_path1) > 0) {
    directory = gtk_widget_render_icon(GTK_WIDGET(treeview),GTK_STOCK_DIRECTORY,GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, ICON, directory, FILENAME, "..", SIZE, "< DIR >", -1);
  }

  /* Return if the path does not exist. */
  if (!g_file_test (location1, G_FILE_TEST_IS_DIR)) {
    file_manager_error ("The path %s does not exist!");
    g_free (location1);
    return;
  }

  /* Display the new location in the address bar. */
  entry1 = glade_xml_get_widget (xml, "entry1");
  gtk_entry_set_text (GTK_ENTRY (entry1), location1);

  /* Add each file to the list along with the file size and modified date. */
  pixbuf_dir = gtk_widget_render_icon(GTK_WIDGET(treeview),GTK_STOCK_DIRECTORY,GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
  pixbuf_file = gtk_widget_render_icon(GTK_WIDGET(treeview),GTK_STOCK_FILE,GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
  dir = g_dir_open (location1, 0, NULL);
  while ((file = (gchar*) g_dir_read_name (dir))) {
    gchar *fn, *filesize; char modified[256];

    fn = g_strconcat (location1, "/", file, NULL);
    if (g_stat (fn, &st) == 0) {
      /* Calculate the file size and order of magnitude. */
      i = 0;
      size = (gfloat) st.st_size;

      /* Create strings for the file size and last modified date. */
//      filesize = g_strdup_printf ("%.1f %s", size, size_type[i]);
      filesize = g_strdup_printf ("%.0f ",size);
	  loctime = localtime (&st.st_mtime);
      strftime(modified,256,"%x %X",loctime);
    }

    /* Add the file and its properties as a new tree view row. */
    gtk_list_store_append (store, &iter);

    if (g_file_test (fn, G_FILE_TEST_IS_DIR))
      gtk_list_store_set (store, &iter, ICON, pixbuf_dir, FILENAME, file,
                          SIZE, "< DIR >", MODIFIED, modified, -1);
    else
      gtk_list_store_set (store, &iter, ICON, pixbuf_file, FILENAME, file,
                          SIZE, filesize, MODIFIED, modified, -1);
    items++;

    g_free (fn);
  }
  gtk_tree_view_set_cursor(GTK_TREE_VIEW (treeview),gtk_tree_path_new_from_string ("0"),NULL,FALSE);
  g_object_unref (pixbuf_dir);
  g_object_unref (pixbuf_file);
}
void populate_tree_model2 (GtkWidget *treeview)
{
  GdkPixbuf *pixbuf_file, *pixbuf_dir;
  GtkListStore *store;
  GtkTreeIter iter;
  GdkPixbuf *directory;
  struct stat st;
  gchar *location2, *file;
  gfloat size;
  gint i, items = 0;;
  GDir *dir;
  struct tm *loctime;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));
  gtk_list_store_clear (store);
  location2 = path_to_string2 ();

  /* If the current location is not the root directory, add the '..' entry. */
  if (g_list_length (current_path2) > 0)
  {
    directory = gtk_widget_render_icon(GTK_WIDGET(treeview),GTK_STOCK_DIRECTORY,GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, ICON, directory, FILENAME, "..", SIZE, "< DIR >", -1);
  }

  /* Return if the path does not exist. */
  if (!g_file_test (location2, G_FILE_TEST_IS_DIR)) {
    file_manager_error ("The path %s does not exist!");
    g_free (location2);
    return;
  }

  /* Display the new location in the address bar. */
  entry2 = glade_xml_get_widget (xml, "entry2");
  gtk_entry_set_text (GTK_ENTRY (entry2), location2);

  /* Add each file to the list along with the file size and modified date. */
  pixbuf_dir = gtk_widget_render_icon(GTK_WIDGET(treeview),GTK_STOCK_DIRECTORY,GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
  pixbuf_file = gtk_widget_render_icon(GTK_WIDGET(treeview),GTK_STOCK_FILE,GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
  dir = g_dir_open (location2, 0, NULL);
  while ((file = (gchar*) g_dir_read_name (dir))) {
    gchar *fn, *filesize; char modified[256];

    fn = g_strconcat (location2, "/", file, NULL);
    if (g_stat (fn, &st) == 0) {
      /* Calculate the file size and order of magnitude. */
      i = 0;
      size = (gfloat) st.st_size;

      /* Create strings for the file size and last modified date. */
//      filesize = g_strdup_printf ("%.1f %s", size, size_type[i]);
      filesize = g_strdup_printf ("%.0f ",size);
	  loctime = localtime (&st.st_mtime);
      strftime(modified,256,"%x %X",loctime);
    }

    /* Add the file and its properties as a new tree view row. */
    gtk_list_store_append (store, &iter);

    if (g_file_test (fn, G_FILE_TEST_IS_DIR))
      gtk_list_store_set (store, &iter, ICON, pixbuf_dir, FILENAME, file,
                          SIZE, "< DIR >", MODIFIED, modified, -1);
    else
      gtk_list_store_set (store, &iter, ICON, pixbuf_file, FILENAME, file,
                          SIZE, filesize, MODIFIED, modified, -1);
    items++;

    g_free (fn);
  }
  gtk_tree_view_set_cursor(GTK_TREE_VIEW (treeview),gtk_tree_path_new_from_string ("0"),NULL,FALSE);
  g_object_unref (pixbuf_dir);
  g_object_unref (pixbuf_file);
}
/*****************************************************************
 *
 *   Convert the GString into a list of path elements stored in current_path.
 *
 *****************************************************************/
void parse_location1 (GString *location1)
{
  gchar *find;
  gsize len;

  /* Free the current content stored in current_path. */
  g_list_foreach (current_path1, (GFunc) g_free, NULL);
  g_list_free (current_path1);
  current_path1 = NULL;

  /* Erase all forward slashes from the end of the string. */
  while (location1->str[location1->len - 1] == '/')
    g_string_erase (location1, location1->len - 1, 1);

  /* Parse through the string, splitting it into a GList at the '/' character. */
  while (location1->str[0] == '/')
  {
    while (location1->str[0] == '/')
      g_string_erase (location1, 0, 1);
    find = g_strstr_len (location1->str, location1->len, "/");

    if (find == NULL)
      current_path1 = g_list_append (current_path1, g_strdup (location1->str));
    else
    {
      len = (gsize) (strlen (location1->str) - strlen (find));
      current_path1 = g_list_append (current_path1, g_strndup (location1->str, len));
      g_string_erase (location1, 0, len);
    }
  }
}
void parse_location2 (GString *location2)
{
  gchar *find;
  gsize len;

  /* Free the current content stored in current_path. */
  g_list_foreach (current_path2, (GFunc) g_free, NULL);
  g_list_free (current_path2);
  current_path2 = NULL;

  /* Erase all forward slashes from the end of the string. */
  while (location2->str[location2->len - 1] == '/')
    g_string_erase (location2, location2->len - 1, 1);

  /* Parse through the string, splitting it into a GList at the '/' character. */
  while (location2->str[0] == '/')
  {
    while (location2->str[0] == '/')
      g_string_erase (location2, 0, 1);
    find = g_strstr_len (location2->str, location2->len, "/");

    if (find == NULL)
      current_path2 = g_list_append (current_path2, g_strdup (location2->str));
    else
    {
      len = (gsize) (strlen (location2->str) - strlen (find));
      current_path2 = g_list_append (current_path2, g_strndup (location2->str, len));
      g_string_erase (location2, 0, len);
    }
  }
}
/*****************************************************************
 *
 *  stderr_done/stdout_done
 *
 *  Called when the stderr pipe was closed
 *   (ie. p2k-core terminated).
 *
 *  Always returns FALSE (to make it nicer
 *   to use in the callback)
 *
 *****************************************************************/

gboolean stderr_done (GIOChannel *ioc)
{
//	gtk_widget_set_sensitive(button, TRUE);
	gtk_main_quit();
	return FALSE;
}
gboolean stdout_done (GIOChannel *ioc)
{
//	gtk_widget_set_sensitive(button, TRUE);
	gtk_main_quit();
	return FALSE;
}

/*****************************************************************
 *
 *  iofunc_stderr/iofunc_stdout
 *
 *  called when there's data to read from the stderr pipe
 *  or when the pipe is closed (ie. the program terminated)
 *
 *****************************************************************/

gboolean iofunc_stderr (GIOChannel *ioc, GIOCondition cond, gpointer data)
//
// will goes to log window
//
{
	GtkTextBuffer *textbuf;
	GtkTextIter    enditer;
	GdkPixbuf *pixbuf_folder;
	GtkListStore *store;
	GtkTreeIter iter;
	GString *p2kentry;
	char *p2kpath;

	/* data for us to read? */
	if (cond & (G_IO_IN | G_IO_PRI))
	{
		GIOStatus  ret;
		char       buf[BUFSIZE];
		gsize      len = 0;
		GtkWidget *textview;
		textview = glade_xml_get_widget (xml, "textview4");

		memset(buf,0,BUFSIZE);
		ret = g_io_channel_read_chars(ioc, buf, BUFSIZE, &len, NULL);

		if (len <= 0 || ret != G_IO_STATUS_NORMAL)
			return stderr_done(ioc); /* = return FALSE */
		textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		gtk_text_buffer_get_end_iter(textbuf, &enditer);

		if(access("p2kc.glade",R_OK)==0) pixbuf_folder=gdk_pixbuf_new_from_file ("pixmaps/p2kfolder.png", NULL);
		else pixbuf_folder=gdk_pixbuf_new_from_file ("/usr/local/share/p2kc/pixmaps/p2kfolder.png", NULL);

		/* Check if messages are in UTF8. If not, assume
		 *  they are in current locale and try to convert.
		 *  We assume we're getting the stream in a 1-byte
		 *   encoding here, ie. that we do not have cut-off
		 *   characters at the end of our buffer (=BAD)
		 */
		if (g_utf8_validate(buf,len,NULL))
		{
			gtk_text_buffer_insert(textbuf, &enditer, buf, len);
			//
			// evaluate answer from p2k-core
			//
			if(strstr(p2kcommand,"file r")!=NULL && strchr(buf,0x0d)!=0) {
				// delete, refresh left panel
				p2kentry = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)));
				p2kpath=strchr(p2kentry->str,'/');
				store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview1)));
				gtk_list_store_clear(store);
				// add parent ".." folder
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, ICON, pixbuf_folder, FILENAME, "..", SIZE, "< DIR >", -1);
				displayP2kList(GTK_TREE_VIEW(treeview1),p2kpath);
				p2kcommand[0]=0;
			}
			if(strstr(p2kcommand,"file d")!=NULL && strchr(buf,0x0d)!=0) {
				// file download, refresh right panel
				usleep(500000);
				populate_tree_model2(treeview2);
				p2kcommand[0]=0;
			}
			if(strstr(p2kcommand,"seem d")!=NULL && strchr(buf,0x0d)!=0) {
				// seem download, refresh right panel
				usleep(500000);
				populate_tree_model2(treeview2);
				p2kcommand[0]=0;
			}
			if(strstr(p2kcommand,"file u")!=NULL && strchr(buf,0x0d)!=0) {
				// upload, refresh left p2k panel
				p2kentry = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)));
				p2kpath=strchr(p2kentry->str,'/');
				store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview1)));
				gtk_list_store_clear(store);
				// add parent ".." folder
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, ICON, pixbuf_folder, FILENAME, "..", SIZE, "< DIR >", -1);
				displayP2kList(GTK_TREE_VIEW(treeview1),p2kpath);
				p2kcommand[0]=0;
			}
			if(strstr(p2kcommand,"file l")!=NULL ) {
				// step progressbar
				gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pb));
				if(access(FileListAtHome,R_OK)==0 && strchr(buf,0x0d)!=0) {
					// operation complete, read FILELIST file
					//logstat("Loading filelist is completed.\n");
					gtk_entry_set_text (GTK_ENTRY (entry1),"P2k:/");
					store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview1)));
						gtk_list_store_clear(store);
						gtk_list_store_append (store, &iter);
						gtk_list_store_set (store, &iter, ICON, pixbuf_folder, FILENAME, actVol,SIZE, "< DIR >",  -1);
						gtk_tree_view_set_cursor(GTK_TREE_VIEW (treeview1),gtk_tree_path_new_from_string ("0"),NULL,FALSE);
					p2kcommand[0]=0;
				}
				else ;
			}
		}
		else
		{
			const gchar *charset;
			gchar       *utf8;

			(void) g_get_charset(&charset);
			utf8 = g_convert_with_fallback(data, len, "UTF-8", charset, NULL, NULL, NULL, NULL);

			if (utf8)
			{
				gtk_text_buffer_insert(textbuf, &enditer, utf8, len);
				g_free(utf8);
			}
			else
			{
				g_warning("p2k-core Message Output is not in UTF-8 nor in locale charset.\n");
			}
		}

		/* A bit awkward, but better than nothing. Scroll text view to end */
		gtk_text_buffer_get_end_iter(textbuf, &enditer);
		gtk_text_buffer_move_mark_by_name(textbuf, "insert", &enditer);
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview),
		                             gtk_text_buffer_get_mark(textbuf, "insert"),
		                             0.0, FALSE, 0.0, 0.0);
		g_free(data);

	}
	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) return stderr_done(ioc); /* = return FALSE */
	return TRUE;
}
/*****************************************************************
 *
 *  iofunc_stdout/iofunc_stdout
 *
 *  called when there's data to read from the stdout pipe
 *  or when the pipe is closed (ie. the program terminated)
 *
 *****************************************************************/
gboolean iofunc_stdout (GIOChannel *ioc, GIOCondition cond, gpointer data)
//
// will goes to statusbar
//
{
	/* data for us to read? */
	if (cond & (G_IO_IN | G_IO_PRI))
	{
		GIOStatus  ret;
		char       buf[BUFSIZE];
		gsize      len = 0;
		GtkWidget *textview;
		textview = glade_xml_get_widget (xml, "textview4");

		ret = g_io_channel_read_chars(ioc, buf, BUFSIZE, &len, NULL);
		strcpy(strchr(buf,0xa)," ");
		if (len <= 0 || ret != G_IO_STATUS_NORMAL)
			return stderr_done(ioc); /* = return FALSE */
		//
		// evaluate answer
		//
		if (strlen(p2kcommand)==0) { // init
			gtk_statusbar_push(GTK_STATUSBAR(statusbar),SBcontext_id,"Just started...");
		}
		if (strcmp(p2kcommand,"info\n")==0) { //re-read pressed
			gtk_statusbar_push(GTK_STATUSBAR(statusbar),SBcontext_id, buf);
		}
	}
	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		return stdout_done(ioc); /* = return FALSE */
	return TRUE;
}
/*****************************************************************
 *
 *  set_up_io_channel
 *
 *  sets up callback to call whenever something interesting
 *   happens on a pipe (ie. we get data, or p2k-core output)
 *
 *****************************************************************/
GIOChannel *
set_up_io_channel (gint fd, GIOCondition cond, GIOFunc func, gpointer data)
{
	GIOChannel *ioc;

	/* set up handler for data */
	ioc = g_io_channel_unix_new (fd);

	/* Set IOChannel encoding to none to make
	 *  it fit for binary data */
	g_io_channel_set_encoding (ioc, NULL, NULL);
	g_io_channel_set_buffered (ioc, FALSE);

	/* Tell the io channel to close the file descriptor
	 *  when the io channel gets destroyed */
	g_io_channel_set_close_on_unref (ioc, TRUE);

	/* g_io_add_watch() adds its own reference,
	 *  which will be dropped when the watch source
	 *  is removed from the main loop (which happens
	 *  when we return FALSE from the callback) */
	g_io_add_watch (ioc, cond, func, data);
	g_io_channel_unref (ioc);

	return ioc;
}
/*****************************************************************
 *
 *  spawn_p2kcore
 *
 *  spawns p2k-core and hooks up stdout/stderr of p2k-core
 *
 *****************************************************************/
GPid spawn_p2kcore ()
{
	GError  *error = NULL;
	gchar  **argv;
	GPid     child_pid;
	/* It's global, so we can only spawn one p2k-core at
	 *  a time the way we've set up our routines */
	g_assert (databuf == NULL);

	/* Spawn p2k-core */
	argv = g_new (gchar *, 10); // max arg number
	argv=g_strsplit(OptEngineCmd," ",10);
	if (access("p2kc.glade",R_OK)==0) argv[0] = g_strconcat ("./p2k-core/",argv[0],NULL);  /* program to call */
	else argv[0]= g_strconcat ("/usr/local/bin/",argv[0],NULL);  /* program to call */
	if (!g_spawn_async_with_pipes(
		NULL,            /* use current working directory */
		argv,            /* the program we want to run and parameters for it */
		NULL,            /* use the environment variables that are set for the parent */
		(GSpawnFlags) G_SPAWN_SEARCH_PATH   /* look for p2k-core in $PATH */
		      | G_SPAWN_DO_NOT_REAP_CHILD,  /* we'll check the exit status ourself */
		NULL,            /* don't need a child setup function either */
		NULL,            /* and therefore no child setup func data argument */
		&child_pid,      /* where to store the child's PID */
		&stdin_fd,       /* if don't need standard input (=> will be /dev/null) */
		&stdout_fd,      /* where to put p2k-core's stdout file descriptor */
		&stderr_fd,      /* where to put p2k-core's stderr file descriptor */
		&error))
	{
		g_warning ("g_spawn_async_with_pipes() failed: %s\n", error->message);
		g_strfreev (argv);
		g_error_free (error);
		error = NULL;
		return (GPid) 0;
	}

	/* Now use GIOChannels to monitor p2k-core's stdout and stderr */
	set_up_io_channel(stderr_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,
	                  iofunc_stderr, NULL);
	set_up_io_channel(stdout_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,
	                  iofunc_stdout, NULL);

	g_strfreev (argv);
	return (GPid) child_pid;
}
void logstat (gchar *logtext)
{
		GtkWidget *textview;
		GtkTextBuffer *textbuf;
		GtkTextIter    enditer;
		textview = glade_xml_get_widget (xml, "textview4");
		textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		gtk_text_buffer_get_end_iter(textbuf, &enditer);
		gtk_text_buffer_insert(textbuf, &enditer, logtext, strlen(logtext));
		gtk_text_buffer_get_end_iter(textbuf, &enditer);
		gtk_text_buffer_move_mark_by_name(textbuf, "insert", &enditer);
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview),
		                             gtk_text_buffer_get_mark(textbuf, "insert"),
		                             0.0, FALSE, 0.0, 0.0);
}
/*******************************
 *******************************
 *******************************
 *******************************
 *
 *    window event handers
 *
 *******************************
 *******************************
 *******************************
 ******************************/
void on_window_destroy_event (GtkObject *object, gpointer user_data)
{
  strcpy(p2kcommand,OptExitCmd);strcat(p2kcommand,"\n");
  write(stdin_fd,p2kcommand,strlen(p2kcommand));
  strcpy(p2kcommand,"exit\n");
  write(stdin_fd,p2kcommand,strlen(p2kcommand));
  gtk_main_quit();
}
/*******************************
 *
 *    button event handers
 *
 *******************************/
void on_buttonR1_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkListStore *store;
		if (isP2k==0) {
			populate_tree_model1(treeview1);
		}
		else {
				sprintf(p2kcommand,"info %s/%s\n",actVol,OptFilter);
				write(stdin_fd,p2kcommand,strlen(p2kcommand));
				strcpy(p2kcommand,"file l\n");
				write(stdin_fd,p2kcommand,strlen(p2kcommand));
				store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview1)));
				gtk_list_store_clear (store);
				column=gtk_tree_view_get_column(GTK_TREE_VIEW(treeview1),2);
				gtk_tree_view_remove_column(GTK_TREE_VIEW(treeview1),column);
				/* Insert a third tree view column that displays the last modified time/date. */
				renderer = gtk_cell_renderer_text_new ();
				column = gtk_tree_view_column_new_with_attributes (" Owner/Attrib", renderer, "text", MODIFIED, NULL);
				gtk_tree_view_column_set_clickable (column,TRUE);
				gtk_tree_view_column_set_resizable (column,TRUE);
				gtk_tree_view_append_column (GTK_TREE_VIEW (treeview1), column);
		}
}
void on_buttonR2_clicked (GtkButton *button, gpointer user_data)
{
	populate_tree_model2(treeview2);
}
void GetFullPath(GtkWidget *treeview,GtkWidget *entry)
{
	gchar *cp;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		model=gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
		gtk_tree_selection_get_selected(selection, &model, &iter);
		gtk_tree_model_get (model, &iter, FILENAME, &cp, -1);
		// compose full filename
		strcpy(FullPath,g_string_new (gtk_entry_get_text (GTK_ENTRY (entry)))->str);
		if (strlen(FullPath)!=1) strcat(FullPath,"/");
		strcat(FullPath,cp);
		return;
}
void GetP2kFullPath(GtkWidget *treeview,GtkWidget *entry)
{
	gchar *cp,*cp2;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	char FullPath2[1000];

		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		model=gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
		gtk_tree_selection_get_selected(selection, &model, &iter);
		gtk_tree_model_get (model, &iter, FILENAME, &cp, -1);
		// compose full filename
		strcpy(FullPath2,g_string_new (gtk_entry_get_text (GTK_ENTRY (entry)))->str);
		cp2=strchr(FullPath2,'/'); // skip "P2k:"
		strcpy(FullPath,cp2);
		strcat(FullPath,cp);
		return;
}
int IsP2kFolder(GtkWidget *treeview)
{
	gchar *cp;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		model=gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
		gtk_tree_selection_get_selected(selection, &model, &iter);
		gtk_tree_model_get (model, &iter, SIZE, &cp, -1);
		// analyze filesize
		if(strstr(cp,"<")==0) return 0;
		return 1;
}
static void LVforeach_func (GtkTreeModel *model,
               GtkTreePath *path,
               GtkTreeIter *iter,
               gpointer data)
{
  gchar *text;
  int *keycommand;
  keycommand=data;
  gtk_tree_model_get (model, iter, FILENAME, &text, -1);
  switch(*keycommand) {
  	case KEYCMD_EDIT:
  		g_print ("Edit: %s\n", text);
  		break;
  	case KEYCMD_VIEW:
  		g_print ("View: %s\n", text);
  		break;
  	default: g_print ("nonfunctional\n");
  }
  g_free (text);
}
void on_edit_activate()
{
	GtkTreeSelection *selection;
	keycmd=KEYCMD_EDIT;
	logstat("edit pressed\n");
	if(isRight==0) selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview1));
	else selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview2));
	gtk_tree_selection_selected_foreach (selection, LVforeach_func,&keycmd);
}
void on_view_activate()
{
	GtkTreeSelection *selection;
	keycmd=KEYCMD_VIEW;
	logstat("view pressed\n");
	if(isRight==0) selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview1));
	else selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview2));
	gtk_tree_selection_selected_foreach (selection, LVforeach_func,&keycmd);
}
void on_delete_activate(GtkButton *button, GtkWindow *parent)
{
	char command[1000],listfile[1000];
	char buf[BUFSIZE],buf2[BUFSIZE],buf3[BUFSIZE];
	GtkWidget *dialog;
	gint result;
	FILE *hFile, *hFile2;

	if (isP2k==0) { // local delete
		if (isRight==0) GetFullPath(treeview1,entry1);
		else GetFullPath(treeview2,entry2);
		// compose local delete command
		strcpy(command,"rm -r -f "); strcat(command,FullPath);
		dialog=gtk_message_dialog_new(parent,GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,FullPath);
		gtk_window_set_title(GTK_WINDOW(dialog),"Are you sure to delete?");
		result=gtk_dialog_run(GTK_DIALOG(dialog));
		if(result==GTK_RESPONSE_YES) system(command);
		gtk_widget_destroy(GTK_WIDGET(dialog));
		if (isRight==0) populate_tree_model1(treeview1);
		else populate_tree_model2(treeview2);
	}
	else { // p2k operation
		if (isRight==0) { // p2k delete
			GetP2kFullPath(treeview1,entry1);
			// file or folder ?
			if(IsP2kFolder(treeview1)==0) logstat ("file\n");
			else logstat("folder\n");
			// compose p2k-core command
			sprintf(p2kcommand,"file r %s\n",FullPath);
			// send command
			dialog=gtk_message_dialog_new(parent,GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,FullPath);
			gtk_window_set_title(GTK_WINDOW(dialog),"Are you sure to delete?");
			result=gtk_dialog_run(GTK_DIALOG(dialog));
			if(result==GTK_RESPONSE_YES) {
				write(stdin_fd,p2kcommand,strlen(p2kcommand));
				// update P2KFILELIST
				hFile=fopen(FileListAtHome,"rb+");
				sprintf(listfile,"%s2",FileListAtHome);
				hFile2=fopen(listfile,"wb+");
				while (fgets(buf, 256, hFile) != NULL) {
					fgets(buf2, 256, hFile); fgets(buf3, 256, hFile);
					// compare filename
					if (strstr(buf,FullPath)==0) {
						fputs(buf,hFile2);fputs(buf2,hFile2);fputs(buf3,hFile2);
					}
				}
				fclose(hFile);fclose(hFile2);
				sprintf(command,"/bin/cp -f %s %s",listfile,FileListAtHome);
				system(command);
			}
			gtk_widget_destroy(GTK_WIDGET(dialog));
		}
		else { // local delete
			GetFullPath(treeview2,entry2);
			// compose local delete command
			strcpy(command,"rm -r -f "); strcat(command,FullPath);
			dialog=gtk_message_dialog_new(parent,GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,FullPath);
			gtk_window_set_title(GTK_WINDOW(dialog),"Are you sure to delete?");
			result=gtk_dialog_run(GTK_DIALOG(dialog));
			if(result==GTK_RESPONSE_YES) system(command);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			populate_tree_model2(treeview2);
		}
	}
}
void on_seemfunc_activate()
{
	logstat("seemfunc pressed\n");
}
void on_newfolder_activate(GtkButton *button, GtkWindow *parent)
{
GladeXML *xmlF7;
GtkWidget *windowF7;

  if(access("p2kc.glade",R_OK)==0) xmlF7 = glade_xml_new ("F7dlg.glade", NULL, NULL);
  else xmlF7 = glade_xml_new ("/usr/local/share/p2kc/F7dlg.glade", NULL, NULL);
  windowF7 = glade_xml_get_widget (xmlF7, "F7dlg");
  F7Entry = glade_xml_get_widget (xmlF7, "entryF7");
  glade_xml_signal_autoconnect(xmlF7);
  gtk_widget_show(GTK_WIDGET(windowF7));
}
void on_buttonF7OK_clicked (GtkButton *button, gpointer user_data)
{
	gchar *newfolder,*actfolder,*temp;
	GString *command;
	GtkWidget *dialog;
	gint result;
	FILE *hFile;

	//command = g_string_new ("");
	newfolder = g_string_new (gtk_entry_get_text (GTK_ENTRY (F7Entry)))->str;
	if (isP2k==0) {
		if (isRight==0) actfolder =g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)))->str;
		else actfolder =g_string_new (gtk_entry_get_text (GTK_ENTRY (entry2)))->str;
		// compose local create command
//		sprintf(command->str,"mkdir %s/%s",actfolder,newfolder);
		command->str = g_strconcat("mkdir ",actfolder,"/",newfolder,NULL);
		dialog=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,newfolder);
		gtk_window_set_title(GTK_WINDOW(dialog),"Are you sure to create folder?");
		result=gtk_dialog_run(GTK_DIALOG(dialog));
		if(result==GTK_RESPONSE_YES) system(command->str);
		gtk_widget_destroy(GTK_WIDGET(dialog));
		if (isRight==0) populate_tree_model1(treeview1);
		else populate_tree_model2(treeview2);
	}
	else { // p2k create
		actfolder =g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)))->str;
		temp=strchr(actfolder,'/'); // skip "P2k:"
		sprintf(command->str,"%s%s",temp,newfolder);
		dialog=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,command->str);
		gtk_window_set_title(GTK_WINDOW(dialog),"Are you sure to create folder?");
		result=gtk_dialog_run(GTK_DIALOG(dialog));
		if(result==GTK_RESPONSE_YES) {
			// compose p2k command
			sprintf(p2kcommand,"fold c %s\n",command->str);
			write(stdin_fd,p2kcommand,strlen(p2kcommand));
			sprintf(p2kcommand,"file u %s %s/\n",dummyfile,command->str);
			logstat(p2kcommand);
			write(stdin_fd,p2kcommand,strlen(p2kcommand));
			// smartrefresh, actualize FILELIST
			hFile=fopen(FileListAtHome,"ab+");
			strcat(command->str,"/s5vi.url");
			fprintf(hFile,"%s\n%u\n%02x %02x\n",command->str,24,0,7);
			fclose(hFile);
		}
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET (F7Entry)));

}
void on_buttonF7Cancel_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET (F7Entry)));
}
void on_attrib_activate()
{
	logstat("attrib pressed\n");
}
void on_restart_activate()
{
  strcpy(p2kcommand,"mode r\n");
  write(stdin_fd,p2kcommand,strlen(p2kcommand));
}
void on_copy_activate()
{
	gchar *CDest,*cp;
	char command[256];
	//gchar tmp[1000];
	FILE *hFile;
	int filesize;

	if (isP2k==0) { // local copy
		if (isRight==0) {
			GetFullPath(treeview1,entry1);
			CDest = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry2)))->str;
		}
		else {
			GetFullPath(treeview2,entry2);
			CDest = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)))->str;
		}
		// compose local copy command
		//logstat(FullPath);logstat("-->"); logstat(CDest);logstat("\n");
		strcpy(command,"/bin/cp -fr "); strcat(command,FullPath);strcat(command," ");strcat(command,CDest);
		system(command);
		if (isRight==0) populate_tree_model2(treeview2);
		else populate_tree_model1(treeview1);
		g_free(CDest);
	}
	else { // p2k operation
		if (isRight==0) { // download
			GetP2kFullPath(treeview1,entry1);
			CDest = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry2)))->str;
			// compose p2k-core command
			//logstat(FullPath);logstat("-->"); logstat(CDest);logstat("\n");
			sprintf(p2kcommand,"file d %s %s\n",FullPath,CDest);
			// send command
			write(stdin_fd,p2kcommand,strlen(p2kcommand));
			g_free(CDest);
		}
		else { // upload
			GetFullPath(treeview2,entry2);
			CDest=strchr(g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)))->str,'/');// skip "P2k:"
			// compose p2k-core command
			sprintf(p2kcommand,"file u %s %s\n",FullPath,CDest);
			logstat(p2kcommand);logstat("\n");
			// send command
			write(stdin_fd,p2kcommand,strlen(p2kcommand));
			// smartrefresh, actualize FILELIST
			hFile=fopen(FullPath,"rb+");
			fseek(hFile,0,SEEK_END);
			filesize=ftell(hFile);
			fclose(hFile);
			hFile=fopen(FileListAtHome,"ab+");
			cp=strrchr(CDest,'/');*cp=0;
			strcpy(command,CDest);strcat(command,strrchr(FullPath,'/'));
			fprintf(hFile,"%s\n%u\n%02x %02x\n",command,filesize,0,7);
			fclose(hFile);
		}
	}
}
/*******************************
 *
 *    menu event handers
 *
 *******************************/
void on_about_activate()
{
	gchar* authors[] = { "Sándor Ötvös","s5vi.hu@gmail.com", NULL };
	gchar* artists[] = { "s5vi", NULL };
	gchar* comments = { "P2kCommander is a filemanager for Motorola mobile phones." };
	gchar* copyright = { "Copyright (c) s5vi" };
	gchar* documenters[] = { "s5vi",NULL };
	gchar* license = "/*--------------------------------------------------------------------------- \n\
 *      p2kc.c  aka P2kCommander for linux \n\
 * \n\
 *      V_0.6    2008/jan/02 \n\
 * \n\
 *      Copyright 2007 Sandor Otvos <s5vi.hu@gmail.com> \n\
 *      \n\
 *      This program is free software; you can redistribute it and/or modify\n\
 *      it under the terms of the GNU General Public License as published by\n\
 *      the Free Software Foundation; either version 2 of the License, or\n\
 *      (at your option) any later version.\n\
 *      \n\
 *      This program is distributed in the hope that it will be useful,\n\
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
 *      GNU General Public License for more details.\n\
 *      \n\
 *      You should have received a copy of the GNU General Public License\n\
 *      along with this program; if not, write to the Free Software\n\
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,\n\
 *      MA 02110-1301, USA.\n\
 *" ;
	static GdkPixbuf* logo;
	gchar* name = "P2kCommander";
	gchar* translator_credits = "s5vi";
	gchar* website = "http://www.el-co.hu/smf";
	gchar* website_label = "s5vi's website";

	if(access("p2kc.glade",R_OK)==0) logo = gdk_pixbuf_new_from_file ("pixmaps/Splash.jpg",0);
	else logo = gdk_pixbuf_new_from_file ("/usr/local/share/p2kc/pixmaps/Splash.jpg",0);

	gtk_show_about_dialog (NULL,
				 "authors", authors,
				"artists", artists,
				"comments", comments, "copyright", copyright,
				"documenters", documenters, "license", license, "logo", logo, "name", name,
				"translator_credits", translator_credits, "version", version, "website", website,
				"website-label", website_label, NULL);
}
void on_email1_activate()
{
	system("$BROWSER mailto:s5vi.hu@gmail.com");
}
void on_homepage1_activate()
{
	system("$BROWSER http://www.el-co.hu/smf");
}
void on_quit_activate()
{
  strcpy(p2kcommand,OptExitCmd);strcat(p2kcommand,"\n");
  write(stdin_fd,p2kcommand,strlen(p2kcommand));
  strcpy(p2kcommand,"exit\n");
  write(stdin_fd,p2kcommand,strlen(p2kcommand));
  gtk_main_quit();
}
void on_oldseem_activate()
{
GladeXML *xmlseem;
GtkWidget *windowseem;

  if(access("p2kc.glade",R_OK)==0) xmlseem = glade_xml_new ("oldseem.glade", NULL, NULL);
  else xmlseem = glade_xml_new ("/usr/local/share/p2kc/oldseem.glade", NULL, NULL);
  windowseem = glade_xml_get_widget (xmlseem, "oldseemdlg");
  seemnum = glade_xml_get_widget (xmlseem, "entryseemnum");
  seemrec = glade_xml_get_widget (xmlseem, "entryseemrec");
  glade_xml_signal_autoconnect(xmlseem);
  gtk_widget_show(GTK_WIDGET(windowseem));
}
void on_buttonSeemDL_clicked (GtkButton *button, gpointer user_data)
{
gchar *CDest;
GString *SeemNumV;
GString *SeemRecV;

  CDest = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry2)))->str;
  SeemNumV = g_string_new (gtk_entry_get_text (GTK_ENTRY (seemnum)));
  SeemRecV = g_string_new (gtk_entry_get_text (GTK_ENTRY (seemrec)));
  sprintf(p2kcommand,"seem d %s %s %s/\n",SeemNumV->str,SeemRecV->str,CDest);
  write(stdin_fd,p2kcommand,strlen(p2kcommand));
  //gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET (seemnum)));
}
void on_buttonSeemUL_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog;
	gint result;
	gchar *seemtext;
	gchar **seemtarget;
	char *string_ptr;
	seemtarget = g_new (gchar *, 4);

	// get filename
	GetFullPath(treeview2,entry2);
	// check if file is valid seem file?
	if(g_str_has_suffix(FullPath,".seem")==TRUE) {
		sprintf(p2kcommand,"seem u %s\n",FullPath);
		dialog=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,FullPath);
		seemtarget=g_strsplit_set(FullPath,"._",3);
		string_ptr=strrchr(seemtarget[0],'/');
		seemtext=g_strconcat("Upload target is seem:",++string_ptr," record:",seemtarget[1],NULL);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), seemtext);
		gtk_window_set_title(GTK_WINDOW(dialog),"Are you sure to upload this seem?");
		result=gtk_dialog_run(GTK_DIALOG(dialog));
		if(result==GTK_RESPONSE_YES) write(stdin_fd,p2kcommand,strlen(p2kcommand));
		gtk_widget_destroy(GTK_WIDGET(dialog));
		//gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET (seemnum)));
	}
	else {
		dialog = gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                   "Not valid seem file!");
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "On the right panel you must select\n a valid seem file\n just like: xxxx_yyyy.seem");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}
void on_opt_activate()
{
  GladeXML *xmlopt;
  GtkWidget *windowopt;

	if(access("p2kc.glade",R_OK)==0) xmlopt = glade_xml_new ("options.glade", NULL, NULL);
	else xmlopt = glade_xml_new ("/usr/local/share/p2kc/options.glade", NULL, NULL);
	windowopt = glade_xml_get_widget (xmlopt, "optionsdlg");
	glade_xml_signal_autoconnect(xmlopt);
	optentry1= glade_xml_get_widget (xmlopt, "entry1");
	optentry2= glade_xml_get_widget (xmlopt, "entry2");
	optentry3= glade_xml_get_widget (xmlopt, "entry3");
	optentry4= glade_xml_get_widget (xmlopt, "entry4");
	// display config
	gtk_entry_set_text (GTK_ENTRY(optentry1), OptEngineCmd);
	gtk_entry_set_text (GTK_ENTRY(optentry2), OptExitCmd);
	gtk_entry_set_text (GTK_ENTRY(optentry3), OptFilter);
	gtk_entry_set_text (GTK_ENTRY(optentry4), OptOpenCmd);
	gtk_widget_show(GTK_WIDGET(windowopt));
}
void on_buttonOptOK_clicked (GtkButton *button, gpointer user_data)
{
  GKeyFile *keyfile;
  gchar *configdata;
  FILE *hFile;
	// read input fields
	strcpy(OptEngineCmd, g_string_new (gtk_entry_get_text (GTK_ENTRY (optentry1)))->str);
	strcpy(OptExitCmd, g_string_new (gtk_entry_get_text (GTK_ENTRY (optentry2)))->str);
	strcpy(OptFilter, g_string_new (gtk_entry_get_text (GTK_ENTRY (optentry3)))->str);
	strcpy(OptOpenCmd, g_string_new (gtk_entry_get_text (GTK_ENTRY (optentry4)))->str);
	// store config
	keyfile = g_key_file_new ();
	g_key_file_load_from_file (keyfile, ConfigFileAtHome, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
	g_key_file_set_string(keyfile,"P2k-core settings","Engine command",OptEngineCmd);
  	g_key_file_set_string(keyfile,"P2k-core settings","Command before exit",OptExitCmd);
  	g_key_file_set_string(keyfile,"P2k-core settings","Filelist filter",OptFilter);
  	g_key_file_set_string(keyfile,"P2k-core settings","File open command",OptOpenCmd);
  	configdata=g_key_file_to_data(keyfile,NULL,NULL);
	hFile=fopen(ConfigFileAtHome,"wb+");
	fwrite(configdata,1,strlen(configdata),hFile);
	fclose(hFile);
	g_free(configdata);
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET (button)));
}
void on_buttonOptCancel_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET (button)));
}
/*******************************
 *
 *    entry (folder-bar) event handers
 *
 *******************************/
/* Go to the address bar location when the button is clicked. */
void on_entry1_activate ()
{
  GtkWidget *entry1, *treeview1;
  GString *location1;

  entry1 = glade_xml_get_widget (xml, "entry1");
  treeview1 = glade_xml_get_widget (xml, "treeview1");
  location1 = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)));

  /* If the directory exists, visit the entered location. */
  if (g_file_test (location1->str, G_FILE_TEST_IS_DIR))
  {
 //   store_history ();
    parse_location1 (location1);
    populate_tree_model1 (GTK_WIDGET (treeview1));
  }
  else
    file_manager_error ("The location does not exist!");

  g_string_free (location1, TRUE);
}
void on_entry2_activate ()
{
  GtkWidget *entry2, *treeview2;
  GString *location2;

  entry2 = glade_xml_get_widget (xml, "entry2");
  treeview2 = glade_xml_get_widget (xml, "treeview2");
  location2 = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry2)));

  /* If the directory exists, visit the entered location. */
  if (g_file_test (location2->str, G_FILE_TEST_IS_DIR))
  {
 //   store_history ();
    parse_location2 (location2);
    populate_tree_model2 (GTK_WIDGET (treeview2));
  }
  else
    file_manager_error ("The location does not exist!");

  g_string_free (location2, TRUE);
}
/*******************************
 *
 *    combobox (drive selection) event handers
 *
 *******************************/
void on_combobox1_changed(GtkComboBox *widget, gpointer user_data)
{
  GString *location1;
  GtkTreeViewColumn *column;
  GtkListStore *store;
  char *cp;

		gtk_widget_grab_focus (GTK_WIDGET (treeview1));
		location1 = g_string_new (gtk_combo_box_get_active_text(widget));
		if(strncmp ("p2k:",location1->str,4)!=0) {
			// local folder
			isP2k=0;
			column=gtk_tree_view_get_column(GTK_TREE_VIEW(treeview1),2);
			gtk_tree_view_column_set_title(column," Last Modified");
			parse_location1 (location1);
			populate_tree_model1 (GTK_WIDGET (treeview1));
		}
		else {
			// p2k volume
			isP2k=1;cp=strchr(location1->str,'/');cp++;
			strcpy(actVol,cp);
			if (strcmp(actVol,"seems")!=0) {
				sprintf(p2kcommand,"info %s/%s\n",actVol,OptFilter);
				write(stdin_fd,p2kcommand,strlen(p2kcommand));
				strcpy(p2kcommand,"file l\n");
				write(stdin_fd,p2kcommand,strlen(p2kcommand));
				store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview1)));
				gtk_list_store_clear (store);
				column=gtk_tree_view_get_column(GTK_TREE_VIEW(treeview1),2);
				gtk_tree_view_column_set_title(column," Attrib/Owner");
			}
			else { // seems
				logstat("Seems as filesystem not implemented yet.\n");
			}
		}
		g_string_free (location1, TRUE);
		isRight=0;
}
void on_combobox2_changed(GtkComboBox *widget, gpointer user_data)
{
  GString *location2;
  GtkTreeViewColumn *column;

		gtk_widget_grab_focus (GTK_WIDGET (treeview2));
		location2 = g_string_new (gtk_combo_box_get_active_text(widget));
//			parse_location2 (location2);
//			populate_tree_model2 (GTK_WIDGET (treeview2));
			column=gtk_tree_view_get_column(GTK_TREE_VIEW(treeview2),2);
			gtk_tree_view_column_set_title(column," Last Modified");
			parse_location2 (location2);
			populate_tree_model2 (GTK_WIDGET (treeview2));
		g_string_free (location2, TRUE);
		isRight=1;
}
void on_combobox1_grab_focus(GtkComboBox *widget, gpointer user_data)
{
	gtk_combo_box_popup(GTK_COMBO_BOX(cb1));
}
void on_combobox2_grab_focus(GtkComboBox *widget, gpointer user_data)
{
	gtk_combo_box_popup(GTK_COMBO_BOX(cb2));
}
/*******************************
 *
 *    textview (logwindow) event handers
 *
 *******************************/
//
// execute p2k-core command typed into log window
//
void on_textview4_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	gchar buff[64];
	if(event->keyval==GDK_Return) {
		strcat(p2kcommand,"\n");
		write(stdin_fd,p2kcommand,strlen(p2kcommand));
		strcpy(p2kcommand,"");
	}
	if(event->keyval==GDK_Escape) {
		if (isRight==1) GetFullPath(treeview2,entry2);
		else {
			if (isP2k==1) GetP2kFullPath(treeview1,entry1);
			else GetFullPath(treeview1,entry1);
		}
		strcat(p2kcommand,FullPath);
		logstat(FullPath);
	}
	buff[0]=gdk_keyval_to_unicode(event->keyval);buff[1]=0;
	strcat(p2kcommand,buff);
	logstat("");
}
/*******************************
 *
 *    Treeview popup menu events
 *
 *******************************/
void view_popup_menu (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
  {
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
  }
/*******************************
 *
 *    listview (filelist) event handers
 *
 *******************************/
/* When a row is activated, either move to the desired location or show
 * more information if the file is not a directory. */
void on_row_activated1 (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *file;
  char *cp,*p2kpath;

  char  buf3[BUFSIZE];
  GdkPixbuf *pixbuf;
  GtkListStore *store;
  GString *p2kentry;

  isRight=0;
  if(access("p2kc.glade",R_OK)==0) pixbuf=gdk_pixbuf_new_from_file ("pixmaps/p2kfolder.png", NULL);
  else pixbuf=gdk_pixbuf_new_from_file ("/usr/local/share/p2kc/pixmaps/p2kfolder.png", NULL);
  model = gtk_tree_view_get_model (treeview);
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, FILENAME, &file, -1);
    /* Move to the parent directory. */
    if (g_ascii_strcasecmp ("..", file) == 0) {
    	if (isP2k==0) {
      		GList *last1 = g_list_last (current_path1);
      		current_path1 = g_list_remove_link (current_path1, last1);
      		g_list_free_1 (last1);
      		populate_tree_model1 (GTK_WIDGET (treeview));
    	}
    	else { //p2k drive goto parent
			store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview1)));
    		p2kentry = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)));
    		if (strlen(p2kentry->str)>5) {
    			cp=strrchr(p2kentry->str,'/');*cp=0;
    			cp=strrchr(p2kentry->str,'/');cp++;*cp=0;
    		}
   			gtk_entry_set_text (GTK_ENTRY (entry1), p2kentry->str);
    		p2kpath=strchr(p2kentry->str,'/');
    		gtk_list_store_clear(store);
    		// add parent ".." folder
   			gtk_list_store_append (store, &iter);
    		if(strlen(p2kpath)>1) gtk_list_store_set (store, &iter, ICON, pixbuf, FILENAME, "..", SIZE, "< DIR >", -1);
    		else gtk_list_store_set (store, &iter, ICON, pixbuf, FILENAME, actVol, SIZE, "< DIR >", -1);
			displayP2kList(GTK_TREE_VIEW(treeview1),p2kpath);
    	}
    }
    /* Move to the chosen directory or show more information about the file. */
    else {
		gchar *location = path_to_string1 ();
		if (isP2k==0) {
		  if (g_file_test (g_strconcat (location, "/", file, NULL), G_FILE_TEST_IS_DIR))  {
			current_path1 = g_list_append (current_path1, file);
			populate_tree_model1 (GTK_WIDGET (treeview));
		  }
		  else { system(g_strconcat (OptOpenCmd," ",location, "/", file, NULL));
		  }
    	}
    	else { // p2k drive !!
			store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview1)));
    		p2kentry = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)));
    		// check file or folder ?
    		if(isP2kFolder(GTK_TREE_VIEW(treeview1))==1) { ; }
    		else { // folder , update entry
	    		sprintf(buf3,"%s%s/",p2kentry->str,file);
    			gtk_entry_set_text (GTK_ENTRY (entry1), buf3);
    		}
    		p2kentry = g_string_new (gtk_entry_get_text (GTK_ENTRY (entry1)));
    		p2kpath=strchr(p2kentry->str,'/');
    		gtk_list_store_clear(store);
    		// add parent ".." folder
    		gtk_list_store_append (store, &iter);
    		gtk_list_store_set (store, &iter, ICON, pixbuf, FILENAME, "..", SIZE, "< DIR >", -1);
			displayP2kList(GTK_TREE_VIEW(treeview1),p2kpath);
    	}
    }
  }
}
void on_row_activated2 (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *file;

  isRight=1;
  model = gtk_tree_view_get_model (treeview);
  if (gtk_tree_model_get_iter (model, &iter, path))
  {
    gtk_tree_model_get (model, &iter, FILENAME, &file, -1);

    /* Move to the parent directory. */
    if (g_ascii_strcasecmp ("..", file) == 0)
    {
      GList *last2 = g_list_last (current_path2);
      current_path2 = g_list_remove_link (current_path2, last2);
      g_list_free_1 (last2);
      populate_tree_model2 (GTK_WIDGET (treeview));
    }
    /* Move to the chosen directory or show more information about the file. */
    else
    {
      gchar *location = path_to_string2 ();

      if (g_file_test (g_strconcat (location, "/", file, NULL), G_FILE_TEST_IS_DIR))
      {
        current_path2 = g_list_append (current_path2, file);
        populate_tree_model2 (GTK_WIDGET (treeview));
      }
      else {
      	system(g_strconcat (OptOpenCmd," ",location, "/", file, NULL));
      }
    }
  }
}
void on_treeview1_focus (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column)
{
	isRight=0;
}
void on_treeview2_focus (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column)
{
	isRight=1;
}
gboolean on_treeview1_button_press_event(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
 {
    isRight=0;
 	/* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
      /* optional: select row if no row is selected or only
       *  one other row is selected (will only do something
       *  if you set a tree selection mode as described later
       *  in the tutorial) */
      if (1) {
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
        /* Note: gtk_tree_selection_count_selected_rows() does not
         *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
        if (gtk_tree_selection_count_selected_rows(selection)  <= 1) {
           GtkTreePath *path;
           /* Get tree path for row that was clicked */
           if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),(gint) event->x,(gint) event->y,&path, NULL, NULL, NULL)) {
             gtk_tree_selection_unselect_all(selection);
             gtk_tree_selection_select_path(selection, path);
             gtk_tree_path_free(path);
           }
        }
      } /* end of optional bit */
      view_popup_menu(treeview, event, userdata);
      return TRUE; /* we handled this */
 	}
 return FALSE; /* we did not handle this */
 }
gboolean on_treeview2_button_press_event(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
 {
    isRight=1;
 	/* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
      /* optional: select row if no row is selected or only
       *  one other row is selected (will only do something
       *  if you set a tree selection mode as described later
       *  in the tutorial) */
      if (1) {
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
        /* Note: gtk_tree_selection_count_selected_rows() does not
         *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
        if (gtk_tree_selection_count_selected_rows(selection)  <= 1) {
           GtkTreePath *path;
           /* Get tree path for row that was clicked */
           if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),(gint) event->x,(gint) event->y,&path, NULL, NULL, NULL)) {
             gtk_tree_selection_unselect_all(selection);
             gtk_tree_selection_select_path(selection, path);
             gtk_tree_path_free(path);
           }
        }
      } /* end of optional bit */
      view_popup_menu(treeview, event, userdata);
      return TRUE; /* we handled this */
 	}
 return FALSE; /* we did not handle this */
 }
gboolean on_treeview1_key_press_event(GtkWidget *treeview, GdkEventKey *event, gpointer userdata)
 {
 	switch(event->keyval) {
        case GDK_Tab:
        	isRight=1;
			gtk_widget_grab_focus (GTK_WIDGET (treeview2));
            return TRUE;
            break;
        case GDK_Left:
        	logstat("Key_left pressed\n");
            return TRUE;
            break;
        default:
            return FALSE;
    }
 }
gboolean on_treeview2_key_press_event(GtkWidget *treeview, GdkEventKey *event, gpointer userdata)
 {
 	switch(event->keyval) {
        case GDK_Tab:
        	isRight=0;
			gtk_widget_grab_focus (GTK_WIDGET (treeview1));
            return TRUE;
            break;
        case GDK_Left:
        	logstat("Key_left pressed\n");
            return TRUE;
            break;
        default:
            return FALSE;
    }
 }
/*******************************
 *
 *    P2k filelist misc functions
 *
 *******************************/
int isP2kFolder(GtkTreeView *treeview)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *text;

		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		model=gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
		gtk_tree_selection_get_selected(selection, &model, &iter);
		gtk_tree_model_get (model, &iter, SIZE, &text, -1);
		if (strcmp(text,"< DIR >")==0) return 0;
		else return 1;
}
int displayP2kList (GtkTreeView *treeview, char *p2kpath)
{
  FILE *hFile;
  char     buf[BUFSIZE],buf2[BUFSIZE],buf3[BUFSIZE];
  char *cp,*filename;
  GdkPixbuf *pixbuf;
  GtkTreeIter iter;
  gchar *text;
  gsize      len = 0;
  GtkTreeModel *model;
  GtkListStore *store;
  gboolean res;

			store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));
			model = gtk_tree_view_get_model (treeview);
			hFile=fopen(FileListAtHome,"r");
			// loop thru filenames
			while (fgets(buf, 256, hFile) != NULL) {
				fgets(buf2, 256, hFile); fgets(buf3, 256, hFile);
				cp=strchr(buf,0x0a); *cp=0; cp=strchr(buf2,0x0a); *cp=0; cp=strchr(buf3,0x0a); *cp=0;
				// compare filename with folder path "/a/ALARMCLOCK" "/a/"
				if (strstr(buf,p2kpath)!=0) {
					filename=buf+strlen(p2kpath);
					cp=strchr(filename,'/'); //folder or file?
					if (cp!=0) {
						*cp=0;
						strcpy(buf2,"< DIR >");strcpy(buf3,"");
						if(access("p2kc.glade",R_OK)==0) pixbuf=gdk_pixbuf_new_from_file ("pixmaps/p2kfolder.png", NULL);
						else pixbuf=gdk_pixbuf_new_from_file ("/usr/local/share/p2kc/pixmaps/p2kfolder.png", NULL);
					}
					else {
						if(access("p2kc.glade",R_OK)==0) pixbuf=gdk_pixbuf_new_from_file ("pixmaps/p2kfile.png", NULL);
						else pixbuf=gdk_pixbuf_new_from_file ("/usr/local/share/p2kc/pixmaps/p2kfile.png", NULL);
					}
					g_utf8_validate(filename,len,NULL); g_utf8_validate(buf2,len,NULL); g_utf8_validate(buf3,len,NULL);
					// check if already present
					res=gtk_tree_model_get_iter_first(model,&iter); // check whole listview
					gtk_tree_model_get (model, &iter, FILENAME, &text, -1);
					if(strcmp(filename,text)!=0) {
						while (res==TRUE) {
							gtk_tree_model_get (model, &iter, FILENAME, &text, -1);
							if(strcmp(filename,text)==0) break;
							res=gtk_tree_model_iter_next(model,&iter);
						}
						if (res==FALSE) {
							gtk_list_store_append (store, &iter);
							gtk_list_store_set (store, &iter, ICON, pixbuf, FILENAME, filename,SIZE, buf2, MODIFIED, buf3, -1);
						}
					}
				}
			}
			fclose(hFile);
			gtk_tree_view_set_cursor(GTK_TREE_VIEW (treeview),gtk_tree_path_new_from_string ("0"),NULL,FALSE);
return 0;
}
