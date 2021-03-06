/* $Id: hero_list.hpp 49605 2011-05-22 17:56:24Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Jörg Hinrichs <joerg.hinrichs@alice-dsl.de>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_ARTIFICAL_LIST_HPP_INCLUDED
#define GUI_DIALOGS_ARTIFICAL_LIST_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"

#include "config.hpp"

class game_display;
class team;
class unit_map;
class artifical;
class hero;
class hero_map;
class game_state;

namespace gui2 {

class tlistbox;

class tartifical_list : public tdialog
{
public:
	explicit tartifical_list(game_display& gui, std::vector<team>& teams, unit_map& units, hero_map& heros, game_state& gamestate, int side_num);

protected:
	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	/** Inherited from tdialog. */
	void post_show(twindow& window);

private:
	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	enum {MIN_PAGE = 0, OWNERSHIP_PAGE = MIN_PAGE, STATUS_PAGE, MAX_PAGE = STATUS_PAGE};
	void catalog_page(twindow& window, int catalog, bool swap);	

	void city_changed(twindow& window);
	void fill_table(int catalog);
	void fill_table_row(team& t, int catalog);

private:
	game_display& gui_;
	std::vector<team>& teams_;
	unit_map& units_;
	hero_map& heros_;
	game_state& gamestate_;

	int side_;
	std::vector<const artifical*> candidate_artificals_;

	tlistbox* hero_table_;
};

}

#endif

