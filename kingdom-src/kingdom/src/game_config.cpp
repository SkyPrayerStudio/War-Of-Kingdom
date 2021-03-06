/* $Id: game_config.cpp 46969 2010-10-08 19:45:32Z mordante $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"
#include "game_config.hpp"

#include "color_range.hpp"
#include "config.hpp"
#include "foreach.hpp"
#include "gettext.hpp"
#include "log.hpp"
#include "util.hpp"
#include "version.hpp"
#include "wesconfig.h"
#include "serialization/string_utils.hpp"
#ifdef HAVE_REVISION
#include "revision.hpp"
#endif /* HAVE_REVISION */

static lg::log_domain log_engine("engine");
#define DBG_NG LOG_STREAM(debug, log_engine)
#define ERR_NG LOG_STREAM(err, log_engine)

namespace game_config
{
	int base_income = 2;
	int village_income = 2;
	int hero_income = 2;
	int poison_amount= 8;
	int rest_heal_amount= 2;
	int recall_cost = 20;
	int kill_experience = 8;
	unsigned lobby_network_timer = 100;
	unsigned lobby_refresh = 4000;
	const int gold_carryover_percentage = 80;
	const std::string version = VERSION;
	int reside_troop_increase_loyalty = 30;
	int field_troop_increase_loyalty = 8;
	int wander_loyalty_threshold = 112; // HERO_MAX_LOYALTY / 4 * 3
	int move_loyalty_threshold = 130; // HERO_MAX_LOYALTY - 20
	int ai_keep_loyalty_threshold = wander_loyalty_threshold + 13;
	int ai_keep_hp_threshold = 50;
	int max_keep_turns = 10;
	int increase_feeling = 1024; // HERO_MAX_FEELING / 64. 64 turns will carry.
	int minimal_activity = 175;
	int maximal_defeated_activity = 0;
	int max_police = 100;
	int min_tradable_police = max_police / 2;
	int max_commoners = 5;
	int active_tactic_slots = 2;

	int default_human_level = 1;
	int default_ai_level = 2;
	int current_level = 1;
	int min_level = 1;
	int max_level = 3;

	int max_tactic_point = 15;
	int increase_tactic_point = 4;

	int max_bomb_turns = 2;

	int kill_xp(int level)
	{
		level += 3;
		return level ? kill_experience * level : kill_experience / 2;
	}
	int attack_xp(int level)
	{
		level += 3;
		return level;
	}

	const std::string revision = VERSION;
	const std::string checksum = "0123456789abcdef";
	std::string wesnoth_program_dir;
	bool debug = false, editor = false, ignore_replay_errors = false, mp_debug = false, exit_at_end = false, no_delay = false, disable_autosave = false;

	bool use_bin = true;
	int cache_compression_level = 6;
	bool tiny_gui = false;
	int start_cards = 6;
	int cards_per_turn = 2;
	int max_cards = 20;

	int title_logo_x = 0, title_logo_y = 0, title_buttons_x = 0, title_buttons_y = 0, title_buttons_padding = 0,
	    title_tip_x = 0, title_tip_width = 0, title_tip_padding = 0;

	std::string title_music,
			lobby_music,
			default_victory_music,
			default_defeat_music;

	namespace images {
	std::string game_title,
			// orbs and hp/xp bar
			moved_orb,
			unmoved_orb,
			partmoved_orb,
			automatic_orb,
			enemy_orb,
			ally_orb,
			bar_hrl_72,
			bar_hrl_64,
			bar_hrl_56,
			bar_hrl_48,
			// flags
			flag,
			flag_icon,
			big_flag,
			// hex overlay
			terrain_mask,
			grid_top,
			grid_bottom,
			mouseover,
			selected,
			editor_brush,
			unreachable,
			linger,
			// GUI elements
			observer,
			tod_bright,
			tod_dark,
			///@todo de-hardcode this
			checked_menu = "buttons/checkbox-pressed.png",
			unchecked_menu = "buttons/checkbox.png",
			wml_menu = "buttons/WML-custom.png",
			level,
			ellipsis,
			missing;
	} //images

	std::string shroud_prefix, fog_prefix;

	std::string flag_rgb;
	std::vector<Uint32> red_green_scale;
	std::vector<Uint32> red_green_scale_text;

	double hp_bar_scaling = 0.666;
	double xp_bar_scaling = 0.5;

	double hex_brightening = 1.5;
	double hex_semi_brightening = 1.25;

	std::vector<std::string> foot_speed_prefix;
	std::string foot_teleport_enter;
	std::string foot_teleport_exit;

	std::map<std::string, color_range > team_rgb_range;
	std::vector<std::pair<std::string, t_string> > team_rgb_name;

	std::map<std::string, std::vector<Uint32> > team_rgb_colors;

	const version_info wesnoth_version(VERSION);

	const std::string observer_team_name = "observer";

	const size_t max_loop = 65536;

	// ai part
	int navigation_per_level = 10000;

	namespace sounds {
		const std::string turn_bell = "bell.wav",
		timer_bell = "timer.wav",
		receive_message = "chat-1.ogg,chat-2.ogg,chat-3.ogg",
		receive_message_highlight = "chat-highlight.ogg",
		receive_message_friend = "chat-friend.ogg",
		receive_message_server = "receive.wav",
		user_arrive = "arrive.wav",
		user_leave = "leave.wav",
		game_user_arrive = "join.wav",
		game_user_leave = "leave.wav";

		const std::string button_press = "button.wav",
		checkbox_release = "checkbox.wav",
		slider_adjust = "slider.wav",
		menu_expand = "expand.wav",
		menu_contract = "contract.wav",
		menu_select = "select.wav";
	}



#ifdef __AMIGAOS4__
	std::string path = "PROGDIR:";
#else
#ifdef __APPLE__
	std::string path = "./";
#else
	std::string path = "";
#endif
#endif

	std::string preferences_dir = "";
	std::string preferences_dir_utf8 = "";

	std::vector<server_info> server_list;

	void load_config(const config* cfg)
	{
		if(cfg == NULL)
			return;

		const config& v = *cfg;

		base_income = v["base_income"].to_int(2);
		village_income = v["village_income"].to_int(2);
		hero_income = v["hero_income"].to_int(2);
		poison_amount = v["poison_amount"].to_int(8);
		rest_heal_amount = v["rest_heal_amount"].to_int(2);
		recall_cost = v["recall_cost"].to_int(20);
		kill_experience = v["kill_experience"].to_int(8);
		lobby_refresh = v["lobby_refresh"].to_int(2000);

		reside_troop_increase_loyalty = v["reside_troop_increase_loyalty"].to_int(15);
		field_troop_increase_loyalty = v["field_troop_increase_loyalty"].to_int(4);
		wander_loyalty_threshold = v["wander_loyalty_threshold"].to_int(112); // HERO_MAX_LOYALTY / 4 * 3
		move_loyalty_threshold = v["wander_loyalty_threshold"].to_int(130); // HERO_MAX_LOYALTY - 20
		ai_keep_loyalty_threshold = wander_loyalty_threshold + 13;
		ai_keep_hp_threshold = v["ai_keep_hp_threshold"].to_int(50);
		max_keep_turns = v["max_keep_turns"].to_int(10);
		max_police = v["max_police"].to_int(100);
		min_tradable_police = v["min_tradable_police"].to_int(max_police / 2);
		max_commoners = v["max_commoners"].to_int(5);

		minimal_activity = v["minimal_activity"].to_int(175);
		if (minimal_activity > 0xff) {
			minimal_activity = 175;
		} else if (minimal_activity < 2) {
			minimal_activity = 2;
		}

		max_cards = v["max_cards"].to_int(20);
		default_ai_level = v["default_ai_level"].to_int(2);

		title_music = v["title_music"].str();
		lobby_music = v["lobby_music"].str();
		default_victory_music = v["default_victory_music"].str();
		default_defeat_music = v["default_defeat_music"].str();

		if(const config &i = v.child("images")){
			using namespace game_config::images;
			game_title = i["game_title"].str();

			moved_orb = i["moved_orb"].str();
			unmoved_orb = i["unmoved_orb"].str();
			partmoved_orb = i["partmoved_orb"].str();
			automatic_orb = i["automatic_orb"].str();
			enemy_orb = i["enemy_orb"].str();
			ally_orb = i["ally_orb"].str();
			bar_hrl_72 = i["bar_hrl_72"].str();
			bar_hrl_64 = i["bar_hrl_64"].str();
			bar_hrl_56 = i["bar_hrl_56"].str();
			bar_hrl_48 = i["bar_hrl_48"].str();

			flag = i["flag"].str();
			flag_icon = i["flag_icon"].str();
			big_flag = i["big_flag"].str();

			terrain_mask = i["terrain_mask"].str();
			grid_top = i["grid_top"].str();
			grid_bottom = i["grid_bottom"].str();
			mouseover = i["mouseover"].str();
			selected = i["selected"].str();
			editor_brush = i["editor_brush"].str();
			unreachable = i["unreachable"].str();
			linger = i["linger"].str();

			observer = i["observer"].str();
			tod_bright = i["tod_bright"].str();
			tod_dark = i["tod_dark"].str();
			level = i["level"].str();
			ellipsis = i["ellipsis"].str();
			missing = i["missing"].str();
		} // images

		hp_bar_scaling = v["hp_bar_scaling"].to_double(0.666);
		xp_bar_scaling = v["xp_bar_scaling"].to_double(0.5);
		hex_brightening = v["hex_brightening"].to_double(1.5);
		hex_semi_brightening = v["hex_semi_brightening"].to_double(1.25);

		foot_speed_prefix = utils::split(v["footprint_prefix"]);
		foot_teleport_enter = v["footprint_teleport_enter"].str();
		foot_teleport_exit = v["footprint_teleport_exit"].str();

		title_logo_x = v["logo_x"];
		title_logo_y = v["logo_y"];
		title_buttons_x = v["buttons_x"];
		title_buttons_y = v["buttons_y"];
		title_buttons_padding = v["buttons_padding"];

		title_tip_x = v["tip_x"].to_int();
		title_tip_width = v["tip_width"].to_int();
		title_tip_padding = v["tip_padding"].to_int();

		shroud_prefix = v["shroud_prefix"].str();
		fog_prefix  = v["fog_prefix"].str();

		add_color_info(v);

		if (const config::attribute_value *a = v.get("flag_rgb"))
			flag_rgb = a->str();

		// red_green_scale = string2rgb(v["red_green_scale"]);
		if (red_green_scale.empty()) {
			red_green_scale.push_back(0x00FFFF00);
		}
		// red_green_scale_text = string2rgb(v["red_green_scale_text"]);
		if (red_green_scale_text.empty()) {
			red_green_scale_text.push_back(0x00FFFF00);
		}

		server_list.clear();
		foreach (const config &server, v.child_range("server"))
		{
			server_info sinf;
			sinf.name = server["name"].str();
			sinf.address = server["address"].str();
			server_list.push_back(sinf);
		}
	}

	void add_color_info(const config &v)
	{
		foreach (const config &teamC, v.child_range("color_range"))
		{
			const config::attribute_value *a1 = teamC.get("id"),
				*a2 = teamC.get("rgb");
			if (!a1 || !a2) continue;
			std::string id = *a1;
			std::vector<Uint32> temp = string2rgb(*a2);
			team_rgb_range.insert(std::make_pair(id,color_range(temp)));
			team_rgb_name.push_back(std::make_pair<std::string, t_string>(id, teamC["name"]));
			//generate palette of same name;
			std::vector<Uint32> tp = palette(team_rgb_range[id]);
			if (tp.empty()) continue;
			team_rgb_colors.insert(std::make_pair(id,tp));
			//if this is being used, output log of palette for artists use.
			DBG_NG << "color palette creation:\n";
			std::ostringstream str;
			str << id << " = ";
			for (std::vector<Uint32>::const_iterator r = tp.begin(),
			     r_end = tp.end(), r_beg = r; r != r_end; ++r)
			{
				int red = ((*r) & 0x00FF0000) >> 16;
				int green = ((*r) & 0x0000FF00) >> 8;
				int blue = ((*r) & 0x000000FF);
				if (r != r_beg) str << ',';
				str << red << ',' << green << ',' << blue;
			}
			DBG_NG << str.str() << '\n';
		}

		std::map<std::string, color_range >& team_rgb_range_p = team_rgb_range;
		std::vector<std::pair<std::string, t_string > >& team_rgb_name_p = team_rgb_name;
		std::map<std::string, std::vector<Uint32> >& team_rgb_colors_p = team_rgb_colors;

		foreach (const config &cp, v.child_range("color_palette"))
		{
			foreach (const config::attribute &rgb, cp.attribute_range())
			{
				try {
					team_rgb_colors.insert(std::make_pair(rgb.first, string2rgb(rgb.second)));
				} catch(bad_lexical_cast&) {
					//throw config::error(_("Invalid team color: ") + rgb_it->second);
					ERR_NG << "Invalid team color: " << rgb.second << "\n";
				}
			}
		}
	}

	const color_range& color_info(const std::string& name)
	{
		std::map<std::string, color_range>::const_iterator i = team_rgb_range.find(name);
		if(i == team_rgb_range.end()) {
			try {
				team_rgb_range.insert(std::make_pair(name,color_range(string2rgb(name))));
				return color_info(name);
			} catch(bad_lexical_cast&) {
				//ERR_NG << "Invalid color range: " << name;
				//return color_info();
				throw config::error(_("Invalid color range: ") + name);
			}
		}
		return i->second;
	}

	const std::vector<Uint32>& tc_info(const std::string& name)
	{
		std::map<std::string, std::vector<Uint32> >::const_iterator i = team_rgb_colors.find(name);
		if(i == team_rgb_colors.end()) {
			try {
				team_rgb_colors.insert(std::make_pair(name,string2rgb(name)));
				return tc_info(name);
			} catch(bad_lexical_cast&) {
				static std::vector<Uint32> stv;
				//throw config::error(_("Invalid team color: ") + name);
				ERR_NG << "Invalid team color: " << name << "\n";
				return stv;
			}
		}
		return i->second;
	}

	Uint32 red_to_green(int val, bool for_text){
		const std::vector<Uint32>& color_scale =
				for_text ? red_green_scale_text : red_green_scale;
		val = std::max<int>(0, std::min<int>(val, 100));
		int lvl = (color_scale.size()-1) * val / 100;
		return color_scale[lvl];
	}

} // game_config
