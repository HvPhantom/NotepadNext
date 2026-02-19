-- ============================================================================
-- MIME Tools for NotepadNext
-- ============================================================================
-- MIME encoding/decoding and email utilities
--
-- Features:
-- - Quoted-Printable encoding/decoding
-- - Uuencode encoding/decoding
-- - MIME type detection
-- - Email header decoding (RFC 2047)
-- ============================================================================

local plugin = {
    name = "MIMETools",
    version = "1.0.0",
    author = "Ported to NotepadNext"
}

-- ============================================================================
-- Quoted-Printable Encoding/Decoding
-- ============================================================================

local function quotedPrintableEncode(text)
    local result = {}
    local lineLength = 0
    
    for i = 1, #text do
        local char = text:sub(i, i)
        local byte = string.byte(char)
        
        -- Символы которые не нужно кодировать
        if (byte >= 33 and byte <= 60) or 
           (byte >= 62 and byte <= 126) or 
           byte == 9 then -- tab
            table.insert(result, char)
            lineLength = lineLength + 1
        else
            -- Кодировать как =HH
            local encoded = string.format("=%02X", byte)
            table.insert(result, encoded)
            lineLength = lineLength + 3
        end
        
        -- Мягкий разрыв строки при длине > 76
        if lineLength > 76 then
            table.insert(result, "=\n")
            lineLength = 0
        end
    end
    
    return table.concat(result)
end

local function quotedPrintableDecode(text)
    local result = {}
    local i = 1
    
    while i <= #text do
        if text:sub(i, i) == "=" then
            if text:sub(i+1, i+2) == "\r\n" or text:sub(i+1, i+1) == "\n" then
                -- Мягкий разрыв строки, пропустить
                i = i + (text:sub(i+1, i+2) == "\r\n" and 3 or 2)
            else
                -- Hex-кодированный символ
                local hex = text:sub(i+1, i+2)
                local byte = tonumber(hex, 16)
                if byte then
                    table.insert(result, string.char(byte))
                    i = i + 3
                else
                    table.insert(result, "=")
                    i = i + 1
                end
            end
        else
            table.insert(result, text:sub(i, i))
            i = i + 1
        end
    end
    
    return table.concat(result)
end

-- ============================================================================
-- Uuencode Encoding/Decoding
-- ============================================================================

local uuencodeAlphabet = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"

local function uuencode(text)
    local result = {}
    
    for i = 1, #text, 3 do
        local b1 = string.byte(text, i) or 0
        local b2 = string.byte(text, i+1) or 0
        local b3 = string.byte(text, i+2) or 0
        
        local n = (b1 * 65536) + (b2 * 256) + b3
        
        local c1 = math.floor(n / 262144) + 32
        local c2 = math.floor((n % 262144) / 4096) + 32
        local c3 = math.floor((n % 4096) / 64) + 32
        local c4 = (n % 64) + 32
        
        table.insert(result, string.format("%c%c%c%c", c1, c2, c3, c4))
    end
    
    return table.concat(result)
end

local function uudecode(text)
    local result = {}
    local cleanText = text:gsub("[^!-_]+", "") -- удалить всё кроме uuencode символов
    
    for i = 1, #cleanText, 4 do
        local c1 = string.byte(cleanText, i) or 33
        local c2 = string.byte(cleanText, i+1) or 33
        local c3 = string.byte(cleanText, i+2) or 33
        local c4 = string.byte(cleanText, i+3) or 33
        
        local b1 = ((c1 - 32) * 64) + ((c2 - 32) / 4)
        local b2 = (((c2 - 32) % 4) * 64) + ((c3 - 32) / 16)
        local b3 = (((c3 - 32) % 16) * 4) + ((c4 - 32) / 256)
        
        if b1 >= 0 and b1 < 256 then table.insert(result, string.char(b1)) end
        if b2 >= 0 and b2 < 256 then table.insert(result, string.char(b2)) end
        if b3 >= 0 and b3 < 256 then table.insert(result, string.char(b3)) end
    end
    
    return table.concat(result)
end

-- ============================================================================
-- MIME Type Detection
-- ============================================================================

local mimeTypes = {
    -- Текстовые форматы
    txt = "text/plain",
    html = "text/html",
    htm = "text/html",
    xml = "text/xml",
    json = "application/json",
    csv = "text/csv",
    
    -- Изображения
    jpg = "image/jpeg",
    jpeg = "image/jpeg",
    png = "image/png",
    gif = "image/gif",
    bmp = "image/bmp",
    svg = "image/svg+xml",
    
    -- Документы
    pdf = "application/pdf",
    doc = "application/msword",
    docx = "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    xls = "application/vnd.ms-excel",
    xlsx = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    
    -- Архивы
    zip = "application/zip",
    rar = "application/x-rar-compressed",
    gz = "application/gzip",
    tar = "application/x-tar",
    
    -- Audio
    mp3 = "audio/mpeg",
    wav = "audio/wav",
    ogg = "audio/ogg",
    
    -- Video
    mp4 = "video/mp4",
    avi = "video/x-msvideo",
    mkv = "video/x-matroska",
    mov = "video/quicktime",
}

local function getMIMEType(filename)
    local ext = filename:match("%.([^%.]+)$")
    if not ext then return "application/octet-stream" end
    
    ext = ext:lower()
    return mimeTypes[ext] or "application/octet-stream"
end

-- ============================================================================
-- RFC 2047 Email Header Decoding
-- ============================================================================

local function decodeEmailHeader(header)
    -- Паттерн: =?charset?encoding?encoded-text?=
    -- Пример: =?UTF-8?B?SGVsbG8gV29ybGQ=?=
    
    local result = header
    
    -- Найти все закодированные части
    result = result:gsub("=?([^?]+)?([BbQq])?([^?]+)?=", function(charset, encoding, encoded)
        if charset and encoding and encoded then
            charset = charset:upper()
            encoding = encoding:upper()
            
            local decoded
            if encoding == "B" then
                -- Base64
                decoded = base64Decode and base64Decode(encoded) or encoded
            elseif encoding == "Q" then
                -- Quoted-Printable with slight modifications for headers
                decoded = quotedPrintableDecode(encoded:gsub("_", " "))
            else
                decoded = encoded
            end
            
            return decoded
        end
        return header
    end)
    
    return result
end

-- ============================================================================
-- Команды
-- ============================================================================

function executeCommand_decodeQuotedPrintable()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local decoded = quotedPrintableDecode(selected)
    editor:replaceSelection(decoded)
    plugin.log("Quoted-Printable decoded")
end

function executeCommand_encodeQuotedPrintable()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local encoded = quotedPrintableEncode(selected)
    editor:replaceSelection(encoded)
    plugin.log("Quoted-Printable encoded")
end

function executeCommand_decodeUuencode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local decoded = uudecode(selected)
    editor:replaceSelection(decoded)
    plugin.log("Uuencode decoded")
end

function executeCommand_encodeUuencode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local encoded = uuencode(selected)
    editor:replaceSelection(encoded)
    plugin.log("Uuencode encoded")
end

function executeCommand_getMIMEType()
    local filename = editor:getCurrentFile()
    if not filename then
        ui.message("MIME Tools", "No file is currently open")
        return
    end
    
    local mimeType = getMIMEType(filename)
    ui.message("MIME Type", "File: " .. fs.basename(filename) .. "\nMIME Type: " .. mimeType)
    plugin.log("MIME Type for " .. filename .. " is " .. mimeType)
end

function executeCommand_decodeEmailHeaders()
    local selected = editor:getSelectedText()
    if selected == "" then
        ui.message("MIME Tools", "Please select email headers to decode")
        return
    end
    
    local decoded = decodeEmailHeader(selected)
    editor:replaceSelection(decoded)
    plugin.log("Email headers decoded")
end

-- ============================================================================
-- События
-- ============================================================================

plugin.on("ready", function()
    plugin.log("MIME Tools plugin loaded")
end)

plugin.on("shutdown", function()
    plugin.log("MIME Tools plugin unloaded")
end)

-- ============================================================================
-- Экспорт
-- ============================================================================

return plugin
