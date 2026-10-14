-- Does mesen-agent's emu.addMemoryCallback ever fire?
--
-- tools/mesen_watch.lua registers cleanly, reports error_count 0, and produced
-- nothing for an address the port's own dumps prove is rewritten every other
-- frame. Either the callback never runs, or it runs and dies somewhere the agent
-- does not report. This script runs all four possibilities side by side against
-- one address and prints the counts, so the next decision is made on a number
-- rather than on a guess about somebody else's emulator API.

local f = io.open(os.getenv("DSH_WATCH_OUT") or "build/cbtest.log", "w")
if not f then print("cbtest: no io") return end
f:write("loaded; callbackType=" .. tostring(emu.callbackType) .. "\n")
local counts = { write = 0, read = 0, writecpu = 0, frame = 0, executed = 0 }

local function bump(name)
  counts[name] = counts[name] + 1
  if counts[name] <= 2 then
    f:write(name .. " fired " .. counts[name] .. "\n"); f:flush()
  end
end

local A = 0x7E0503
local ok1, e1 = pcall(emu.addMemoryCallback, function(...) bump("write") end,
                     emu.callbackType.write, A, A)
local ok2, e2 = pcall(emu.addMemoryCallback, function(...) bump("read") end,
                      emu.callbackType.read, A, A)
local cpu = emu.cpuType and (emu.cpuType.cpu or emu.cpuType.SPC_NG)
local ok3, e3 = pcall(emu.addMemoryCallback, function(...) bump("writecpu") end,
                      emu.callbackType.write, A, A, cpu)
local ok4, e4 = pcall(emu.addEventCallback, function() bump("frame") end,
                      emu.eventType.endFrame)
local ok5, e5 = pcall(emu.addEventCallback, function() bump("executed") end,
                      emu.eventType.codeBreak)
-- If --breakpoint reaches Lua as a codeBreak event, the handler can read the
-- program counter itself, which is the one thing a memory callback would have
-- given. What getState returns is unknown, so its shape is recorded once.
local shape = false
pcall(emu.addEventCallback, function()
  bump("executed")
  local ok, st = pcall(emu.getState)
  if not shape then
    shape = true
    if ok and type(st) == "table" then
      local k = {}
      for n, v in pairs(st) do k[#k + 1] = n .. "(" .. type(v) .. ")" end
      table.sort(k)
      f:write("getState keys: " .. table.concat(k, " ") .. "\n"); f:flush()
    else
      f:write("getState -> " .. tostring(ok) .. " " .. tostring(st) .. "\n"); f:flush()
    end
  end
  local pc = "?"
  if ok and type(st) == "table" then
    for _, key in ipairs({ "cpu", "CPU", "system" }) do
      if type(st[key]) == "table" and st[key].pc then pc = string.format("%06X", st[key].pc) end
    end
    if pc == "?" and st.pc then pc = string.format("%06X", st.pc) end
  end
  f:write("codeBreak pc=" .. tostring(pc) .. " frame=" .. counts.frame .. "\n"); f:flush()
end, emu.eventType.codeBreak)
f:write("register write=" .. tostring(ok1) .. " read=" .. tostring(ok2)
        .. " write+cpu=" .. tostring(ok3) .. "(" .. tostring(cpu) .. ")"
        .. " endFrame=" .. tostring(ok4) .. " codeBreak=" .. tostring(ok5) .. "\n")
f:write("cpuType=" .. tostring(emu.cpuType and next(emu.cpuType)) .. "\n")

pcall(emu.addEventCallback, function()
  f:write("counts write=" .. counts.write .. " read=" .. counts.read
          .. " writecpu=" .. counts.writecpu .. " frame=" .. counts.frame
          .. " executed=" .. counts.executed .. "\n")
  f:flush()
  print("cbtest counts write=" .. counts.write .. " read=" .. counts.read
        .. " writecpu=" .. counts.writecpu .. " frame=" .. counts.frame)
end, emu.eventType.scriptEnded)
f:flush()
