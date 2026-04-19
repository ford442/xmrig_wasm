/* XMRig
 * Copyright (c) 2016-2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* Emscripten/WASM stub for App_unix.cpp.
 *
 * Background (daemon) mode via fork()/setsid() is not available in WASM.
 * This stub satisfies the linker without introducing POSIX process calls.
 */

#include "App.h"


bool xmrig::App::background(int &)
{
    // Daemon mode is unsupported in WASM; caller treats false as "keep running".
    return false;
}
