#pragma once

#include <gtk/gtk.h>
#include "../types/Achievement.h"

class GtkAchievementBoxRow {
public:
    GtkAchievementBoxRow(const Achievement_t& data);
    ~GtkAchievementBoxRow();

    /**
     * Unlock the achievement represented by this row if it is locked
     */
    void unlock();
    /**
     * Lock the achievement represented by this row if it is unlocked
     */
    void lock();
    /**
     * Invert (or toggle) the achievement represented by this row
     */
    void invert();

    GtkWidget* get_main_widget() { return m_main_box; };

    Achievement_t m_data;
    bool m_ignore_toggle;

private:
    GtkWidget *m_main_box;
    GtkWidget *m_lock_unlock_button; // Cache this for easy lookups
};