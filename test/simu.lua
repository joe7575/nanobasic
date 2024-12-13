--[[
  VM2 a 16-bit virtual machine for use in the game Minetest.
  Copyright (C) 2023-2024 Joachim Stolberg <iauit@gmx.de>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
]]--

-- Parameters types
NB_NONE     = 0
NB_NUM      = 1
NB_STR      = 2
NB_ARR      = 3

-- Return values of 'nb_run()'
NB_END      = 0  -- programm end reached
NB_ERROR    = 1  -- error in programm
NB_BUSY     = 2  -- programm still running
NB_BREAK    = 3  -- break command
NB_XFUNC    = 4  -- 'call' external function

local nblib = require("nanobasiclib")

local function simu()
    print("Version:", nblib.version())
    print("Sleep:  ", nblib.add_function("sleep", {NB_NUM}, NB_NONE))
    local vm, sts = nblib.create('print 1+2\nexit\n')
    print("Create: ", sts, vm)
    if sts ~= 0 then
        print("Error:", sts)
        return
    end
    print(nblib.run(vm, 1000))
    --local s = nblib.pack_vm(vm)
    --print(#s)
    --nblib.unpack_vm(vm, s)
    print(nblib.reset(vm))
    print(nblib.run(vm, 1000))
    nblib.destroy(vm)
end

for i = 1, 10000000 do
    simu()
end

