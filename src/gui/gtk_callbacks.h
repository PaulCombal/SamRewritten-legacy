#pragma once
#include <gtk/gtk.h>

/**************************************
 * Gtk Callbacks
 **************************************/
extern "C" 
{
    /**
     * Information for implementing lazy loading of games.
     * source: https://www.bassi.io/pages/lazy-loading/
     */

    /* states in the GUI-loading FSM */
    enum {
        STATE_STARTED,  /* start state */
        STATE_WAITING_FOR_OWNED_APPS, /* waiting for owned apps thread to finish */
        STATE_LOADING_GUI,  /* feeding game rows to the model */
        STATE_DOWNLOADING_ICONS,  /* fire off icon downloads (to be added to the model) */
        STATE_FINISHED  /* finish state */
    };

    /* data to be passed to the idle handler */
    typedef struct
    {
        /* the current state */
        guint state;

        /**
         * other information is just pulled from the global
         * or other singleton info for now
         */

        /* the currently loaded item */
        guint current_item;
    } IdleData;

    /**
     * When you click on the close button if steam is not running
     * Quit the GTK process, and destroy as much as possible: the program
     * is about to be exitted completely.
     */
    void 
    on_close_button_clicked();

    /** 
     * When the user wants to refresh the game list.
     * This is also called when the main window just got spawned.
     * - Clear the game list (will show the loading widget)
     * - Do the backend work (get owned apps..)
     * - Add all the retrieved data to the view.
     * - Do the backend work again for icons: view must be initialized first
     *   to display the app logos.
     * - Draw the result.
     */
    void 
    on_ask_game_refresh();

    /** 
     * When the main window... started showing / will be showing??
     * TODO: Make sure the loading widget is showing while starting to 
     * do the backend work
     */
    void 
    on_main_window_show();

    void
    on_about_button_clicked();

    void
    on_about_dialog_close_button_clicked ();

    void
    on_search_changed(GtkWidget* search_widget);

    void 
    on_game_row_activated(GtkListBox *box, GtkListBoxRow *row);

    void
    on_back_button_clicked();

    void
    on_store_button_clicked();
}