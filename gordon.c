#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

#define IMAGE     "gordon.png"
#define ANIMATION "gordon.gif"

/*  Gordon's animated progressbar
                                
    gcc -Wall -Wextra -o gordon gordon.c `pkg-config --cflags --libs gtk+-3.0`
    Code: Misko 2021
                                 depends: gcc libgtk-3-dev (3.24)
*/

guint threadID = 0;
guint anim_threadID = 0;

typedef struct {
    GtkWidget              *progress_bar;
    GtkWidget              *button1;
    GdkPixbufAnimation     *animation;
    GdkPixbufAnimationIter *iter;
    GtkWidget              *progress_image;
    GtkWidget              *progress_label;
    GtkStyleProvider       *progressbar_style_provider;
} app_widgets;


void destroy_handler (GtkApplication* app, gpointer data)
{
    (void) app;
    g_application_quit(G_APPLICATION (data));
}


void
stop_progress_cb (gpointer user_data)
{
  app_widgets *widgets = (app_widgets *) user_data;
  gdouble fraction;
  fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR(widgets->progress_bar));
  gtk_image_set_from_file(GTK_IMAGE(widgets->progress_image),
                                                          IMAGE);
  g_print("squash progress: %.0f %%\n", fraction*100);
}


static gboolean
fill (gpointer  user_data)
{
  app_widgets *widgets = (app_widgets *) user_data;
  GtkAllocation *alloc = g_new(GtkAllocation, 1);
  gdouble fraction;

  fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR(widgets->progress_bar));

   if (fraction > 0.999) {
      fraction = 0;
   }

  /* Increase the bar by 1% each time this function is called */
  if (fraction < 0.999)
    fraction += 0.01;

  gtk_widget_get_allocation (widgets->progress_bar, alloc);
  
  gtk_widget_set_margin_start(widgets->progress_image,
                          alloc->width*fraction);
  g_free(alloc);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(widgets->progress_bar),
                                fraction);
  gchar temp[256];
  memset(&temp, 0x0, 256);
  if (fraction > 0.999) {
     snprintf(temp, 255, "Squashed: %.0f %%", fraction*100);
     gtk_button_set_label (GTK_BUTTON(widgets->button1),
                                              "Squash");
     threadID = 0;
     anim_threadID = 0;
   }
   else {
     snprintf(temp, 255, "Squashing: %.0f %%", fraction*100);
   }
  gtk_label_set_text (GTK_LABEL(widgets->progress_label),
                                                   temp);
  /* Ensures that the fraction stays below 1.0 */
  if (fraction < 0.999)
    return TRUE;

  return FALSE;
}

static gboolean
animate (gpointer  user_data)
{
  app_widgets *widgets = (app_widgets *) user_data;

  gdk_pixbuf_animation_iter_advance (widgets->iter, NULL);

  gtk_image_set_from_pixbuf(GTK_IMAGE(widgets->progress_image),
                            gdk_pixbuf_animation_iter_get_pixbuf(widgets->iter));

  if (anim_threadID != 0)
    return TRUE;
  
  return FALSE;
}


void
button1_clicked_cb (GtkButton *button,
                    app_widgets *widgets)
{
    if (threadID == 0)
    {
        threadID = g_timeout_add_full (0, 100, fill,
                      widgets, stop_progress_cb);
        gtk_button_set_label (button,
                      "Pause");
        widgets->iter = gdk_pixbuf_animation_get_iter (widgets->animation,
                                                                    NULL);
        guint64 delay = gdk_pixbuf_animation_iter_get_delay_time(widgets->iter);

        anim_threadID = g_timeout_add(delay, animate, widgets);
    }
    else
    {
        GSource *source = g_main_context_find_source_by_id(NULL,
                                                       threadID);
        GSource *anim_source = g_main_context_find_source_by_id(NULL,
                                                       anim_threadID);
         if (source)
         {
            g_source_destroy (source);
         }
         if (anim_source)
         {
            g_source_destroy (anim_source);
         }
         threadID = 0;
         anim_threadID = 0;
         gtk_button_set_label (button,
                      "Move");
         animate(widgets);

         g_object_unref(widgets->iter);
    }
}

static void
progress_bar_size_allocate (GtkWidget       *progress_bar,
                              GdkRectangle    *allocation,
                              gpointer         user_data)
{
  (void) progress_bar;
  app_widgets *widgets = (app_widgets *) user_data;

  gdouble fraction;
  
  /*Get the current progress*/
  fraction = gtk_progress_bar_get_fraction
              (GTK_PROGRESS_BAR(widgets->progress_bar));
  if (fraction == 0)
  { 
     fraction = 0.01;
  }
  /*Set the margin of animation when the window width changes*/
  gtk_widget_set_margin_start(widgets->progress_image,
                               allocation->width*fraction);
  if (fraction > 0.999)
     gtk_widget_set_margin_start(widgets->progress_image, 1);
}


static void
progress_image_size_allocate (GtkWidget   *animation,
                              GdkRectangle    *allocation,
                              gpointer         user_data)
{
  (void) animation;
  app_widgets   *widgets = (app_widgets *) user_data;
  GtkAllocation *progress_bar_allocation = g_new(GtkAllocation, 1);
  char          *css_text;

  gtk_widget_get_allocation (widgets->progress_bar,
                             progress_bar_allocation);

  progress_bar_allocation->height = allocation->height;

  css_text = g_strdup_printf ("progressbar trough,\n"
                              "progressbar progress\n"
                              "{\n"
                              "  min-height: %dpx;\n"
                              "}\n",
                              allocation->height);

  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER
                                   (widgets->progressbar_style_provider),
                                   css_text, -1, NULL);
  g_free(progress_bar_allocation);
  g_free (css_text);
}


static void
activate (GtkApplication  *app,
          app_widgets     *widgets,
          gpointer        user_data)
{
  (void) user_data;
  GtkSizeGroup       *size_group;
  GtkWidget          *window;
  GtkWidget          *grid;
  GtkWidget          *button2;
  GtkWidget          *progress_overlay;
  GError              *error = NULL;
  GtkWidget           *box;

  gdouble fraction = 0.0;

  window = gtk_application_window_new (app);

  gtk_window_set_title (GTK_WINDOW (window),
                               "Gordon's Progress Bar");
  gtk_window_set_default_size (GTK_WINDOW (window),
                               420, 60);
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 10);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 10);
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);

  widgets->progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR(widgets->progress_bar),
                                 FALSE);
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR(widgets->progress_bar),
                                 FALSE);            
  /* Fill in the given fraction of the bar.
    It has to be between 0.0-1.0 inclusive */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widgets->progress_bar),
                                 fraction);
  widgets->progress_label = gtk_label_new ("Gordon is ready");

  widgets->button1 = gtk_button_new_with_label ("Squash");
  button2 = gtk_button_new_with_label ("Cancel");

  progress_overlay = gtk_overlay_new ();
  gtk_widget_set_hexpand (progress_overlay, TRUE);
  gtk_widget_set_vexpand (progress_overlay, FALSE);
  gtk_container_add (GTK_CONTAINER (progress_overlay),
                     widgets->progress_bar);
  widgets->animation = gdk_pixbuf_animation_new_from_file(ANIMATION, &error);
  if (error) {
      g_warning("No image found\n*ERROR %s\n", error->message);
      destroy_handler(NULL, app);
  }
   gtk_window_set_icon (GTK_WINDOW(window),
                        gdk_pixbuf_new_from_file(IMAGE, NULL));
  widgets->progress_image = gtk_image_new_from_file (IMAGE);
  gtk_widget_set_vexpand(widgets->progress_bar, FALSE);
  gtk_widget_set_name (widgets->progress_image,
                         "progress-image");
  /*create a box container for the image*/
  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX(box), widgets->progress_image,
                      FALSE, FALSE, 2); 
  gtk_widget_set_halign (widgets->progress_image,
                        GTK_ALIGN_START);
  gtk_overlay_add_overlay (GTK_OVERLAY (progress_overlay),
                           box);
  gtk_overlay_set_overlay_pass_through (GTK_OVERLAY (progress_overlay),
                           box, TRUE);
  size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (size_group, widgets->progress_bar);
  gtk_size_group_add_widget (size_group, box);

  widgets->progressbar_style_provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_style_context_add_provider (gtk_widget_get_style_context (widgets->progress_bar),
                                  widgets->progressbar_style_provider,
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_grid_attach (GTK_GRID (grid), progress_overlay, 0, 0, 4, 1);
  gtk_grid_attach (GTK_GRID (grid), widgets->progress_label, 0, 1, 2, 1);
  gtk_grid_attach (GTK_GRID (grid), widgets->button1, 2, 1, 1, 1);
  gtk_grid_attach_next_to (GTK_GRID (grid), button2, widgets->button1,
                           GTK_POS_RIGHT, 1, 1);
  gtk_container_add (GTK_CONTAINER (window), grid);
  gtk_container_set_border_width(GTK_CONTAINER(grid),12);
  gtk_container_set_border_width(GTK_CONTAINER(window),5);

  g_signal_connect (widgets->progress_image, "size-allocate",
                    G_CALLBACK (progress_image_size_allocate),
                     widgets);
  g_signal_connect (widgets->progress_bar, "size-allocate",
                    G_CALLBACK (progress_bar_size_allocate),
                     widgets);
  g_signal_connect (widgets->button1, "clicked",
                    G_CALLBACK (button1_clicked_cb), widgets);
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (destroy_handler), app);
  g_signal_connect (G_OBJECT(window), "destroy",
                    G_CALLBACK (destroy_handler), app);
  gtk_widget_show_all (window);

}


int
main (int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.gordon_bar", G_APPLICATION_FLAGS_NONE);
  app_widgets *widgets = g_slice_new(app_widgets);
  g_signal_connect (app, "activate", G_CALLBACK (activate), widgets);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_slice_free(app_widgets, widgets);
  g_object_unref (app);

  return status;
}
