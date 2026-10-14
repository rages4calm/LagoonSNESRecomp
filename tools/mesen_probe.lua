-- What does mesen-agent's Lua environment actually contain?
--
-- The watch script asked for memory.registerwrite and got "unsupported", which
-- is only useful if it is true rather than a wrong name. This lists every global
-- and, for anything table-shaped, its function names, so the API is read from the
-- emulator instead of guessed at. Output goes to DSH_WATCH_OUT and also to
-- stdout, because whether io is honoured is itself one of the unknowns.

local out = os.getenv("DSH_WATCH_OUT") or "build/lua-probe.log"
local f = io and io.open and io.open(out, "w")
local names = {}
for k, v in pairs(_G) do names[#names + 1] = { name = tostring(k), type = type(v) } end
table.sort(names, function(a, b) return a.name < b.name end)

local lines = {}
lines[#lines + 1] = "# io=" .. tostring(io ~= nil) .. " globals=" .. #names
for _, e in ipairs(names) do
  lines[#lines + 1] = e.name .. " : " .. e.type
  if e.type == "table" then
    local sub = {}
    for k2, v2 in pairs(_G[e.name]) do sub[#sub + 1] = tostring(k2) .. "(" .. type(v2) .. ")" end
    table.sort(sub)
    local shown = table.concat(sub, " ")
    if #shown > 4000 then shown = shown:sub(1, 4000) .. " ..." end
    lines[#lines + 1] = "    " .. shown
  end
end
local text = table.concat(lines, "\n")
if f then f:write(text .. "\n"); f:close() end
print(text:sub(1, 4000))
