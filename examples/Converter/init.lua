-- ============================================================================
-- Converter for NotepadNext
-- ============================================================================
-- Text and encoding converter plugin
--
-- Features:
-- - Base64 encode/decode
-- - Hex encode/decode
-- - URL encode/decode
-- - Case conversions (UPPER, lower, Swap, camelCase)
-- ============================================================================

local plugin = {
    name = "Converter",
    version = "1.0.0",
    author = "Ported to NotepadNext"
}

-- ============================================================================
-- Base64 Encoding/Decoding
-- ============================================================================

local base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

local function base64Encode(data)
    local result = {}
    
    for i = 1, #data, 3 do
        local a, b, c = string.byte(data, i), string.byte(data, i+1), string.byte(data, i+2)
        a = a or 0
        b = b or 0
        c = c or 0
        
        local bits = (a * 65536) + (b * 256) + c
        
        table.insert(result, base64_chars:sub(bit.band(bits / 262144, 63) + 1, bit.band(bits / 262144, 63) + 1))
        table.insert(result, base64_chars:sub(bit.band(bits / 4096, 63) + 1, bit.band(bits / 4096, 63) + 1))
        
        if i + 1 <= #data then
            table.insert(result, base64_chars:sub(bit.band(bits / 64, 63) + 1, bit.band(bits / 64, 63) + 1))
        else
            table.insert(result, "=")
        end
        
        if i + 2 <= #data then
            table.insert(result, base64_chars:sub(bit.band(bits, 63) + 1, bit.band(bits, 63) + 1))
        else
            table.insert(result, "=")
        end
    end
    
    return table.concat(result)
end

local function base64Decode(data)
    local result = {}
    local bin = {}
    
    -- Создать reverse lookup
    local lookup = {}
    for i = 1, #base64_chars do
        lookup[base64_chars:sub(i, i)] = i - 1
    end
    
    for i = 1, #data, 4 do
        local c1 = lookup[data:sub(i, i)] or 0
        local c2 = lookup[data:sub(i+1, i+1)] or 0
        local c3 = lookup[data:sub(i+2, i+2)] or 0
        local c4 = lookup[data:sub(i+3, i+3)] or 0
        
        local bits = (c1 * 262144) + (c2 * 4096) + (c3 * 64) + c4
        
        table.insert(result, string.char(bit.band(bits / 65536, 255)))
        if data:sub(i+2, i+2) ~= "=" then
            table.insert(result, string.char(bit.band(bits / 256, 255)))
        end
        if data:sub(i+3, i+3) ~= "=" then
            table.insert(result, string.char(bit.band(bits, 255)))
        end
    end
    
    return table.concat(result)
end

-- ============================================================================
-- Hex Encoding/Decoding
-- ============================================================================

local function hexEncode(data)
    local result = {}
    for i = 1, #data do
        table.insert(result, string.format("%02X", string.byte(data, i)))
    end
    return table.concat(result, " ")
end

local function hexDecode(data)
    local result = {}
    local hex = data:gsub("[^0-9A-Fa-f]", "")
    
    for i = 1, #hex, 2 do
        local byte = tonumber(hex:sub(i, i+1), 16)
        if byte then
            table.insert(result, string.char(byte))
        end
    end
    
    return table.concat(result)
end

-- ============================================================================
-- URL Encoding/Decoding
-- ============================================================================

local function urlEncode(data)
    return data:gsub("([^%w])", function(char)
        return string.format("%%%02X", string.byte(char))
    end)
end

local function urlDecode(data)
    return data:gsub("%%([0-9A-Fa-f][0-9A-Fa-f])", function(hex)
        return string.char(tonumber(hex, 16))
    end)
end

-- ============================================================================
-- Case Conversions
-- ============================================================================

local function toUppercase(text)
    return text:upper()
end

local function toLowercase(text)
    return text:lower()
end

local function swapCase(text)
    local result = {}
    for i = 1, #text do
        local char = text:sub(i, i)
        if char:match("[a-z]") then
            table.insert(result, char:upper())
        elseif char:match("[A-Z]") then
            table.insert(result, char:lower())
        else
            table.insert(result, char)
        end
    end
    return table.concat(result)
end

local function toCamelCase(text)
    -- "hello world test" -> "helloWorldTest"
    local words = {}
    for word in text:gmatch("%w+") do
        table.insert(words, word)
    end
    
    if #words == 0 then return text end
    
    local result = {words[1]:lower()}
    for i = 2, #words do
        local word = words[i]
        table.insert(result, word:sub(1, 1):upper() .. word:sub(2):lower())
    end
    
    return table.concat(result)
end

-- ============================================================================
-- Команды
-- ============================================================================

function executeCommand_base64Encode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local encoded = base64Encode(selected)
    editor:replaceSelection(encoded)
    plugin.log("Base64 encoded")
end

function executeCommand_base64Decode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local decoded = base64Decode(selected)
    editor:replaceSelection(decoded)
    plugin.log("Base64 decoded")
end

function executeCommand_hexEncode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local encoded = hexEncode(selected)
    editor:replaceSelection(encoded)
    plugin.log("Hex encoded")
end

function executeCommand_hexDecode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local decoded = hexDecode(selected)
    editor:replaceSelection(decoded)
    plugin.log("Hex decoded")
end

function executeCommand_urlEncode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local encoded = urlEncode(selected)
    editor:replaceSelection(encoded)
    plugin.log("URL encoded")
end

function executeCommand_urlDecode()
    local selected = editor:getSelectedText()
    if selected == "" then
        selected = editor:getText()
    end
    
    local decoded = urlDecode(selected)
    editor:replaceSelection(decoded)
    plugin.log("URL decoded")
end

function executeCommand_toUppercase()
    local selected = editor:getSelectedText()
    if selected ~= "" then
        editor:replaceSelection(toUppercase(selected))
        plugin.log("Converted to UPPERCASE")
    end
end

function executeCommand_toLowercase()
    local selected = editor:getSelectedText()
    if selected ~= "" then
        editor:replaceSelection(toLowercase(selected))
        plugin.log("Converted to lowercase")
    end
end

function executeCommand_swapCase()
    local selected = editor:getSelectedText()
    if selected ~= "" then
        editor:replaceSelection(swapCase(selected))
        plugin.log("Swapped case")
    end
end

function executeCommand_toCamelCase()
    local selected = editor:getSelectedText()
    if selected ~= "" then
        editor:replaceSelection(toCamelCase(selected))
        plugin.log("Converted to camelCase")
    end
end

-- ============================================================================
-- События
-- ============================================================================

plugin.on("ready", function()
    plugin.log("Converter plugin loaded")
end)

plugin.on("shutdown", function()
    plugin.log("Converter plugin unloaded")
end)

-- ============================================================================
-- Экспорт
-- ============================================================================

return plugin
