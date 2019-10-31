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

    Achievement_t get_achievement() { return m_data; };
    GtkWidget* get_main_widget() { return m_main_box; };

    // TODO: pull this data back into private when transformed
    // to proper GTKMM C++ signal handlers
    Achievement_t m_data;
    // Whether the achievement has a pending change
    // This simplifies logic with the toggle button.
    bool m_active;
    bool m_ignore_toggle;

private:
    GtkWidget *m_main_box;
    GtkWidget *m_lock_unlock_button; // Cache this for easy lookups
};