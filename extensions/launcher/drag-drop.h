#pragma once

#include <gtk/gtk.h>

#include "../../src/element.h"
#include "main.h"

class DragDrop {
 private:
  Launcher* launcher;

  static const GtkTargetEntry dragTargets[];
  static const gint nDragTargets;

 public:
  explicit DragDrop(Launcher* launcher);

  void setupDragAndDrop(EventBox* eventBox, App& app);
  void setupDropTargets(FlowBox* pinGrid, FlowBox* appGrid);

 private:
  static void onDragBegin(GtkWidget* widget, GdkDragContext* context,
                          gpointer userData);
  static void onDragEnd(GtkWidget* widget, GdkDragContext* context,
                        gpointer userData);
  static void onDragDataGet(GtkWidget* widget, GdkDragContext* context,
                            GtkSelectionData* data, guint info, guint time,
                            gpointer userData);

  // Pinning and reordering
  static gboolean onPinnedGridDragEnter(GtkWidget* widget,
                                        GdkDragContext* context, guint time,
                                        gpointer userData);
  static void onPinnedGridDragLeave(GtkWidget* widget, GdkDragContext* context,
                                    guint time, gpointer userData);
  static void onPinnedGridDragDataReceived(GtkWidget* widget,
                                           GdkDragContext* context, gint x,
                                           gint y, GtkSelectionData* data,
                                           guint info, guint time,
                                           gpointer userData);

  // Unpinning
  static gboolean onAppGridDragEnter(GtkWidget* widget, GdkDragContext* context,
                                     guint time, gpointer userData);
  static void onAppGridDragLeave(GtkWidget* widget, GdkDragContext* context,
                                 guint time, gpointer userData);
  static void onAppGridDragDataReceived(GtkWidget* widget,
                                        GdkDragContext* context, gint x, gint y,
                                        GtkSelectionData* data, guint info,
                                        guint time, gpointer userData);
};
