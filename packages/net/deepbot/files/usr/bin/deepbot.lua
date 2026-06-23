#!/usr/bin/env lua
-- AI Telegram Bot for OpenWrt (Lua 5.1.5)
-- Config: /etc/config/deepbot
-- Depends: curl lua luasocket lua-cjson lsqlite3 uci

local socket  = require("socket")
local json    = require("cjson")
local sqlite3 = require("lsqlite3")

-- ===== UCI =====
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
local TELEGRAM_BOT_TOKEN  = os.getenv("TELEGRAM_BOT_TOKEN")
                            or uci_get("main.token")
                            or "YOUR_BOT_TOKEN"
local DEEPSEEK_API_KEY    = os.getenv("DEEPSEEK_API_KEY")
                            or uci_get("main.deepseek_key")
                            or "YOUR_DEEPSEEK_KEY"
local DEEPSEEK_API_URL    = os.getenv("DEEPSEEK_API_URL")
                            or uci_get("main.deepseek_url")
                            or "https://api.deepseek.com/v1/chat/completions"
local AI_MODEL            = os.getenv("AI_MODEL")
                            or uci_get("main.model")
                            or "deepseek-chat"
local PROXY_URL           = os.getenv("PROXY_URL")
                            or uci_get("main.proxy")
                            or ""
local DATABASE_NAME       = os.getenv("DATABASE_NAME")
                            or uci_get("main.database")
                            or "/usr/share/deepbot/chat_history.db"
-- Global limit — used when no personal limit is set for the user
local TOKEN_LIMIT_PER_DAY = tonumber(os.getenv("TOKEN_LIMIT_PER_DAY"))
                            or uci_get_int("main.token_limit", 10000)
local RATE_LIMIT          = tonumber(os.getenv("RATE_LIMIT"))
                            or uci_get_int("main.rate_limit", 5)
local DEBUG               = os.getenv("DEBUG") == "1"
                            or uci_get_bool("main.debug", false)
local GROUP_MENTION_ONLY  = os.getenv("GROUP_MENTION_ONLY") == "1"
                            or uci_get_bool("main.group_mention_only", true)
local ADMIN_ID            = os.getenv("ADMIN_ID")
                            or uci_get("main.admin_id")
                            or ""
local WHITELIST_MODE      = os.getenv("WHITELIST_MODE")
                            or uci_get("main.whitelist_mode")
                            or "none"  -- none, all, private, group (whitelist mode)

local TG_API_BASE  = "https://api.telegram.org/bot" .. TELEGRAM_BOT_TOKEN
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
local function tg_call(method, params, silent)
    silent = silent or false
    local body = params and json.encode(params) or "{}"
    if DEBUG then dbg("tg_call " .. method .. " body: " .. body:sub(1, 200)) end

    local resp = curl_post(
        TG_API_BASE .. "/" .. method,
        { ["Content-Type"] = "application/json" },
        body
    )
    if not resp or resp == "" then
        if not silent then err("tg_call " .. method .. ": empty response") end
        return nil
    end
    local ok, data = pcall(json.decode, resp)
    if not ok then
        if not silent then
            err("tg_call " .. method .. " JSON error: " .. tostring(data))
            if DEBUG then err("raw: " .. resp:sub(1, 300)) end
        end
        return nil
    end
    if not data.ok then
        if not silent then
            err("tg_call " .. method .. " API error: " ..
                tostring(data.description or json.encode(data)))
        end
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

        CREATE TABLE IF NOT EXISTS allowed_users (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id   INTEGER NOT NULL UNIQUE,
            added_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            added_by  INTEGER
        );
        CREATE INDEX IF NOT EXISTS idx_allowed_user_id ON allowed_users (user_id);

        CREATE TABLE IF NOT EXISTS user_limits (
            user_id     INTEGER PRIMARY KEY,
            daily_limit INTEGER NOT NULL,
            set_at      DATETIME DEFAULT CURRENT_TIMESTAMP,
            set_by      INTEGER
        );

        CREATE TABLE IF NOT EXISTS user_rate_limits (
            user_id    INTEGER PRIMARY KEY,
            rate_limit INTEGER NOT NULL,
            set_at     DATETIME DEFAULT CURRENT_TIMESTAMP,
            set_by     INTEGER
        );
    ]])
    if rc ~= sqlite3.OK then
        err("DB init error: " .. tostring(db:errmsg()))
    else
        info("Database: " .. DATABASE_NAME)
    end
end

local function validate_utf8(text)
    if not text then return "" end
    -- Remove bytes that cannot appear in valid UTF-8:
    -- 0xC0, 0xC1 (overlong encoding), 0xF5-0xFF (out of Unicode range)
    -- Continuation bytes (0x80-0xBF) are left alone —
    -- cjson handles them; we just strip clearly invalid single bytes.
    local clean = text:gsub("[ÀÁõ-ÿ]", "?")
    return clean
end

-- ===== USER LIMITS =====
-- Returns per-user token limit, falls back to global default
local function get_user_limit(user_id)
    local stmt = db:prepare("SELECT daily_limit FROM user_limits WHERE user_id=?")
    stmt:bind_values(user_id)
    local limit = nil
    for row in stmt:nrows() do limit = row.daily_limit end
    stmt:finalize()
    return limit or TOKEN_LIMIT_PER_DAY
end

-- Set a personal daily token limit for a user
local function set_user_limit(user_id, daily_limit, set_by)
    local stmt = db:prepare([[
        INSERT INTO user_limits (user_id, daily_limit, set_by)
        VALUES (?, ?, ?)
        ON CONFLICT(user_id) DO UPDATE SET
            daily_limit = excluded.daily_limit,
            set_at      = CURRENT_TIMESTAMP,
            set_by      = excluded.set_by
    ]])
    stmt:bind_values(user_id, daily_limit, set_by)
    stmt:step()
    stmt:finalize()
end

-- Remove personal limit, revert to global default
local function reset_user_limit(user_id)
    db:exec(string.format("DELETE FROM user_limits WHERE user_id=%d", user_id))
end

-- ===== USER RATE LIMITS =====
local function get_user_rate_limit(user_id)
    local stmt = db:prepare("SELECT rate_limit FROM user_rate_limits WHERE user_id=?")
    stmt:bind_values(user_id)
    local limit = nil
    for row in stmt:nrows() do limit = row.rate_limit end
    stmt:finalize()
    return limit or RATE_LIMIT
end

local function set_user_rate_limit(user_id, rate_limit, set_by)
    local stmt = db:prepare([[
        INSERT INTO user_rate_limits (user_id, rate_limit, set_by)
        VALUES (?, ?, ?)
        ON CONFLICT(user_id) DO UPDATE SET
            rate_limit = excluded.rate_limit,
            set_at     = CURRENT_TIMESTAMP,
            set_by     = excluded.set_by
    ]])
    stmt:bind_values(user_id, rate_limit, set_by)
    stmt:step()
    stmt:finalize()
end

local function reset_user_rate_limit(user_id)
    db:exec(string.format("DELETE FROM user_rate_limits WHERE user_id=%d", user_id))
end

local function get_all_user_rate_limits()
    local result = {}
    for row in db:nrows([[
        SELECT user_id, rate_limit, set_at, set_by
        FROM user_rate_limits
        ORDER BY user_id
    ]]) do
        table.insert(result, {
            user_id    = row.user_id,
            rate_limit = row.rate_limit,
            set_at     = row.set_at,
            set_by     = row.set_by,
        })
    end
    return result
end

-- Get all users who have a personal token limit set
local function get_all_user_limits()
    local result = {}
    for row in db:nrows([[
        SELECT ul.user_id, ul.daily_limit, ul.set_at, ul.set_by,
               COALESCE(tu.tokens_used, 0) AS used_today
        FROM user_limits ul
        LEFT JOIN token_usage tu
            ON tu.user_id = ul.user_id AND tu.date = date('now')
        ORDER BY ul.user_id
    ]]) do
        table.insert(result, {
            user_id     = row.user_id,
            daily_limit = row.daily_limit,
            set_at      = row.set_at,
            set_by      = row.set_by,
            used_today  = row.used_today,
        })
    end
    return result
end

-- ===== CONVERSATIONS =====
local function get_user_messages(user_id, limit)
    limit = limit or 10
    local messages = {}
    local stmt = db:prepare(
        "SELECT role, content FROM conversations WHERE user_id=? ORDER BY timestamp ASC LIMIT ?"
    )
    stmt:bind_values(user_id, limit)
    for row in stmt:nrows() do
        table.insert(messages, { role = row.role, content = row.content })
    end
    stmt:finalize()
    return messages
end

local function get_history_messages(user_id, limit)
    limit = limit or 20
    local messages = {}
    local stmt = db:prepare(
        "SELECT role, content FROM conversations WHERE user_id=? ORDER BY timestamp DESC LIMIT ?"
    )
    stmt:bind_values(user_id, limit)
    for row in stmt:nrows() do
        table.insert(messages, { role = row.role, content = row.content })
    end
    stmt:finalize()
    return messages
end

local function normalize_text(text)
    if not text then return "" end
    -- \r\n (Windows) and \r (old Mac) -> \n
    text = text:gsub("\r\n", "\n"):gsub("\r", "\n")
    text = validate_utf8(text)
    return text
end

local function save_message(user_id, role, content, tokens)
    tokens = tokens or 0
    content = normalize_text(content)

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

-- ===== WHITELIST =====
local function add_allowed_user(user_id, added_by)
    local stmt = db:prepare(
        "INSERT OR IGNORE INTO allowed_users (user_id, added_by) VALUES (?, ?)"
    )
    stmt:bind_values(user_id, added_by)
    stmt:step()
    stmt:finalize()
end

local function remove_allowed_user(user_id)
    db:exec(string.format("DELETE FROM allowed_users WHERE user_id=%d", user_id))
end

local function get_allowed_users_full()
    local users = {}
    for row in db:nrows("SELECT user_id, added_at, added_by FROM allowed_users ORDER BY added_at DESC") do
        table.insert(users, {
            user_id  = row.user_id,
            added_at = row.added_at,
            added_by = row.added_by,
        })
    end
    return users
end

local function get_allowed_users_list()
    local users = {}
    for row in db:nrows("SELECT user_id FROM allowed_users ORDER BY user_id") do
        table.insert(users, tostring(row.user_id))
    end
    return users
end

-- ===== ACCESS CONTROL =====
local function is_admin(user_id)
    return ADMIN_ID ~= "" and tostring(user_id) == ADMIN_ID
end

local function is_user_allowed(user_id, chat_type)
    if WHITELIST_MODE == "none" then return true end

    if WHITELIST_MODE == "all" then
        if is_admin(user_id) then return true end
        local stmt = db:prepare("SELECT 1 FROM allowed_users WHERE user_id=?")
        stmt:bind_values(user_id)
        local allowed = false
        for _ in stmt:nrows() do allowed = true end
        stmt:finalize()
        return allowed
    end

    if WHITELIST_MODE == "private" then
        if chat_type ~= "private" then return true end
        if is_admin(user_id) then return true end
        local stmt = db:prepare("SELECT 1 FROM allowed_users WHERE user_id=?")
        stmt:bind_values(user_id)
        local allowed = false
        for _ in stmt:nrows() do allowed = true end
        stmt:finalize()
        return allowed
    end

    if WHITELIST_MODE == "group" then
        if chat_type == "private" then return is_admin(user_id) end
        if is_admin(user_id) then return true end
        local stmt = db:prepare("SELECT 1 FROM allowed_users WHERE user_id=?")
        stmt:bind_values(user_id)
        local allowed = false
        for _ in stmt:nrows() do allowed = true end
        stmt:finalize()
        return allowed
    end

    return false
end

-- ===== UTILS =====
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
    local now        = socket.gettime()
    local user_limit = get_user_rate_limit(user_id)
    local ts         = rate_table[user_id] or {}
    local fresh = {}
    for _, t in ipairs(ts) do
        if now - t < 60 then fresh[#fresh+1] = t end
    end
    if #fresh >= user_limit then
        rate_table[user_id] = fresh
        return false, user_limit
    end
    fresh[#fresh+1] = now
    rate_table[user_id] = fresh
    return true, user_limit
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

-- ===== GROUP FILTER =====
local function group_filter(msg)
    local chat_type = msg.chat.type
    if chat_type == "private" then return true, msg.text end
    if not GROUP_MENTION_ONLY then return true, msg.text end

    local text = msg.text or ""

    if msg.reply_to_message and msg.reply_to_message.from then
        if msg.reply_to_message.from.username == BOT_USERNAME then
            dbg("group_filter: reply to bot")
            return true, text
        end
    end

    if BOT_USERNAME ~= "" then
        local mention     = "@" .. BOT_USERNAME
        local lower_text  = text:lower()
        local lower_ment  = mention:lower()
        if lower_text:find(lower_ment, 1, true) then
            local clean = lower_text:gsub(lower_ment, ""):match("^%s*(.-)%s*$")
            if clean == "" then clean = text end
            dbg("group_filter: mention found")
            return true, normalize_text(clean)
        end
    end

    dbg("group_filter: not addressed to bot")
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
    local user_limit  = get_user_limit(user_id)

    if daily_usage >= user_limit then
        return nil, string.format("Daily token limit reached (%d/%d)", daily_usage, user_limit)
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
    if daily_usage + total_est > user_limit then
        return nil, string.format(
            "This request would exceed your daily token limit (%d/%d)",
            daily_usage, user_limit)
    end

    local messages = {}
    for _, m in ipairs(conversation) do messages[#messages+1] = m end
    messages[#messages+1] = { role = "user", content = prompt }

    local ok_enc, payload = pcall(json.encode, {
        model       = AI_MODEL,
        messages    = messages,
        temperature = 0.5,
    })
    if not ok_enc then
        err("json.encode failed: " .. tostring(payload))
        return nil, "Failed to encode request"
    end

    local resp_body = curl_post(DEEPSEEK_API_URL, {
        ["Authorization"] = "Bearer " .. DEEPSEEK_API_KEY,
        ["Content-Type"]  = "application/json",
    }, payload)

    if not resp_body or resp_body == "" then
        return nil, "Empty response from API"
    end

    local ok, data = pcall(json.decode, resp_body)
    if not ok then
        err("JSON error: " .. tostring(data))
        return nil, "JSON parse error"
    end

    if data.error then
        local emsg = data.error.message or json.encode(data.error)
        err("API error: " .. tostring(emsg))
        return nil, tostring(emsg)
    end

    if data.choices then
        local ai_reply     = data.choices[1].message.content
        local usage        = data.usage or {}
        local reply_tokens = usage.completion_tokens or estimate_tokens(ai_reply)
        save_message(user_id, "assistant", ai_reply, reply_tokens)
        if #prompt < 100 then cache_set(cache_key, data) end
        info(string.format("AI OK user=%d tokens=%d/%d model=%s",
            user_id, daily_usage + reply_tokens, user_limit, data.model or AI_MODEL))
        return data
    else
        err("Unknown response: " .. resp_body:sub(1, 300))
        return nil, "Unknown API response"
    end
end

-- ===== SEND =====
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

    local mode_desc = {
        none    = "⚪ Disabled (all users)",
        all     = "🔴 Strict (whitelist only)",
        private = "🟡 Private only",
        group   = "🟠 Group only",
    }
    send_message(msg.chat.id,
        "🤖 *AI Bot*\n\n"
        .. "Conversation history is saved locally.\n\n"
        .. "*Commands:*\n"
        .. "/clear — start a new conversation\n"
        .. "/history — show recent messages\n"
        .. "/tokens — daily token usage\n"
        .. "/whoami — show your Telegram ID\n"
        .. "/admin — admin info\n"
        .. "/allowed — whitelist (admin only)\n"
        .. "/whitelist — manage whitelist (admin only)\n"
        .. "/limit — manage per-user token limits (admin only)\n\n"
        .. "*Whitelist mode:* " .. (mode_desc[WHITELIST_MODE] or WHITELIST_MODE),
        msg.message_id
    )
end

local function handle_clear(msg)
    info("handle_clear user=" .. msg.from.id)
    clear_user_conversation(msg.from.id)
    send_message(msg.chat.id, "🔄 *Conversation reset*", msg.message_id)
end

local function handle_history(msg)
    local messages = get_history_messages(msg.from.id, 20)
    if #messages == 0 then
        send_message(msg.chat.id, "📜 *Conversation history is empty*", msg.message_id)
        return
    end
    local lines = { "📜 *Recent Conversation:*\n" }
    for i, m in ipairs(messages) do
        local role = (m.role == "user") and "👤 You" or "🤖 AI"
        local text = m.content:sub(1, 200)
        if #m.content > 200 then text = text .. "…" end
        lines[#lines+1] = string.format("%s: %s\n", role, text)
        if i >= 10 then
            lines[#lines+1] = "\n*... and more*"
            break
        end
    end
    send_message(msg.chat.id, table.concat(lines, "\n"), msg.message_id)
end

local function handle_tokens(msg)
    local user_id      = msg.from.id
    local daily_usage  = get_daily_token_usage(user_id)
    local user_limit   = get_user_limit(user_id)
    local user_rate    = get_user_rate_limit(user_id)
    local tok_custom   = user_limit ~= TOKEN_LIMIT_PER_DAY
    local rate_custom  = user_rate  ~= RATE_LIMIT
    local pct = string.format("%.1f", daily_usage / user_limit * 100)
    send_message(msg.chat.id,
        string.format(
            "🧮 *Your Limits:*\n\n"
            .. "• Tokens today: %d / %d %s\n"
            .. "• Usage: %s%%\n"
            .. "• Rate limit: %d req/min %s\n\n"
            .. "_Tokens reset at midnight UTC._",
            daily_usage,
            user_limit, tok_custom  and "_(personal)_" or "_(global)_",
            pct,
            user_rate,  rate_custom and "_(personal)_" or "_(global)_"
        ),
        msg.message_id
    )
end

local function handle_whoami(msg)
    local user_id    = msg.from.id
    local username   = msg.from.username or "no username"
    local first_name = msg.from.first_name or ""
    send_message(msg.chat.id,
        string.format(
            "🆔 *Your Telegram ID*\n\n"
            .. "• ID: `%d`\n"
            .. "• Username: @%s\n"
            .. "• Name: %s\n\n"
            .. "Share this ID with admin to be added to whitelist.",
            user_id, username, first_name
        ),
        msg.message_id
    )
end

local function handle_admin_info(msg)
    local user_id = msg.from.id
    local message = "👑 *Admin System*\n\n"
    if ADMIN_ID ~= "" then
        message = message .. "• Admin ID: `" .. ADMIN_ID .. "`\n"
        message = message .. "• Your ID: `" .. user_id .. "`\n\n"
        if is_admin(user_id) then
            message = message .. "✅ *You are the admin*"
        else
            message = message .. "❌ *You are not the admin*"
        end
    else
        message = message .. "⚠️ *No admin configured*\n"
        message = message .. "Set `admin_id` in UCI config."
    end
    send_message(msg.chat.id, message, msg.message_id)
end

local function handle_allowed(msg)
    if not is_admin(msg.from.id) then
        send_message(msg.chat.id, "⛔ *Admin only*", msg.message_id)
        return
    end
    if WHITELIST_MODE == "none" then
        send_message(msg.chat.id, "⚪ *Whitelist is disabled*", msg.message_id)
        return
    end
    local users = get_allowed_users_full()
    if #users == 0 then
        send_message(msg.chat.id, "📭 *Whitelist is empty*", msg.message_id)
        return
    end
    local lines = { string.format("👥 *Allowed Users* (%d):\n", #users) }
    for i, u in ipairs(users) do
        local date_str = (u.added_at or ""):match("(%d+-%d+-%d+)") or "?"
        lines[#lines+1] = string.format("%d. `%d` — %s", i, u.user_id, date_str)
        if #table.concat(lines, "\n") > 3500 then
            lines[#lines+1] = "*... and more*"
            break
        end
    end
    send_message(msg.chat.id, table.concat(lines, "\n"), msg.message_id)
end

-- ===== /limit HANDLER =====
local function handle_limit(msg, text)
    local user_id = msg.from.id

    if not is_admin(user_id) then
        send_message(msg.chat.id, "⛔ *Admin only*", msg.message_id)
        return
    end

    -- /limit  or  /limit list
    if text == "/limit" or text == "/limit list" then
        local limits = get_all_user_limits()
        if #limits == 0 then
            send_message(msg.chat.id,
                string.format(
                    "📊 *Per-user Token Limits*\n\n"
                    .. "No individual limits set.\n"
                    .. "Global default: *%d* tokens/day\n\n"
                    .. "*Commands:*\n"
                    .. "`/limit set USER_ID LIMIT` — set personal limit\n"
                    .. "`/limit reset USER_ID` — reset to global default\n"
                    .. "`/limit list` — show all personal limits",
                    TOKEN_LIMIT_PER_DAY
                ),
                msg.message_id
            )
            return
        end
        local lines = {
            string.format("📊 *Per-user Token Limits*\n_(Global default: %d)_\n", TOKEN_LIMIT_PER_DAY)
        }
        for _, u in ipairs(limits) do
            local pct = string.format("%.0f%%", u.used_today / u.daily_limit * 100)
            lines[#lines+1] = string.format(
                "• `%d` — limit: *%d* | today: %d (%s)",
                u.user_id, u.daily_limit, u.used_today, pct
            )
        end
        send_message(msg.chat.id, table.concat(lines, "\n"), msg.message_id)
        return
    end

    -- /limit set USER_ID TOKENS
    local set_uid, set_lim = text:match("^/limit set (%d+) (%d+)")
    if set_uid and set_lim then
        set_uid = tonumber(set_uid)
        set_lim = tonumber(set_lim)
        if set_lim < 0 then
            send_message(msg.chat.id, "❌ Limit must be >= 0", msg.message_id)
            return
        end
        set_user_limit(set_uid, set_lim, user_id)
        info(string.format("User limit: user=%d limit=%d set_by=%d", set_uid, set_lim, user_id))
        send_message(msg.chat.id,
            string.format("✅ User `%d` daily limit set to *%d* tokens", set_uid, set_lim),
            msg.message_id
        )
        return
    end

    -- /limit reset USER_ID
    local reset_uid = text:match("^/limit reset (%d+)")
    if reset_uid then
        reset_uid = tonumber(reset_uid)
        reset_user_limit(reset_uid)
        info(string.format("User limit reset: user=%d by=%d", reset_uid, user_id))
        send_message(msg.chat.id,
            string.format(
                "🔄 User `%d` limit reset to global default (*%d* tokens/day)",
                reset_uid, TOKEN_LIMIT_PER_DAY
            ),
            msg.message_id
        )
        return
    end

    -- /limit rate USER_ID N
    local rate_uid, rate_val = text:match("^/limit rate (%d+) (%d+)")
    if rate_uid and rate_val then
        rate_uid = tonumber(rate_uid)
        rate_val = tonumber(rate_val)
        if rate_val < 1 then
            send_message(msg.chat.id, "❌ Rate must be >= 1", msg.message_id)
            return
        end
        set_user_rate_limit(rate_uid, rate_val, user_id)
        info(string.format("User rate: user=%d rate=%d set_by=%d", rate_uid, rate_val, user_id))
        send_message(msg.chat.id,
            string.format("✅ User `%d` rate limit set to *%d* req/min", rate_uid, rate_val),
            msg.message_id
        )
        return
    end

    -- /limit rate reset USER_ID
    local rate_reset_uid = text:match("^/limit rate reset (%d+)")
    if rate_reset_uid then
        rate_reset_uid = tonumber(rate_reset_uid)
        reset_user_rate_limit(rate_reset_uid)
        info(string.format("User rate reset: user=%d by=%d", rate_reset_uid, user_id))
        send_message(msg.chat.id,
            string.format(
                "🔄 User `%d` rate reset to global default (*%d* req/min)",
                rate_reset_uid, RATE_LIMIT
            ),
            msg.message_id
        )
        return
    end

    -- /limit rates — list all personal rate limits
    if text == "/limit rates" then
        local rates = get_all_user_rate_limits()
        if #rates == 0 then
            send_message(msg.chat.id,
                string.format("📊 *Per-user Rate Limits*\n\nNo individual rate limits set.\nGlobal default: *%d* req/min", RATE_LIMIT),
                msg.message_id
            )
            return
        end
        local lines = { string.format("📊 *Per-user Rate Limits*\n_(Global default: %d req/min)_\n", RATE_LIMIT) }
        for _, u in ipairs(rates) do
            lines[#lines+1] = string.format("• `%d` — *%d* req/min", u.user_id, u.rate_limit)
        end
        send_message(msg.chat.id, table.concat(lines, "\n"), msg.message_id)
        return
    end

    -- unknown subcommand — print usage
    send_message(msg.chat.id,
        "*Token limits:*\n"
        .. "`/limit` — show token limits\n"
        .. "`/limit set USER_ID TOKENS` — set token limit\n"
        .. "`/limit reset USER_ID` — reset token limit\n\n"
        .. "*Rate limits:*\n"
        .. "`/limit rates` — show rate limits\n"
        .. "`/limit rate USER_ID N` — set N req/min\n"
        .. "`/limit rate reset USER_ID` — reset rate limit",
        msg.message_id
    )
end

-- ===== WHITELIST HANDLERS =====
local function handle_whitelist_status(msg)
    if not is_admin(msg.from.id) then
        send_message(msg.chat.id, "⛔ *Admin only*", msg.message_id)
        return
    end
    local mode_desc = {
        none    = "⚪ *Disabled* — all users",
        all     = "🔴 *Strict* — whitelist everywhere",
        private = "🟡 *Private only* — whitelist for PM",
        group   = "🟠 *Group only* — whitelist for groups",
    }
    local users = get_allowed_users_list()
    local message = string.format(
        "🛡️ *Whitelist System*\n\n"
        .. "*Mode:* %s\n"
        .. "*Users:* %d\n\n"
        .. "*Commands:*\n"
        .. "• `/whitelist mode none|all|private|group`\n"
        .. "• `/whitelist add USER_ID`\n"
        .. "• `/whitelist remove USER_ID`",
        mode_desc[WHITELIST_MODE] or WHITELIST_MODE, #users
    )
    if #users > 0 then
        message = message .. "\n\n*Allowed:*\n"
        for i, uid in ipairs(users) do
            message = message .. string.format("%d. `%s`\n", i, uid)
            if #message > 3500 then break end
        end
    end
    send_message(msg.chat.id, message, msg.message_id)
end

local function handle_whitelist_mode(msg, text)
    if not is_admin(msg.from.id) then
        send_message(msg.chat.id, "⛔ *Admin only*", msg.message_id)
        return
    end
    local mode = text:match("/whitelist mode (%w+)")
    if not mode or not ({ none=1, all=1, private=1, group=1 })[mode] then
        send_message(msg.chat.id, "❌ *Usage:* `/whitelist mode {none|all|private|group}`", msg.message_id)
        return
    end
    os.execute("uci set deepbot.main.whitelist_mode='" .. mode .. "'")
    os.execute("uci commit deepbot")
    WHITELIST_MODE = mode
    local msgs = {
        none    = "⚪ Whitelist *disabled*",
        all     = "🔴 Whitelist: *ALL* (all chats)",
        private = "🟡 Whitelist: *PRIVATE* (PM only)",
        group   = "🟠 Whitelist: *GROUP* (groups only)",
    }
    send_message(msg.chat.id, msgs[mode], msg.message_id)
    info("Whitelist mode → " .. mode)
end

local function handle_whitelist_add(msg, text)
    if not is_admin(msg.from.id) then
        send_message(msg.chat.id, "⛔ *Admin only*", msg.message_id)
        return
    end
    local new_user = text:match("/whitelist add (%d+)")
    if not new_user then
        send_message(msg.chat.id, "❌ *Usage:* `/whitelist add USER_ID`", msg.message_id)
        return
    end
    add_allowed_user(tonumber(new_user), msg.from.id)
    send_message(msg.chat.id, "✅ User `" .. new_user .. "` added to whitelist", msg.message_id)
    info("Whitelist add: " .. new_user)
end

local function handle_whitelist_remove(msg, text)
    if not is_admin(msg.from.id) then
        send_message(msg.chat.id, "⛔ *Admin only*", msg.message_id)
        return
    end
    local rm_user = text:match("/whitelist remove (%d+)")
    if not rm_user then
        send_message(msg.chat.id, "❌ *Usage:* `/whitelist remove USER_ID`", msg.message_id)
        return
    end
    remove_allowed_user(tonumber(rm_user))
    send_message(msg.chat.id, "🗑️ User `" .. rm_user .. "` removed from whitelist", msg.message_id)
    info("Whitelist remove: " .. rm_user)
end

-- ===== HANDLE TEXT =====
local function handle_text(msg, clean_text)
    local user_id   = msg.from.id
    local chat_id   = msg.chat.id
    local chat_type = msg.chat.type

    if not is_user_allowed(user_id, chat_type) then
        info("Access denied user=" .. user_id)
        local denial = {
            all     = "⛔ *Access denied!* You are not authorized.",
            private = "⛔ *Access denied!* You are not in the whitelist.\nUse `/whoami` to get your ID.",
            group   = "⛔ *Access denied!*",
        }
        send_message(chat_id, denial[WHITELIST_MODE] or "⛔ *Access denied!*", msg.message_id)
        return
    end

    local text = normalize_text(clean_text or msg.text or "")
    if text == "" then return end
    info(string.format("handle_text user=%d chat=%d text=%q",
        user_id, chat_id, text:sub(1, 80)))

    local rate_ok, user_rate = check_rate(user_id)
    if not rate_ok then
        send_message(chat_id,
            string.format("⏳ *Too many requests!* Limit: %d/min. Please wait.", user_rate),
            msg.message_id)
        return
    end

    tg_call("sendChatAction", { chat_id = chat_id, action = "typing" }, true)

    local data, api_err = call_deepseek(user_id, text)
    if api_err then
        err("Error user=" .. user_id .. ": " .. api_err)
        send_message(chat_id, "❌ *Error:* " .. api_err, msg.message_id)
        return
    end

    local answer = data.choices[1].message.content
    send_message(chat_id, answer, msg.message_id)
end

-- ===== DISPATCH =====
local function dispatch(msg)
    if not msg then dbg("dispatch: nil") return end
    if not msg.text then dbg("dispatch: no text") return end
    dbg(string.format("dispatch: type=%s text=%q",
        tostring(msg.chat and msg.chat.type), msg.text:sub(1, 80)))

    local text = msg.text

    if     text == "/start"   or text:find("^/start@")   then handle_start(msg)
    elseif text == "/clear"   or text:find("^/clear@")   then handle_clear(msg)
    elseif text == "/history" or text:find("^/history@") then handle_history(msg)
    elseif text == "/tokens"  or text:find("^/tokens@")  then handle_tokens(msg)
    elseif text == "/whoami"  or text:find("^/whoami@")  then handle_whoami(msg)
    elseif text == "/admin"   or text:find("^/admin@")   then handle_admin_info(msg)
    elseif text == "/allowed" or text:find("^/allowed@") then handle_allowed(msg)
    elseif text == "/whitelist" or text:find("^/whitelist@") then handle_whitelist_status(msg)
    elseif text:find("^/whitelist mode ")   then handle_whitelist_mode(msg, text)
    elseif text:find("^/whitelist add ")    then handle_whitelist_add(msg, text)
    elseif text:find("^/whitelist remove ") then handle_whitelist_remove(msg, text)
    elseif text == "/limit" or text:find("^/limit ") then handle_limit(msg, text)
    elseif text:sub(1,1) ~= "/" then
        local should_reply, clean_text = group_filter(msg)
        if should_reply then handle_text(msg, clean_text) end
    end
end

-- ===== LONG POLLING =====
local function run()
    info("Lua version: " .. _VERSION)
    info("Token configured: "  .. (TELEGRAM_BOT_TOKEN ~= "YOUR_BOT_TOKEN"   and "YES" or "NO"))
    info("AI key configured: " .. (DEEPSEEK_API_KEY   ~= "YOUR_DEEPSEEK_KEY" and "YES" or "NO"))
    info("AI Model: "          .. AI_MODEL)
    info("Debug mode: "        .. (DEBUG and "ON" or "OFF"))
    info("Proxy: "             .. (PROXY_URL ~= "" and PROXY_URL or "none"))
    info("Group mention only: " .. (GROUP_MENTION_ONLY and "ON" or "OFF"))
    info("Admin ID: "          .. (ADMIN_ID ~= "" and ADMIN_ID or "NOT SET"))
    info("Whitelist mode: "    .. WHITELIST_MODE)
    info("Global token limit: " .. TOKEN_LIMIT_PER_DAY .. "/day")
    info("Global rate limit:  " .. RATE_LIMIT .. " req/min")

    if WHITELIST_MODE ~= "none" and ADMIN_ID == "" then
        err("WARNING: whitelist_mode='" .. WHITELIST_MODE .. "' but admin_id not set!")
    end

    local test = io.popen("curl --version 2>&1 | head -1")
    if test then info("curl: " .. (test:read("*l") or "?")); test:close() end

    info("Checking token via getMe...")
    local me = tg_call("getMe", {}, true)
    if me and me.ok then
        BOT_USERNAME = me.result.username or ""
        info("Bot: @" .. BOT_USERNAME)
    else
        err("getMe failed — check TELEGRAM_BOT_TOKEN!")
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
        }, true)

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
            if not res then socket.sleep(5) end
        end
    end
end

run()
