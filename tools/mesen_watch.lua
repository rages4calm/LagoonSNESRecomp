-- RAM write watch for mesen-agent --lua.
--
-- Purpose: find which of the original ROM's routines writes a variable, using an
-- emulator that is not this project's. mesen-agent accepts --breakpoint but its
-- output does not change when such a breakpoint fires, so the debugger Lua hook
-- is the path that reports anything; the API namespace is `emu` (there is no
-- `memory` table), and the watch is emu.addMemoryCallback.
--
-- Addresses come from DSH_WATCH_ADDRS ("7E0520,7E0526"), output goes to
-- DSH_WATCH_OUT (default build/watch.log), DSH_WATCH_LIMIT caps lines. Hits are
-- grouped by writing PC, because the answer wanted is the routine, not a
-- thousand identical lines. Everything is wrapped in pcall and the API's own
-- shape is written to the log: a watch that silently did nothing is worse than
-- no watch, since it reads like evidence.
--
-- Run through tools/mesen_reference.ps1 -Lua tools/mesen_watch.lua

local addrs = (os.getenv("DSH_WATCH_ADDRS") or "7E0520")
local out = os.getenv("DSH_WATCH_OUT") or "build/watch.log"
local limit = tonumber(os.getenv("DSH_WATCH_LIMIT") or "600")

local f = io and io.open and io.open(out, "w")
if not f then print("watch: cannot open " .. out) return end

local function hex(v)
  if type(v) == "number" then return string.format("%06X", v) end
  return tostring(v)
end

local function keys(t)
  if type(t) ~= "table" then return "-" end
  local k = {}
  for n in pairs(t) do k[#k + 1] = tostring(n) end
  table.sort(k)
  return table.concat(k, ",")
end

f:write("# watch " .. addrs .. "\n# callbackType=" .. keys(emu.callbackType)
        .. "\n# eventType=" .. keys(emu.eventType) .. "\n")

local function pc_now()
  for _, spec in ipairs({ {"cpu.pc", function(s) return s.cpu and s.cpu.pc end},
                          {"PC", function(s) return s.PC end},
                          {"pc", function(s) return s.pc end} }) do
    local ok, st = pcall(emu.getState)
    if ok and type(st) == "table" then
      local ok2, v = pcall(spec[2], st)
      if ok2 and type(v) == "number" then return spec[1], v end
    end
  end
  return "unknown", nil
end

local frame = 0
-- The event names come from the emulator, not from the usual Lua conventions:
-- the probe log says endFrame, not onFrameEnd, and a wrong name here used to
-- leave the frame column at zero forever.
local ev = emu.eventType or {}
local frame_end = ev.endFrame or ev.onFrameEnd
if emu.addEventCallback and frame_end then
  pcall(emu.addEventCallback, function() frame = frame + 1 end, frame_end)
end

local seen, total = {}, 0
local function note(addr, ...)
  total = total + 1
  if total > limit then return end
  local src, pc = pc_now()
  local key = addr .. "/" .. tostring(pc)
  seen[key] = (seen[key] or 0) + 1
  if seen[key] > 3 then return end
  local args = {}
  for _, a in ipairs({ ... }) do args[#args + 1] = tostring(a) end
  f:write(string.format("addr=%s frame=%d %s=%s args=%s hits=%d\n",
        hex(addr), frame, src, hex(pc), table.concat(args, ","), seen[key]))
  f:flush()
end

for a in addrs:gmatch("%x+") do
  local n = tonumber(a, 16)
  if n and emu.addMemoryCallback and emu.callbackType then
    local ok, e = pcall(emu.addMemoryCallback, function(...) note(n, ...) end,
                        emu.callbackType.onWrite, n, n)
    f:write("# register " .. a .. " -> " .. tostring(ok) .. " " .. tostring(e) .. "\n")
  else
    f:write("# register " .. a .. " -> unsupported\n")
  end
end
f:flush()
