/* $Id: mp_create_game.cpp 49597 2011-05-22 17:55:49Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/dialogs/mp_create_game.hpp"

#include "game_display.hpp"
#include "foreach.hpp"
#include "game_preferences.hpp"
#include "gettext.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/dialogs/field.hpp"
#include "gui/dialogs/helper.hpp"
#include "gui/widgets/integer_selector.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/slider.hpp"
#include "../../settings.hpp"
#include "filesystem.hpp"
#include "map.hpp"
#include "map_exception.hpp"
#include "map_create.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/mp_create_game_set_password.hpp"
#include "gui/dialogs/transient_message.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "formula_string_utils.hpp"
#include <boost/bind.hpp>

namespace gui2 {

REGISTER_DIALOG(mp_create_game)

tmp_create_game::tmp_create_game(game_display& gui, const config& cfg) :
	trandom_map(cfg)
	, gui_(gui)
	, num_turns_(0)
	, era_index_(0)
	, era_(NULL)
	, launch_game_(NULL)
	, maximal_defeated_activity_(NULL)
	, use_map_settings_(register_bool("use_map_settings",
		true,
		preferences::use_map_settings,
		preferences::set_use_map_settings,
		dialog_callback<tmp_create_game, &tmp_create_game::update_map_settings>))
	, fog_(register_bool("fog",
			true,
			preferences::fog,
			preferences::set_fog))
	, observers_(register_bool("observers",
			true,
			preferences::allow_observers,
			preferences::set_allow_observers))
	, shroud_(register_bool("shroud",
			true,
			preferences::shroud,
			preferences::set_shroud))
	, start_time_(register_bool("random_start_time",
			true,
			preferences::random_start_time,
			preferences::set_random_start_time))

	, turns_(register_integer("turn_count",
		true,
		preferences::turns ,
		preferences::set_turns))
	, gold_(register_integer("village_gold",
		true,
		preferences::village_gold ,
		preferences::set_village_gold))
	, experience_(register_integer("experience_modifier",
		true,
		preferences::xp_modifier ,
		preferences::set_xp_modifier))
{
}

void tmp_create_game::pre_show(CVideo& /*video*/, twindow& window)
{
	name_entry_ = find_widget<ttext_box>(&window, "game_name", false, true);

	era_ = find_widget<tbutton>(&window, "era", false, true);
	launch_game_ = find_widget<tbutton>(&window, "ok", false, true);

	find_widget<tslider>(&window, "experience_modifier", false, true)->set_visible(twidget::INVISIBLE);
	find_widget<ttoggle_button>(&window, "observers", false, true)->set_visible(twidget::INVISIBLE);
	find_widget<ttoggle_button>(&window, "time_limit", false, true)->set_visible(twidget::INVISIBLE);
	find_widget<ttoggle_button>(&window, "random_start_time", false, true)->set_visible(twidget::INVISIBLE);
			
	maximal_defeated_activity_ = find_widget<tbutton>(&window, "maximal_defeated_activity", false, true);
	
	connect_signal_mouse_left_click(
		*era_
		, boost::bind(
			&tmp_create_game::era
			, this
			, boost::ref(window)));
	connect_signal_mouse_left_click(
		find_widget<tbutton>(&window, "password", false)
		, boost::bind(
			&tmp_create_game::password
			, this
			, boost::ref(window)));

	connect_signal_mouse_left_click(
		*maximal_defeated_activity_
		, boost::bind(
			&tmp_create_game::maximal_defeated_activity
			, this
			, boost::ref(window)));

	if (size_t(preferences::era()) < cfg_.child_count("era")) {
		era_index_ = preferences::era();
	}
	const config& era_cfg = cfg_.child("era", era_index_);
	era_->set_label(era_cfg["name"]);

	utils::string_map i18n_symbols;
	i18n_symbols["login"] = preferences::login();
	name_entry_->set_value(vgettext("$login|'s game", i18n_symbols));

	std::stringstream str;
	str << game_config::maximal_defeated_activity;
	maximal_defeated_activity_->set_label(str.str());

	observers_->set_widget_value(window, preferences::allow_observers());

	trandom_map::pre_show(window);

	// Force first update to be directly.
	lobby_base::network_handler();
	lobby_update_timer_ = add_timer(game_config::lobby_network_timer
			, boost::bind(&lobby_base::network_handler, this)
			, true);
}

void tmp_create_game::post_update_map(twindow& window, int select)
{
	// Support "load game" in future.
	launch_game_->set_active(select? true: false);
}

void tmp_create_game::update_map_settings(twindow& window)
{
	const bool use_map_settings = use_map_settings_->get_widget_value(window);

	fog_->widget_set_enabled(window, !use_map_settings, false);
	shroud_->widget_set_enabled(window, !use_map_settings, false);
	start_time_->widget_set_enabled(window, !use_map_settings, false);

	turns_->widget_set_enabled(window, !use_map_settings, false);
	gold_->widget_set_enabled(window, !use_map_settings, false);
	experience_->widget_set_enabled(window, !use_map_settings, false);

	if (use_map_settings) {
		config::const_child_itors sides = parameters_.scenario_data.child_range("side");
		if (sides.first != sides.second) {
			const config &side = parameters_.scenario_data.child("side");

			fog_->set_widget_value(window, side["fog"].to_bool(true));
			shroud_->set_widget_value(window, side["shroud"].to_bool(false));
			gold_->set_widget_value(window, ::settings::get_village_gold(side["village_gold"]));
		}
		start_time_->set_widget_value(window, parameters_.scenario_data["random_start_time"].to_bool(true));

		turns_->set_widget_value(window, ::settings::get_turns(parameters_.scenario_data["turns"]));
		experience_->set_widget_value(window, ::settings::get_xp_modifier(parameters_.scenario_data["experience_modifier"]));
		// No scenario selected just leave the state unchanged for now.

	} else {

		// Fixme we should store the value and reuse it later...
		fog_->set_widget_value(window, preferences::fog());
		shroud_->set_widget_value(window, preferences::shroud());
		start_time_->set_widget_value(window, preferences::random_start_time());

		turns_->set_widget_value(window, preferences::turns());
		gold_->set_widget_value(window, preferences::village_gold());
		experience_->set_widget_value(window, preferences::xp_modifier());
	}
}

void tmp_create_game::era(twindow& window)
{
	// The possible eras to play
	std::vector<std::string> eras;
	foreach (const config &er, cfg_.child_range("era")) {
		eras.push_back(er["name"]);
	}
	if (eras.empty()) {
		gui2::show_transient_message(gui_.video(), "", _("No eras found."));
		throw config::error(_("No eras found"));
	}

	gui2::tcombo_box dlg(eras, era_index_);
	dlg.show(gui_.video());

	era_index_ = dlg.selected_index();
	era_->set_label(eras[era_index_]);
	preferences::set_era(era_index_);
}

void tmp_create_game::password(twindow& window)
{
	gui2::tmp_create_game_set_password pwd_dlg(parameters_.password);
	pwd_dlg.show(gui_.video());
	// parameters_.password = pwd_dlg.password();
}

void tmp_create_game::maximal_defeated_activity(twindow& window)
{
	// The possible eras to play
	std::vector<std::string> activities;
	activities.push_back("0");
	activities.push_back("50");
	activities.push_back("100");
	activities.push_back("150");
	
	int activity_index;
	if (game_config::maximal_defeated_activity <= 0) {
		activity_index = 0;
	} else if (game_config::maximal_defeated_activity <= 50) {
		activity_index = 1;
	} else if (game_config::maximal_defeated_activity <= 100) {
		activity_index = 2;
	} else {
		activity_index = 3;
	}

	gui2::tcombo_box dlg(activities, activity_index);
	dlg.show(gui_.video());

	activity_index = dlg.selected_index();
	if (activity_index == 0) {
		game_config::maximal_defeated_activity = 0;
	} else if (activity_index == 1) {
		game_config::maximal_defeated_activity = 50;
	} else if (activity_index == 2) {
		game_config::maximal_defeated_activity = 100;
	} else {
		game_config::maximal_defeated_activity = 150;
	}

	std::stringstream str;
	str << game_config::maximal_defeated_activity;
	maximal_defeated_activity_->set_label(str.str());
}

void tmp_create_game::post_show(twindow& window)
{
	num_turns_ = turns_->get_widget_value(window) < ::settings::turns_max? 
		turns_->get_widget_value(window): -1;

	const int mp_countdown_turn_bonus_val = preferences::countdown_turn_bonus();
	const int mp_countdown_action_bonus_val = preferences::countdown_action_bonus();
	const int mp_countdown_reservoir_time_val = preferences::countdown_reservoir_time();
	int mp_countdown_init_time_val = preferences::countdown_init_time();
	if (mp_countdown_reservoir_time_val > 0 && mp_countdown_init_time_val > mp_countdown_reservoir_time_val)
		mp_countdown_init_time_val = mp_countdown_reservoir_time_val;

	// Updates the values in the "parameters_" member to match
	// the values selected by the user with the widgets:
	parameters_.name = name_entry_->get_value();

	config::const_child_itors era_list = cfg_.child_range("era");
	std::advance(era_list.first, era_index_);
	parameters_.mp_era = (*era_list.first)["id"].str();

	// CHECK
	parameters_.mp_countdown_init_time = mp_countdown_init_time_val;
	parameters_.mp_countdown_turn_bonus = mp_countdown_turn_bonus_val;
	parameters_.mp_countdown_reservoir_time = mp_countdown_reservoir_time_val;
	parameters_.mp_countdown_action_bonus = mp_countdown_action_bonus_val;
	parameters_.mp_countdown = false;
	parameters_.village_gold = gold_->get_widget_value(window);
	parameters_.xp_modifier = experience_->get_widget_value(window);
	parameters_.use_map_settings = use_map_settings_->get_widget_value(window);
	parameters_.random_start_time = start_time_->get_widget_value(window);
	parameters_.fog_game = fog_->get_widget_value(window);
	parameters_.shroud_game = shroud_->get_widget_value(window);
	parameters_.allow_observers = observers_->get_widget_value(window);
	// parameters_.share_view = vision_combo_.selected() == 0;
	parameters_.share_view = true;
	// parameters_.share_maps = vision_combo_.selected() == 1;
	parameters_.share_maps = false;

	// treat password as checksum
	parameters_.password = game_config::checksum;

	remove_timer(lobby_update_timer_);
	lobby_update_timer_ = 0;
}

void tmp_create_game::process_network_data(const config& data, const network::connection sock)
{
}

} // namespace gui2

