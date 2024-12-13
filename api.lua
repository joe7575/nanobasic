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

local vm2lib, db, drive = ...

local M = minetest.get_meta
local VMList = {}
local UIDList = {}

-------------------------------------------------------------------------------
local VERSION     = 1.0  -- See readme.md
-------------------------------------------------------------------------------
vm2.HALTED    = 1  -- After halt instruction
vm2.HW_BREAK  = 2  -- Hardware break-point
vm2.ERROR     = 3  -- Error occurred
vm2.WAITING   = 4  -- Waiting for interrupt
vm2.RUNNING   = 5  -- Running
vm2.STOPPED   = 6  -- Stopped (not initialized)
vm2.INV_STATE = 7  -- Invalid API state

-- VM2 CPU flags
vm2.ZERO_FLG    = 0  -- zero flag
vm2.NEG_FLG     = 1  -- negative flag (msb set)
vm2.EOS_FLG	= 2  -- end-of-string flag
vm2.INT_FLG     = 3  -- interrupt flag
vm2.SSTEP_FLG   = 4  -- single-step flag

vm2.CallResults = {"HALTED", "BREAK", "ERROR", "WAITING", "RUNNING", "STOPPED", "INV_STATE"}

function vm2.version()
	return VERSION
end

-------------------------------------------------------------------------------
-- Wrapper for the VM2 library in C
-------------------------------------------------------------------------------

-- @param pos: node position
-- @param ram_size: 2^n words with n = 12..16
-- @return true, if VM was successfully initialized
function vm2.init(pos, ram_size)
	local hash = vm2lib.hash_node_position(pos)
	VMList[hash] = vm2lib.init(ram_size)
	return VMList[hash] ~= nil
end

-- @param pos: node position
-- @return true, if VM was successfully reset
function vm2.reset(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.reset(VMList[hash])
end

-- @param pos: node position
-- @param addr: address to set the breakpoint (0 to clear breakpoint)
-- @return true, if breakpoint was successfully set
function vm2.set_breakpoint(pos, addr)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.set_breakpoint(VMList[hash], addr)
end

-- @param pos: node position
-- @return address of the breakpoint or 0
function vm2.get_breakpoint(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_breakpoint(VMList[hash]) or 0
end

-- @param pos: node position
-- @return frame pointer value
function vm2.get_frame_pointer(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_frame_pointer(VMList[hash]) or 0
end

-- @param pos: node position
-- @return total number of executed CPU cycles
function vm2.get_cycles_total(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_cycles_total(VMList[hash]) or 0
end

-- @param pos: node position
-- @param addr: address to set the program counter
-- @return true, if PC was successfully set
function vm2.set_pc(pos, addr)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.set_pc(VMList[hash], addr)
end

-- @param pos: node position
-- @return current program counter
function vm2.get_pc(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_pc(VMList[hash]) or 0
end

-- @param pos: node position
-- @return current flags (ZERO, NEG, INT, SSTEP)
function vm2.get_flags(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_flags(VMList[hash]) or 0
end

-- @param pos: node position
-- @param addr: address to read from
-- @param num: number of words to read
-- @return table with the read values
function vm2.read_mem(pos, addr, num)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.read_mem(VMList[hash], addr, num) or {}
end

-- @param pos: node position
-- @param addr: address to write to
-- @param tbl: table with the values to write
-- @return number of written words
function vm2.write_mem(pos, addr, tbl)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.write_mem(VMList[hash], addr, tbl) or 0
end

-- @param pos: node position
-- @return true, if memory was successfully cleared
function vm2.clear_mem(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.clear_mem(VMList[hash])
end

-- @param pos: node position
-- @return table with the registers (R0 - R7)
function vm2.read_registers(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.read_regs(VMList[hash]) or {}
end

-- @param pos: node position
-- @param tbl: table with the registers (R0 - R7) to write
function vm2.write_registers(pos, tbl)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.write_regs(VMList[hash], tbl)
end

-- @param pos: node position
-- @param addr: address to read from
-- @return value at the address
function vm2.peek(pos, addr)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.peek(VMList[hash], addr) or 0
end

-- @param pos: node position
-- @param addr: address to write to
-- @param value: value to write
-- @return true, if value was successfully written
function vm2.poke(pos, addr, value)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.poke(VMList[hash], addr, value)
end

-- @param pos: node position
-- @return error code
function vm2.get_error_code(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_error_code(VMList[hash]) or 0
end

-- @param pos: node position
-- @return output register
function vm2.get_output_reg(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_output_reg(VMList[hash]) or 0
end

-- @param pos: node position
-- @param value: value to set
-- @return true, if input register was successfully set
function vm2.set_input_reg(pos, value)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.set_input_reg(VMList[hash], value)
end

-- @param pos: node position
-- @return 1 if success, -1 if error
function vm2.step(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.step(VMList[hash]) or -1
end

-- @param pos: node position
-- @param cycles: number of cycles to execute
-- @return number of executed cycles, or -1 if error
function vm2.exe(pos, cycles)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.exe(VMList[hash], cycles) or -1
end

-- Start the VM thread
-- @param pos: node position
-- @param cycles: number of cycles/s to execute (100000 .. 2000000)
-- @return 0, if VM was successfully started, -1 if error
function vm2.start(pos, cycles)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.start(VMList[hash], cycles) or -1
end

-- Stop the VM thread
-- @param pos: node position
-- @return 0, if VM was successfully stopped, -1 if error
function vm2.stop(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.stop(VMList[hash]) or -1
end

-- @param pos: node position
-- @return current state of the VM
function vm2.get_state(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.get_state(VMList[hash]) or 0
end

-- @param pos: node position
-- @param addr: tty I/O address (0..63)
-- @param data: data to write to the TTY
-- @return 1, if success, 0 if not, -1 if error
function vm2.tty_write(pos, addr, data)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.tty_write(VMList[hash], addr, data) or -1
end

-- @param pos: node position
-- @param addr: tty I/O address (0..63)
-- @return data read from the TTY, or -1 if no data available
function vm2.tty_read(pos, addr)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.tty_read(VMList[hash], addr) or 0
end

-- @param pos: node position
-- @param port: tape drive I/O port (0..7)
-- @param uid: unique ID of the tape drive
-- @param enable: true to enable the tape drive, false to disable
-- @return true, if success, false if error
function vm2.tapedrive_enable(pos, port, uid, enable)
	local hash = vm2lib.hash_node_position(pos)
	UIDList[hash] = UIDList[hash] or {}
	UIDList[hash][port] = enable and uid or nil
	local blocks = enable and drive.MAX_TAPE_BLOCK_NUM or 0
	return VMList[hash] and vm2lib.tapedrive_enable(VMList[hash], port, blocks) or false
end

-- @param pos: node position
-- @param port: disk drive I/O port (0..7)
-- @param uid: unique ID of the disk drive
-- @param enable: true to enable the disk drive, false to disable
-- @return true, if success, false if error
function vm2.diskdrive_enable(pos, port, uid, enable)
	local hash = vm2lib.hash_node_position(pos)
	UIDList[hash] = UIDList[hash] or {}
	UIDList[hash][port] = enable and uid or nil
	local blocks = enable and drive.MAX_DISK_BLOCK_NUM or 0
	return VMList[hash] and vm2lib.diskdrive_enable(VMList[hash], port, blocks) or false
end

-- @param pos: node position
-- @param port: techage I/O port (0..63)
-- @param topic: command topic value (0..65535)
-- @param payload: command payload as table (max 32 words) or
--                 as string (max 64 characters)
-- @return true, if success, false if error
function vm2.techage_write_cmnd(pos, port, topic, payload)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.techage_write_cmnd(VMList[hash], port, topic, payload) or false
end

-- @param pos: node position
-- @return port, topic and payload as table or string, or nil if no data available
function vm2.techage_read_cmnd(pos)
	local hash = vm2lib.hash_node_position(pos)
	if VMList[hash] then
        	return vm2lib.techage_read_cmnd(VMList[hash])
        end
end

-- @param pos: node position
-- @param port: techage I/O port (0..63)
-- @param signal: signal value (0..65535)
-- @return true, if success, false if error
function vm2.techage_write_signal(pos, port, signal)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] and vm2lib.techage_write_signal(VMList[hash], port, signal) or false
end

-- @param pos: node position
-- @return port and signal as values, or nil if no data available
function vm2.techage_read_signal(pos)
	local hash = vm2lib.hash_node_position(pos)
	if VMList[hash] then
		return vm2lib.techage_read_signal(VMList[hash])
        end
end

-- @param value: value to test
-- @param bit: bit number to test (0..15)
-- @return true, if bit is set, false if not
function vm2.testbit(value, bit)
	return vm2lib.testbit(value, bit) or false
end

-- @param pos: node position
-- @return hash value for the node position
function vm2.hash_node_position(pos)
	return vm2lib.hash_node_position(pos)
end

-------------------------------------------------------------------------------
-- Further API functions
-------------------------------------------------------------------------------

-- @param hash: hash value for the node position
-- @return node position as table (x,y,z)
function vm2.get_position_from_hash(hash)
	local x = (hash:byte(1) - 48) + (hash:byte(2) - 48) * 64 + (hash:byte(3) - 48) * 4096
	local y = (hash:byte(4) - 48) + (hash:byte(5) - 48) * 64 + (hash:byte(6) - 48) * 4096
	local z = (hash:byte(7) - 48) + (hash:byte(8) - 48) * 64 + (hash:byte(9) - 48) * 4096
	return {x = x - 32768, y = y - 32768, z = z - 32768}
end

-- Create a new VM without starting it
-- @param pos: node position
-- @param ram_size: ram_size is 2^n words with n = 12..16
-- @return true, if VM was successfully created
function vm2.create(pos, ram_size)
	local hash = vm2lib.hash_node_position(pos)
	VMList[hash] = vm2lib.init(ram_size)
	local meta = minetest.get_meta(pos)
	meta:set_int("vm2size", ram_size)
	return VMList[hash] ~= nil
end

-- Delete the VM in memory
-- @param pos: node position
function vm2.destroy(pos)
	minetest.get_meta(pos):set_string("vm2size", "")
	local hash = vm2lib.hash_node_position(pos)
	if VMList[hash] then
		vm2.stop(pos)
		VMList[hash] = nil
	end
end

-- @param pos: node position
-- @return true, if VM is loaded
function vm2.is_loaded(pos)
	local hash = vm2lib.hash_node_position(pos)
	return VMList[hash] ~= nil
end

-- Restore a previously backed up VM without starting it
-- @param pos: node position
function vm2.vm_restore(pos)
	local meta = minetest.get_meta(pos)
	local hash = vm2lib.hash_node_position(pos)
	if not VMList[hash] then
		local s = db.read_mem(hash) or ""
		local size = meta:get_int("vm2size")
		if s ~= "" and size > 0 then
			VMList[hash] = vm2lib.init(size)
			vm2lib.unpack_vm(VMList[hash], s)
		end
	end
end

-- Create a new VM without starting it
-- @param pos: node position
-- @param ram_size: ram_size is 2^n words with n = 12..16
function vm2.on_power_on(pos, ram_size)
	if not vm2.is_loaded(pos) then
		if vm2.create(pos, ram_size) then
			return true
		end
	end
end

-- Delete the VM in memory
-- @param pos: node position
function vm2.on_power_off(pos)
	if vm2.is_loaded(pos) then
		vm2.destroy(pos)
		return true
	end
end

-- Load the VM from the database
-- @param pos: node position
-- @return true, if VM was successfully loaded
function vm2.load(pos)
	if not vm2.is_loaded(pos) then
		vm2.vm_restore(pos)
		return true
	end
end

-- Keep the drives working
-- @param pos: node position
-- @return true, if drives are working
function vm2.run_drives(pos)
	local hash = vm2lib.hash_node_position(pos)
	if VMList[hash] and UIDList[hash] then
		drive.update(VMList[hash], UIDList[hash])
	end
end

-------------------------------------------------------------------------------
-- World maintenance
-------------------------------------------------------------------------------

-- Store the VM in the database
local function vm_store(hash, vm)
	vm2lib.stop(vm)
	local s = vm2lib.pack_vm(vm)
	db.write_mem(hash, s)
end



minetest.register_on_shutdown(function()
		for hash, vm in pairs(VMList) do
			vm_store(hash, vm)
		end
	end)

local function remove_unloaded_vms()
	local tbl = table.copy(VMList)
	local cnt = 0
	VMList = {}
	for hash, vm in pairs(tbl) do
		local pos = vm2.get_position_from_hash(hash)
		if minetest.get_node_or_nil(pos) then
			VMList[hash] = vm
			cnt = cnt + 1
		else
			vm_store(hash, vm)
		end
	end
	minetest.after(60, remove_unloaded_vms)
end

minetest.after(60, remove_unloaded_vms)
