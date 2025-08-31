#include "drag-drop.h"

#include <filesystem>

#include "../../src/utils/storage.h"

extern std::vector<App> apps;
extern StorageManager<LauncherConfig> config;

namespace Pinned {
extern bool has(std::string_view file);
extern void insertAt(const std::string& filename, int index);
extern void reorder(const std::string& filename, int newIndex);
extern void toggle(std::string_view file, bool force);
}

const GtkTargetEntry DragDrop::dragTargets[] = {
    {(gchar*)"application/x-pinned-app", GTK_TARGET_SAME_APP, 0}};
const gint DragDrop::nDragTargets =
    sizeof(dragTargets) / sizeof(dragTargets[0]);

DragDrop::DragDrop(Launcher* launcher) : launcher(launcher) {}

void DragDrop::setupDragAndDrop(EventBox* eventBox, App& app) {
  gtk_drag_source_set(eventBox->widget, GDK_BUTTON1_MASK, dragTargets,
                      nDragTargets, GDK_ACTION_MOVE);

  g_signal_connect(eventBox->widget, "drag-begin", G_CALLBACK(onDragBegin),
                   &app);
  g_signal_connect(eventBox->widget, "drag-end", G_CALLBACK(onDragEnd), &app);
  g_signal_connect(eventBox->widget, "drag-data-get", G_CALLBACK(onDragDataGet),
                   &app);
}

void DragDrop::setupDropTargets(FlowBox* pinGrid, FlowBox* appGrid) {
  gtk_drag_dest_set(pinGrid->widget, GTK_DEST_DEFAULT_ALL, dragTargets,
                    nDragTargets, GDK_ACTION_MOVE);

  g_signal_connect(pinGrid->widget, "drag-enter",
                   G_CALLBACK(onPinnedGridDragEnter), launcher);
  g_signal_connect(pinGrid->widget, "drag-leave",
                   G_CALLBACK(onPinnedGridDragLeave), launcher);
  g_signal_connect(pinGrid->widget, "drag-data-received",
                   G_CALLBACK(onPinnedGridDragDataReceived), launcher);

  gtk_drag_dest_set(appGrid->widget, GTK_DEST_DEFAULT_ALL, dragTargets,
                    nDragTargets, GDK_ACTION_MOVE);

  g_signal_connect(appGrid->widget, "drag-enter",
                   G_CALLBACK(onAppGridDragEnter), launcher);
  g_signal_connect(appGrid->widget, "drag-leave",
                   G_CALLBACK(onAppGridDragLeave), launcher);
  g_signal_connect(appGrid->widget, "drag-data-received",
                   G_CALLBACK(onAppGridDragDataReceived), launcher);
}

void DragDrop::onDragBegin(GtkWidget* widget, GdkDragContext* context,
                           gpointer userData) {
  App* app = static_cast<App*>(userData);
  app->element->addState(GTK_STATE_FLAG_ACTIVE);
  app->element->addClass("dragging");

  if (!app->icon.empty()) {
    GtkWidget* dragIcon = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(dragIcon, 40, 40);

    GtkCssProvider* provider = gtk_css_provider_new();
    std::string css =
        ".drag-icon { background-image: url('" + app->icon + "'); }";

    gtk_css_provider_load_from_data(provider, css.c_str(), -1, nullptr);

    GtkStyleContext* context_style = gtk_widget_get_style_context(dragIcon);
    gtk_style_context_add_provider(context_style, GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_class(context_style, "drag-icon");

    gtk_widget_show_all(dragIcon);
    gtk_drag_set_icon_widget(context, dragIcon, 20, 20);

    g_object_unref(provider);
  } else {
    gtk_drag_set_icon_default(context);
  }
}

void DragDrop::onDragEnd(GtkWidget* widget, GdkDragContext* context,
                         gpointer userData) {
  App* app = static_cast<App*>(userData);
  app->element->removeState(GTK_STATE_FLAG_ACTIVE);
  app->element->removeClass("dragging");
}

void DragDrop::onDragDataGet(GtkWidget* widget, GdkDragContext* context,
                             GtkSelectionData* data, guint info, guint time,
                             gpointer userData) {
  App* app = static_cast<App*>(userData);
  std::string filename = std::filesystem::path(app->file).filename();
  gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8,
                         (const guchar*)filename.c_str(), filename.length());
}

gboolean DragDrop::onPinnedGridDragEnter(GtkWidget* widget,
                                         GdkDragContext* context, guint time,
                                         gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->pinGrid->addClass("drag-over");
  return TRUE;
}

void DragDrop::onPinnedGridDragLeave(GtkWidget* widget, GdkDragContext* context,
                                     guint time, gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->pinGrid->removeClass("drag-over");
}

void DragDrop::onPinnedGridDragDataReceived(GtkWidget* widget,
                                            GdkDragContext* context, gint x,
                                            gint y, GtkSelectionData* data,
                                            guint info, guint time,
                                            gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->pinGrid->removeClass("drag-over");

  if (gtk_selection_data_get_length(data) <= 0) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  std::string draggedFilename((const char*)gtk_selection_data_get_data(data),
                              gtk_selection_data_get_length(data));

  GtkFlowBoxChild* childAtPos =
      gtk_flow_box_get_child_at_pos(GTK_FLOW_BOX(widget), x, y);
  int dropIndex = -1;

  if (childAtPos) {
    dropIndex = gtk_flow_box_child_get_index(childAtPos);
  } else {
    dropIndex = launcher->pinGrid->children.size();
  }

  if (Pinned::has(draggedFilename))
    Pinned::reorder(draggedFilename, dropIndex);
  else
    Pinned::insertAt(draggedFilename, dropIndex);

  launcher->update();

  gtk_drag_finish(context, TRUE, FALSE, time);
}

gboolean DragDrop::onAppGridDragEnter(GtkWidget* widget,
                                      GdkDragContext* context, guint time,
                                      gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->grid->addClass("drag-over");
  return TRUE;
}

void DragDrop::onAppGridDragLeave(GtkWidget* widget, GdkDragContext* context,
                                  guint time, gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->grid->removeClass("drag-over");
}

void DragDrop::onAppGridDragDataReceived(GtkWidget* widget,
                                         GdkDragContext* context, gint x,
                                         gint y, GtkSelectionData* data,
                                         guint info, guint time,
                                         gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->grid->removeClass("drag-over");

  if (gtk_selection_data_get_length(data) <= 0) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  std::string draggedFilename((const char*)gtk_selection_data_get_data(data),
                              gtk_selection_data_get_length(data));

  if (Pinned::has(draggedFilename)) {
    Pinned::toggle(draggedFilename, false);
    launcher->update();
  }

  gtk_drag_finish(context, TRUE, FALSE, time);
}