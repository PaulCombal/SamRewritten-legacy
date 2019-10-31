#pragma once

#include <gtk/gtk.h>
#include "../types/Achievement.h"

class GtkAchievementBoxRow {
public:
    GtkAchievementBoxRow(const Achievement_t& data);
    ~GtkAchievementBoxRow();

    GtkWidget* get_main_widget() { return m_main_box; };

private:
    Achievement_t m_data;

    GtkWidget *m_main_box;
};