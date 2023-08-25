/**************************************************************************
 *  SQLiteDB plug-in for FAR 3.0 modifed by VPROFi 2023 for far2l         *
 *  Copyright (C) 2010-2014 by Artem Senichev <artemsen@gmail.com>        *
 *  https://sourceforge.net/projects/farplugs/                            *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include "progress.h"
#include "lng.h"
#include <cassert>

#define PROGRESS_WIDTH 30

progress::progress(const int msg_id, const uint64_t max_value /*= 0*/)
:	_visible(false),
	_max_value(max_value)
{
	_title = Plugin::psi.GetMsg(Plugin::psi.ModuleNumber, ps_title_short);
	_message = Plugin::psi.GetMsg(Plugin::psi.ModuleNumber, msg_id);
	show();
}


progress::~progress()
{
	hide();
}


void progress::show()
{
	if (!_visible) {
		_visible = true;
		Plugin::psi.AdvControl(Plugin::psi.ModuleNumber, ACTL_SETPROGRESSSTATE, (void*)PGS_INDETERMINATE);
	}

	if (_bar.empty()) {
		const wchar_t* msg[] = { _title, _message };
		Plugin::psi.Message(Plugin::psi.ModuleNumber, 0,  nullptr, msg, sizeof(msg) / sizeof(msg[0]), 0);
	}
	else {
		const wchar_t* msg[] = { _title, _message, _bar.c_str() };
		Plugin::psi.Message(Plugin::psi.ModuleNumber, 0,  nullptr, msg, sizeof(msg) / sizeof(msg[0]), 0);
	}
}


void progress::hide()
{
	if (_visible) {
		Plugin::psi.AdvControl(Plugin::psi.ModuleNumber, ACTL_PROGRESSNOTIFY, 0);
		Plugin::psi.AdvControl(Plugin::psi.ModuleNumber, ACTL_SETPROGRESSSTATE, (void*)PGS_NOPROGRESS);
		Plugin::psi.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
		Plugin::psi.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
		_visible = false;
	}
}


void progress::update(const uint64_t val)
{
	if (!_max_value)
		return;

	const size_t percent = static_cast<size_t>((val * 100) / _max_value);
	assert(percent <= 100);

	PROGRESSVALUE pv;
	memset(&pv, 0, sizeof(pv));
	pv.Completed = percent;
	pv.Total = 100;
	Plugin::psi.AdvControl(Plugin::psi.ModuleNumber, ACTL_SETPROGRESSVALUE, &pv);

	if (_bar.empty())
		_bar.resize(PROGRESS_WIDTH);
	const size_t fill_length = percent * _bar.size() / 100;
	std::fill(_bar.begin() + fill_length, _bar.end(), L'\x2591');
	std::fill(_bar.begin(), _bar.begin() + fill_length, L'\x2588');

	show();
}


bool progress::aborted()
{
	HANDLE std_in = stdin;
	INPUT_RECORD rec;
	DWORD read_count = 0;
	while( PeekConsoleInput(std_in, &rec, 1, &read_count) && read_count != 0 ) {
		ReadConsoleInput(std_in, &rec, 1, &read_count);
		if( rec.EventType == KEY_EVENT && rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && rec.Event.KeyEvent.bKeyDown )
			return true;
	}
	return false;
}
