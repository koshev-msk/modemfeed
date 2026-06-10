#!/usr/bin/env lua
-- DeepSeek Telegram Bot (Lua 5.1.5)
-- Config: /etc/config/deepbot
-- Depends: curl lua luasocket lua-cjson lsqlite3 uci

local socket  = require("socket")
local json    = require("cjson")
local sqlite3 = require("lsqlite3")

-- ===== UCI КОНФИГ =====
local function uci_get(key)
    local f = io.popen("uci -q get deepbot." .. key .. " 2>/dev/null")
    if not f then return nil end
    local v = f:read("*l")
    f:close()
    return (v and v ~= "") and v or nil
end

local function uci_get_bool(key, default)
    local v = uci_get(key)
    if v == nil then return default end
    return v == "1" or v == "true" or v == "yes"
end

local function uci_get_int(key, default)
    local v = uci_get(key)
    return v and tonumber(v) or default
end

-- ===== Config =====
-- Prio: envilopment variables > UCI > default value
local TELEGRAM_BOT_TOKEN  = os.getenv("TELEGRAM_BOT_TOKEN")
                            or uci_get("main.token")
                            or "YOUR_BOT_TOKEN"
local DEEPSEEK_API_KEY    = os.getenv("DEEPSEEK_API_KEY")
                            or uci_get("main.deepseek_key")
                            or "YOUR_DEEPSEEK_KEY"
local DEEPSEEK_API_URL    = os.getenv("DEEPSEEK_API_URL")
                            or uci_get("main.deepseek_url")
                            or "https://api.deepseek.com/v1/chat/completions"
local PROXY_URL           = os.getenv("PROXY_URL")
                            or uci_get("main.proxy")
                            or ""
local DATABASE_NAME       = uci_get("main.database")
                            or "/usr/share/deepbot/chat_history.db"
local TOKEN_LIMIT_PER_DAY = uci_get_int("main.token_limit", 10000)
local RATE_LIMIT          = uci_get_int("main.rate_limit", 5)
local DEBUG               = os.getenv("DEBUG") == "1"
                            or uci_get_bool("main.debug", false)
-- reply in chant only ask or @bot call
local GROUP_MENTION_ONLY  = uci_get_bool("main.group_mention_only", true)

local TG_API_BASE = "https://api.telegram.org/bot" .. TELEGRAM_BOT_TOKEN

-- bot_username defined after getMe
local BOT_USERNAME = ""

-- ===== LOG =====
local function log(level, msg)
    io.stderr:write(string.format("[%s] [%s] %s\n",
        os.date("%Y-%m-%d %H:%M:%S"), level, tostring(msg)))
    io.stderr:flush()
end
local function info(msg) log("INFO",  msg) end
local function err(msg)  log("ERROR", msg) end
local function dbg(msg)  if DEBUG then log("DEBUG", msg) end end

-- ===== CURL =====
local function curl_post(url, headers, body)
    local header_args = ""
    for k, v in pairs(headers) do
        header_args = header_args .. string.format(" -H '%s: %s'", k, v)
    end
    local proxy_arg = (PROXY_URL ~= "") and (" -x '" .. PROXY_URL .. "'") or ""

    local pid_f = io.popen("echo $$")
    local pid   = pid_f and pid_f:read("*l") or "0"
    if pid_f then pid_f:close() end
    local tmpfile = string.format("/tmp/deepbot_%s_%s.json", pid, tostring(os.time()))

    local f = io.open(tmpfile, "w")
    if not f then
        err("curl_post: cannot write tmpfile " .. tmpfile)
        return nil
    end
    f:write(body)
    f:flush()
    f:close()

    local cmd = string.format(
        "curl -s -m 30 -X POST%s%s --data-binary @'%s' '%s'",
        header_args, proxy_arg, tmpfile, url
    )
    dbg("curl_post CMD: " .. cmd)

    local pipe = io.popen(cmd)
    if not pipe then
        os.remove(tmpfile)
        err("curl_post: popen failed")
        return nil
    end
    local resp = pipe:read("*a")
    pipe:close()
    os.remove(tmpfile)

    dbg("curl_post RESP (" .. #resp .. " bytes): " .. resp:sub(1, 300))
    return resp
end

-- ===== TELEGRAM API =====
local function tg_call(method, params)
    local body = params and json.encode(params) or "{}"
    dbg("tg_call " .. method .. " body: " .. body:sub(1, 200))
    local resp = curl_post(
        TG_API_BASE .. "/" .. method,
        { ["Content-Type"] = "application/json" },
        body
    )
    if not resp or resp == "" then
        err("tg_call " .. method .. ": empty response")
        return nil
    end
    local ok, data = pcall(json.decode, resp)
    if not ok then
        err("tg_call " .. method .. " JSON error: " .. tostring(data))
        err("raw: " .. resp:sub(1, 300))
        return nil
    end
    if not data.ok then
        err("tg_call " .. method .. " API error: " ..
            tostring(data.description or json.encode(data)))
    end
    return data
end

-- ===== DATABASE =====
local db

local function init_database()
    os.execute("mkdir -p /usr/share/deepbot")
    db = sqlite3.open(DATABASE_NAME)
    local rc = db:exec([[
        CREATE TABLE IF NOT EXISTS conversations (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id   INTEGER NOT NULL,
            role      TEXT NOT NULL,
            content   TEXT NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            tokens    INTEGER DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_user_id   ON conversations (user_id);
        CREATE INDEX IF NOT EXISTS idx_timestamp ON conversations (timestamp);
        CREATE TABLE IF NOT EXISTS token_usage (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id     INTEGER NOT NULL,
            date        DATE NOT NULL,
            tokens_used INTEGER DEFAULT 0,
            UNIQUE(user_id, date)
        );
        CREATE INDEX IF NOT EXISTS idx_token_usage ON token_usage (user_id, date);
    ]])
    if rc ~= sqlite3.OK then
        err("DB init error: " .. tostring(db:errmsg()))
    else
        info("Database: " .. DATABASE_NAME)
    end
end

local function get_user_messages(user_id, limit)
    limit = limit or 10
    local messages = {}
    local stmt = db:prepare(
        "SELECT role, content FROM conversations WHERE user_id=? ORDER BY timestamp DESC LIMIT ?"
    )
    stmt:bind_values(user_id, limit)
    for row in stmt:nrows() do
        table.insert(messages, 1, { role = row.role, content = row.content })
    end
    stmt:finalize()
    return messages
end

local function save_message(user_id, role, content, tokens)
    tokens = tokens or 0
    local stmt = db:prepare(
        "INSERT INTO conversations (user_id, role, content, tokens) VALUES (?,?,?,?)"
    )
    stmt:bind_values(user_id, role, content, tokens)
    stmt:step()
    stmt:finalize()
    local today = os.date("%Y-%m-%d")
    db:exec(string.format(
        "INSERT OR IGNORE INTO token_usage (user_id, date, tokens_used) VALUES (%d,'%s',0)",
        user_id, today
    ))
    db:exec(string.format(
        "UPDATE token_usage SET tokens_used=tokens_used+%d WHERE user_id=%d AND date='%s'",
        tokens, user_id, today
    ))
end

local function clear_user_conversation(user_id)
    db:exec(string.format("DELETE FROM conversations WHERE user_id=%d", user_id))
end

local function get_daily_token_usage(user_id)
    local today = os.date("%Y-%m-%d")
    local stmt = db:prepare(
        "SELECT tokens_used FROM token_usage WHERE user_id=? AND date=?"
    )
    stmt:bind_values(user_id, today)
    local result = 0
    for row in stmt:nrows() do result = row.tokens_used end
    stmt:finalize()
    return result
end

-- ===== utils =====
local function estimate_tokens(text)
    return math.max(1, math.floor(#text / 4))
end

local function sanitize_text(text)
    if #text > 4000 then text = text:sub(1, 4000) .. "…" end
    return text
end

-- ===== RATE LIMITER =====
local rate_table = {}

local function check_rate(user_id)
    local now = socket.gettime()
    local ts  = rate_table[user_id] or {}
    local fresh = {}
    for _, t in ipairs(ts) do
        if now - t < 60 then fresh[#fresh+1] = t end
    end
    if #fresh >= RATE_LIMIT then
        rate_table[user_id] = fresh
        return false
    end
    fresh[#fresh+1] = now
    rate_table[user_id] = fresh
    return true
end

-- ===== LRU CACHE =====
local response_cache = {}
local cache_order    = {}
local CACHE_MAX      = 200

local function cache_get(key) return response_cache[key] end
local function cache_set(key, val)
    if not response_cache[key] then
        table.insert(cache_order, key)
        if #cache_order > CACHE_MAX then
            response_cache[table.remove(cache_order, 1)] = nil
        end
    end
    response_cache[key] = val
end

-- ===== CHAT: reply filter =====
-- reply cleared text
local function group_filter(msg)
    local chat_type = msg.chat.type  -- "private","group","supergroup","channel"

    -- private msg reply always
    if chat_type == "private" then
        return true, msg.text
    end

    -- in chat — if GROUP_MENTION_ONLY disabled
    if not GROUP_MENTION_ONLY then
        return true, msg.text
    end

    local text = msg.text or ""

    -- 1. Reply
    if msg.reply_to_message and msg.reply_to_message.from then
        if msg.reply_to_message.from.username == BOT_USERNAME then
            dbg("group_filter: reply to bot")
            return true, text
        end
    end

    -- 2. ask bot @BotUsername (any case)
    if BOT_USERNAME ~= "" then
        local mention = "@" .. BOT_USERNAME
        local lower_text    = text:lower()
        local lower_mention = mention:lower()
        if lower_text:find(lower_mention, 1, true) then
            --  clean quote
            local clean = text:gsub("(?i)" .. mention, "")
            -- lua (?i) not support, make case-insensitive
            clean = lower_text:gsub(lower_mention, "")
            clean = clean:match("^%s*(.-)%s*$")  -- trim
            if clean == "" then clean = text end  -- if empty — keep
            dbg("group_filter: mention found, clean text: " .. clean)
            return true, clean
        end
    end

    dbg("group_filter: not addressed to bot, skipping")
    return false, nil
end

-- ===== DEEPSEEK API =====
local function call_deepseek(user_id, prompt)
    local cache_key = tostring(user_id) .. "\0" .. prompt
    if cache_get(cache_key) then
        info("Cache hit user=" .. user_id)
        return cache_get(cache_key)
    end

    local daily_usage = get_daily_token_usage(user_id)
    if daily_usage >= TOKEN_LIMIT_PER_DAY then
        return nil, "Daily token limit reached"
    end

    local conversation = get_user_messages(user_id, 10)
    if #conversation == 0 then
        local sys = "You are a helpful AI assistant for Telegram users."
        table.insert(conversation, { role = "system", content = sys })
        save_message(user_id, "system", sys)
    end

    local total_est = estimate_tokens(prompt) + 100
    for _, m in ipairs(conversation) do
        total_est = total_est + estimate_tokens(m.content)
    end
    if daily_usage + total_est > TOKEN_LIMIT_PER_DAY then
        return nil, "This request would exceed daily token limit"
    end

    local messages = {}
    for _, m in ipairs(conversation) do messages[#messages+1] = m end
    messages[#messages+1] = { role = "user", content = prompt }

    local payload = json.encode({
        model       = "deepseek-chat",
        messages    = messages,
        temperature = 0.5,
    })

    local resp_body = curl_post(
        DEEPSEEK_API_URL,
        {
            ["Authorization"] = "Bearer " .. DEEPSEEK_API_KEY,
            ["Content-Type"]  = "application/json",
        },
        payload
    )

    if not resp_body or resp_body == "" then
        return nil, "Empty response from DeepSeek API"
    end

    local ok, data = pcall(json.decode, resp_body)
    if not ok then
        err("DeepSeek JSON error: " .. tostring(data))
        err("Raw: " .. resp_body:sub(1, 300))
        return nil, "JSON parse error"
    end

    if data.choices then
        local ai_reply     = data.choices[1].message.content
        local usage        = data.usage or {}
        local reply_tokens = usage.completion_tokens or estimate_tokens(ai_reply)
        save_message(user_id, "assistant", ai_reply, reply_tokens)
        if #prompt < 100 then cache_set(cache_key, data) end
        info(string.format("DeepSeek OK user=%d tokens=%d", user_id, reply_tokens))
        return data
    elseif data.error then
        local emsg = data.error.message or json.encode(data.error)
        err("DeepSeek API error: " .. tostring(emsg))
        return nil, tostring(emsg)
    else
        err("DeepSeek unknown response: " .. resp_body:sub(1, 300))
        return nil, "Unknown API response"
    end
end

-- ===== Send =====
local function send_message(chat_id, text, reply_to)
    local payload = {
        chat_id                  = chat_id,
        text                     = sanitize_text(text),
        parse_mode               = "Markdown",
        disable_web_page_preview = true,
    }
    if reply_to then payload.reply_to_message_id = reply_to end
    local res = tg_call("sendMessage", payload)
    if not res or not res.ok then
        -- retry without Markdown
        payload.parse_mode = nil
        res = tg_call("sendMessage", payload)
    end
    return res
end

-- ===== HANDLERS =====
local function handle_start(msg)
    local user_id = msg.from.id
    info("handle_start user=" .. user_id)
    clear_user_conversation(user_id)
    save_message(user_id, "system", "You are a helpful AI assistant for Telegram users.")
    send_message(msg.chat.id,
        "🤖 *DeepSeek AI Bot*\n\n"
        .. "Conversation history is saved locally.\n"
        .. "/clear — start a new conversation\n"
        .. "/history — show recent messages\n"
        .. "/tokens — daily token usage",
        msg.message_id
    )
end

local function handle_clear(msg)
    info("handle_clear user=" .. msg.from.id)
    clear_user_conversation(msg.from.id)
    send_message(msg.chat.id, "🔄 *Conversation reset*", msg.message_id)
end

local function handle_history(msg)
    local messages = get_user_messages(msg.from.id, 20)
    if #messages == 0 then
        send_message(msg.chat.id, "📜 *Conversation history is empty*", msg.message_id)
        return
    end
    local lines = { "📜 *Recent Conversation:*\n" }
    local start = math.max(1, #messages - 4)
    for i = start, #messages do
        local m    = messages[i]
        local role = (m.role == "user") and "You" or "AI"
        local text = m.content:sub(1, 200)
        if #m.content > 200 then text = text .. "…" end
        lines[#lines+1] = string.format("*%s:* %s\n", role, text)
    end
    send_message(msg.chat.id, table.concat(lines, "\n"), msg.message_id)
end

local function handle_tokens(msg)
    local user_id     = msg.from.id
    local daily_usage = get_daily_token_usage(user_id)
    local pct = string.format("%.1f", daily_usage / TOKEN_LIMIT_PER_DAY * 100)
    send_message(msg.chat.id,
        string.format(
            "🧮 *Daily Token Usage:*\n\n"
            .. "• Used today: %d\n"
            .. "• Daily limit: %d\n"
            .. "• Percentage: %s%%\n\n"
            .. "Stats reset at midnight UTC.",
            daily_usage, TOKEN_LIMIT_PER_DAY, pct
        ),
        msg.message_id
    )
end

local function handle_text(msg, clean_text)
    local user_id = msg.from.id
    local chat_id = msg.chat.id
    local text    = clean_text or msg.text
    info(string.format("handle_text user=%d chat=%d text=%q",
        user_id, chat_id, text:sub(1, 80)))

    if not check_rate(user_id) then
        send_message(chat_id, "⏳ *Too many requests!* Please wait 1 minute.", msg.message_id)
        return
    end

    tg_call("sendChatAction", { chat_id = chat_id, action = "typing" })

    local data, api_err = call_deepseek(user_id, text)
    if api_err then
        err("DeepSeek error user=" .. user_id .. ": " .. api_err)
        send_message(chat_id, "❌ *Error:* " .. api_err, msg.message_id)
        return
    end

    local answer = data.choices[1].message.content
    send_message(chat_id, answer, msg.message_id)
end

-- ===== manager =====
local function dispatch(msg)
    if not msg then dbg("dispatch: nil message") return end
    if not msg.text then dbg("dispatch: no text") return end
    dbg(string.format("dispatch: chat_type=%s text=%q",
        tostring(msg.chat and msg.chat.type), msg.text:sub(1, 80)))

    local text = msg.text

    -- commands run any 
    if     text == "/start"   or text:find("^/start@")   then handle_start(msg)
    elseif text == "/clear"   or text:find("^/clear@")   then handle_clear(msg)
    elseif text == "/history" or text:find("^/history@") then handle_history(msg)
    elseif text == "/tokens"  or text:find("^/tokens@")  then handle_tokens(msg)
    elseif text:sub(1,1) ~= "/" then
        -- for standart msg — check public filter
        local should_reply, clean_text = group_filter(msg)
        if should_reply then
            handle_text(msg, clean_text)
        end
    end
end

-- ===== LONG POLLING =====
local function run()
    info("Lua version: " .. _VERSION)
    info("Token configured: "      .. (TELEGRAM_BOT_TOKEN ~= "YOUR_BOT_TOKEN"   and "YES" or "NO"))
    info("DeepSeek key configured: " .. (DEEPSEEK_API_KEY  ~= "YOUR_DEEPSEEK_KEY" and "YES" or "NO"))
    info("Debug mode: "            .. (DEBUG and "ON" or "OFF"))
    info("Proxy: "                 .. (PROXY_URL ~= "" and PROXY_URL or "none"))
    info("Group mention only: "    .. (GROUP_MENTION_ONLY and "ON" or "OFF"))

    local test = io.popen("curl --version 2>&1 | head -1")
    if test then info("curl: " .. (test:read("*l") or "?")); test:close() end

    info("Checking token via getMe...")
    local me = tg_call("getMe", {})
    if me and me.ok then
        BOT_USERNAME = me.result.username or ""
        info("Bot: @" .. BOT_USERNAME)
    else
        err("getMe failed — проверь TELEGRAM_BOT_TOKEN!")
        os.exit(1)
    end

    init_database()
    info("Bot started. Long polling...")

    local offset = 0
    while true do
        dbg("getUpdates offset=" .. offset)
        local res = tg_call("getUpdates", {
            offset          = offset,
            timeout         = 30,
            allowed_updates = { "message" },
        })

        if res and res.ok and res.result then
            local count = #res.result
            if count > 0 then info("getUpdates: " .. count .. " update(s)") end
            for _, update in ipairs(res.result) do
                offset = update.update_id + 1
                dbg("update_id=" .. update.update_id)
                local ok, e = pcall(dispatch, update.message)
                if not ok then err("dispatch error: " .. tostring(e)) end
            end
        else
            err("getUpdates failed, sleeping 5s...")
            if res then err("Response: " .. json.encode(res)) end
            socket.sleep(5)
        end
    end
end

run()
