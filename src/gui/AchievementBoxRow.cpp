#include "AchievementBoxRow.h"
#include "../controller/MySteam.h"
#include "../common/functions.h"
#include "../globals.h"

#include <gtkmm-3.0/gtkmm/box.h>
#include <gtkmm-3.0/gtkmm/popovermenu.h>
#include <gtkmm-3.0/gtkmm/separator.h>
#include <gtkmm-3.0/gtkmm/progressbar.h>
#include <gtkmm-3.0/gtkmm/label.h>
#include <gtkmm-3.0/gtkmm/menubutton.h>

#define ACH_BOX_ROW_ICON_LOCKED "changes-prevent"
#define ACH_BOX_ROW_ICON_UNLOCKED "changes-allow"
#define ACH_BOX_ROW_ICON_TO_LOCK "emblem-important"
#define ACH_BOX_ROW_ICON_TO_UNLOCK "emblem-default"

AchievementBoxRow::AchievementBoxRow(const Achievement_t& data) 
: m_active(false),
  m_data(data)
{
    std::string ach_title_text, ach_player_percent_text, ach_locked_text, button_icon_name, escaped_name;
    ach_locked_text = data.achieved ? "  Unlocked" : "  Locked";
    button_icon_name = data.achieved ? ACH_BOX_ROW_ICON_UNLOCKED : ACH_BOX_ROW_ICON_LOCKED;
    escaped_name = data.name;
    escape_html(escaped_name);
    ach_title_text = "<b>" + escaped_name + "</b>";
    char tmpbuf[16];
    snprintf(tmpbuf, 16, "%.1f", data.global_achieved_rate);
    ach_player_percent_text = "Achieved by " + std::string(tmpbuf) + "% of the players";

    //TODO show new level bar only if ach has progress bar
    Gtk::Box* layout = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_HORIZONTAL, 0);
    Gtk::Box* title_desc_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_VERTICAL, 0);
    Gtk::Label* title_label = Gtk::make_managed<Gtk::Label>("");
    Gtk::Label* desc_label = Gtk::make_managed<Gtk::Label>(data.desc);
    Gtk::MenuButton* more_info_button = Gtk::make_managed<Gtk::MenuButton>();
    Gtk::Image* more_info_image = Gtk::make_managed<Gtk::Image>("gtk-about", Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);
    Gtk::PopoverMenu* popover_menu = Gtk::make_managed<Gtk::PopoverMenu>();
    Gtk::Box* popover_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_VERTICAL, 3);
    Gtk::Label* more_info_label  = Gtk::make_managed<Gtk::Label>();
    Gtk::Label* percentage_players_label = Gtk::make_managed<Gtk::Label>(ach_player_percent_text);
    Gtk::Separator* sep_one = Gtk::make_managed<Gtk::Separator>();
    Gtk::Separator* sep_two = Gtk::make_managed<Gtk::Separator>();
    Gtk::ProgressBar* ach_progress_bar = Gtk::make_managed<Gtk::ProgressBar>(); // TODO
    Gtk::ProgressBar* ach_ach_rate_bar = Gtk::make_managed<Gtk::ProgressBar>();
    Gtk::Label* progress_label = Gtk::make_managed<Gtk::Label>("Completed X of Y");

    set_size_request(-1, 80);
    set_missing();

    // https://developer.gnome.org/Labels/#Labels_in_resizable_windows
    desc_label->set_line_wrap();
    desc_label->set_width_chars(1); // Not sure what n_chars is
    title_label->set_markup(ach_title_text);
    more_info_label->set_markup("<b>Additional information</b>");
    more_info_button->set_popover(*popover_menu);
    more_info_button->set_valign(Gtk::Align::ALIGN_CENTER);
    more_info_button->set_margin_end(10);
    m_lock_unlock_button.set_size_request(150, -1);
    m_lock_unlock_button.set_active(false);
    m_lock_unlock_button.set_label(ach_locked_text);
    m_lock_unlock_button.set_image_from_icon_name(button_icon_name);
    popover_box->set_border_width(5);
    ach_ach_rate_bar->set_fraction(data.global_achieved_rate / 100);
    more_info_button->get_style_context()->add_class("circular");

    more_info_button->add(*more_info_image);
    title_desc_box->pack_start(*title_label, false, true, 0);
    title_desc_box->pack_start(*desc_label, true, true, 0);
    layout->pack_start(m_icon, false, true, 0);
    layout->pack_start(*title_desc_box, true, true, 0);

    if (is_permission_protected(data.permission))
    {
        Gtk::Label* label = Gtk::make_managed<Gtk::Label>("⚠️   ");
        label->set_tooltip_text("This achievement is protected. SamRewritten will likely not be able to change it!");
        layout->pack_start(*label, false, true, 0);
    }

    if (data.special & ACHIEVEMENT_RARE)
    {
        Gtk::Label* label = Gtk::make_managed<Gtk::Label>("💎   ");
        label->set_tooltip_text("This is a difficult achievement, less than 5% people earned it!");
        layout->pack_start(*label, false, true, 0);
    }

    if (data.special & ACHIEVEMENT_NEXT_MOST_ACHIEVED)
    {
        Gtk::Label* label = Gtk::make_managed<Gtk::Label>("📈   ");
        label->set_tooltip_text("This is the next most unlocked achievement");
        layout->pack_start(*label, false, true, 0);
    }

    layout->pack_start(*more_info_button, false, true, 0);
    layout->pack_start(m_lock_unlock_button, false, true, 0);
    popover_box->pack_start(*more_info_label, false, true, 0);
    popover_box->pack_start(*sep_one, false, true, 0);
    popover_box->pack_start(*percentage_players_label, false, true, 0);
    popover_box->pack_start(*ach_ach_rate_bar, false, true, 0);
    popover_box->pack_start(*sep_two, false, true, 0);
    popover_box->pack_start(*progress_label, false, true, 0);
    popover_box->pack_start(*ach_progress_bar, false, true, 0);

    popover_menu->add(*popover_box);
    popover_box->show_all();
    add(*layout);

    m_lock_unlock_button.signal_toggled().connect(sigc::mem_fun(this, &AchievementBoxRow::invert));
}

AchievementBoxRow::~AchievementBoxRow() {}

void
AchievementBoxRow::invert_scripted() {
    const bool pressed = m_lock_unlock_button.get_active();
    m_lock_unlock_button.set_active(!pressed);
}

void
AchievementBoxRow::unlock_scripted() {
    if ( !m_data.achieved ) {
        m_lock_unlock_button.set_active(true);
    }
}

void
AchievementBoxRow::lock_scripted() {
    if ( m_data.achieved ) {
        m_lock_unlock_button.set_active(true);
    }
}

void
AchievementBoxRow::invert() {
    const bool achieved = m_data.achieved;
    const std::string ach_id = m_data.id;

    if (!m_active && achieved) {
        m_lock_unlock_button.set_label("  To relock");
        m_lock_unlock_button.set_image_from_icon_name(ACH_BOX_ROW_ICON_TO_LOCK);
        m_active = true;
        g_steam->add_modification_ach(ach_id, false);
    } else if (m_active && achieved) {
        m_lock_unlock_button.set_label("  Unlocked");
        m_lock_unlock_button.set_image_from_icon_name(ACH_BOX_ROW_ICON_UNLOCKED);

        m_active = false;
        g_steam->remove_modification_ach(ach_id);
    } else if (!m_active && !achieved) {
        m_lock_unlock_button.set_label("  To unlock");
        m_lock_unlock_button.set_image_from_icon_name(ACH_BOX_ROW_ICON_TO_UNLOCK);

        m_active = true;
        g_steam->add_modification_ach(ach_id, true);
    } else if (m_active && !achieved) {
        m_lock_unlock_button.set_label("  Locked");
        m_lock_unlock_button.set_image_from_icon_name(ACH_BOX_ROW_ICON_LOCKED);
        m_active = false;
        g_steam->remove_modification_ach(ach_id);
    }
}
// => invert