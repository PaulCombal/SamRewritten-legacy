#include "StatBoxRow.h"
#include "../controller/MySteam.h"
#include "../globals.h"
#include "../common/functions.h"

#include <string>
#include <gtkmm-3.0/gtkmm/box.h>
#include <gtkmm-3.0/gtkmm/label.h>
#include <gtkmm-3.0/gtkmm/adjustment.h>
#include <gtkmm-3.0/gtkmm/popovermenu.h>
#include <gtkmm-3.0/gtkmm/separator.h>
#include <gtkmm-3.0/gtkmm/menubutton.h>


StatBoxRow::StatBoxRow(const StatValue_t& data) 
: m_data(data)
{
    Gtk::Box* layout = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_HORIZONTAL, 0);
    Gtk::Box* title_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_VERTICAL, 0);

    Gtk::MenuButton* more_info_button = Gtk::make_managed<Gtk::MenuButton>();
    Gtk::Image* more_info_image = Gtk::make_managed<Gtk::Image>("gtk-about", Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);
    Gtk::PopoverMenu* popover_menu = Gtk::make_managed<Gtk::PopoverMenu>();
    Gtk::Box* popover_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_VERTICAL, 3);
    Gtk::Label* more_info_label  = Gtk::make_managed<Gtk::Label>();
    Gtk::Separator* sep_one = Gtk::make_managed<Gtk::Separator>();

    Gtk::Box* values_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_VERTICAL, 0);
    Gtk::Box* new_values_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_HORIZONTAL, 0);
    Gtk::Label* title_label = Gtk::make_managed<Gtk::Label>("");
    Gtk::Label* type_label = Gtk::make_managed<Gtk::Label>("");
    Gtk::Label* cur_value_label = Gtk::make_managed<Gtk::Label>("");
    Gtk::Label* incr_only_label = Gtk::make_managed<Gtk::Label>("");
    Glib::RefPtr<Gtk::Adjustment> adjustment;

    set_size_request(-1, 40);
    m_invalid_conversion_box.set_valign(Gtk::Align::ALIGN_CENTER);
    m_invalid_conversion_box.set_margin_end(10);
    // Invalid_conversion_image is set up lazily in on_new_value_changed
    // to avoid ancestors calls to show_all from showing it and having to implement
    // logic to go hide it again.
    // Reserve space for image
    m_invalid_conversion_box.set_size_request(20, -1);

    more_info_label->set_markup("<b>Additional information</b>");
    more_info_button->set_popover(*popover_menu);
    more_info_button->set_valign(Gtk::Align::ALIGN_CENTER);
    more_info_button->set_margin_end(10);
    popover_box->set_border_width(5);
    more_info_button->get_style_context()->add_class("circular");

    values_box->set_size_request(150, -1);

    m_new_value_entry.set_width_chars(10);
    m_new_value_entry.set_max_width_chars(10);
    // Bound this to some reasonable value
    m_new_value_entry.set_max_length(100);

    // We don't really care to clamp the min/max of these entries
    if (data.type == UserStatType::Integer) {
        type_label->set_label("Type: Integer");
        cur_value_label->set_label("Current value: " + std::to_string(std::any_cast<long long>(data.value)));
        adjustment = Gtk::Adjustment::create(std::any_cast<long long>(data.value), -DBL_MAX, DBL_MAX, 1.0, 5.0, 0.0);
        m_new_value_entry.configure(adjustment, 1, 0);
    } else if (data.type == UserStatType::Float) {
        type_label->set_label("Type: Float");
        cur_value_label->set_label("Current value: " + std::to_string(std::any_cast<double>(data.value)));
        adjustment = Gtk::Adjustment::create(std::any_cast<double>(data.value), -DBL_MAX, DBL_MAX, 0.01, 5.0, 0.0);
        m_new_value_entry.configure(adjustment, 0.01, 5);
    } else {
        type_label->set_label("Type: Unknown");
        cur_value_label->set_label("Current value: Unknown");
        adjustment = Gtk::Adjustment::create(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        m_new_value_entry.configure(adjustment, 0.0, 0);
    }

    if (data.incrementonly) {
        incr_only_label->set_label("Increment Only: Yes");
    } else {
        incr_only_label->set_label("Increment Only: No");
    }

    title_label->set_label(data.display_name == "" ? data.id : data.display_name);

    title_box->pack_start(*title_label, false, true, 0);
    new_values_box->pack_start(m_new_value_entry, true, true, 0);
    values_box->pack_start(*new_values_box, false, true, 0);
    layout->pack_start(*title_box, true, true, 0);
    layout->pack_start(m_invalid_conversion_box, false, true, 0);
    layout->pack_start(*more_info_button, false, true, 0);
    layout->pack_start(*values_box, false, true, 0);

    more_info_button->add(*more_info_image);
    popover_box->pack_start(*more_info_label, false, true, 0);
    popover_box->pack_start(*sep_one, false, true, 0);
    popover_box->pack_start(*type_label, false, true, 0);
    popover_box->pack_start(*cur_value_label, false, true, 0);
    popover_box->pack_start(*incr_only_label, false, true, 0);
    popover_menu->add(*popover_box);
    popover_box->show_all();

    layout->set_valign(Gtk::Align::ALIGN_CENTER);
    add(*layout);

    m_new_value_entry.signal_changed().connect(sigc::mem_fun(this, &StatBoxRow::on_new_value_changed));
}

StatBoxRow::~StatBoxRow() {}

void
StatBoxRow::on_new_value_changed(void) {
    // Note this is not run after a short delay of the text not being changed
    // so maybe we don't want to edit the list every single time a button is pressed.
    // The alternative is to read through all stat boxes when commit changes is
    // pressed.
    bool valid_conversion;
    std::any new_value;
    const std::string& buf = m_new_value_entry.get_text();

    valid_conversion = convert_user_stat_value(m_data.type, buf, &new_value);

    // Remove pending modifications with different values, if any
    g_steam->remove_modification_stat(m_data);

    // Note that currently, if a stat's new_value entry is changed at all,
    // the new value will always be sent to steam even if it's returned to its
    // original value. This isn't that big of a deal.
    if (valid_conversion) {
        g_steam->add_modification_stat(m_data, new_value);
        if (m_b_has_invalid_conversion_image_been_set_up) {
            m_invalid_conversion_image.hide();
        }
    } else {
        if (!m_b_has_invalid_conversion_image_been_set_up) {
            m_invalid_conversion_image = Gtk::Image("gtk-dialog-warning", Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);
            m_invalid_conversion_image.set_tooltip_text("The entered value is invalid and will not be sent to Steam");
            m_invalid_conversion_box.pack_start(m_invalid_conversion_image, false, true, 0);
            m_b_has_invalid_conversion_image_been_set_up = true;
        }
        m_invalid_conversion_image.show();
    }
}
