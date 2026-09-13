/*
 * Copyright (c) 1998-2013 Danfeng Zhang (zhangdf@cs.cornell.edu)
 * DIFCVerilog modifications Copyright (c) 2026 Yubo Shi
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include <map>
#include "Module.h"
#include <stdexcept>

#ifdef _WIN32
__declspec(thread) extern std::map<perm_string, Module*>* g_current_modules;
#else
thread_local extern std::map<perm_string, Module*>* g_current_modules;
#endif

inline std::map<perm_string, Module*>& get_modules_ref() {
    if (g_current_modules == nullptr) {
        throw std::runtime_error("get_modules_ref(): modules not mounted! "
                                 "Call this function only during typecheck execution.");
    }
    return *g_current_modules;
}

#endif // GLOBAL_VARS_H
