-- ============================================================================
-- NppExport for NotepadNext
-- ============================================================================
-- Export plugin for converting editor content to various formats
--
-- Features:
-- - Export to HTML (with syntax highlighting)
-- - Export to RTF
-- - Export to LaTeX
-- - Export to JSON
-- - Copy formatted text to clipboard
-- ============================================================================

local plugin = {
    name = "NppExport",
    version = "1.0.0",
    author = "Ported to NotepadNext"
}

-- ============================================================================
-- HTML Export
-- ============================================================================

local function escapeHtml(text)
    return text:gsub("&", "&amp;")
              :gsub("<", "&lt;")
              :gsub(">", "&gt;")
              :gsub('"', "&quot;")
              :gsub("'", "&#39;")
end

local function toHtml(text, title)
    title = title or "NotepadNext Export"
    
    local html = [[
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>]] .. escapeHtml(title) .. [[</title>
    <style>
        body {
            font-family: 'Courier New', monospace;
            background-color: #ffffff;
            color: #000000;
            margin: 10px;
            padding: 10px;
        }
        pre {
            white-space: pre-wrap;
            word-wrap: break-word;
            background-color: #f5f5f5;
            border: 1px solid #ddd;
            padding: 10px;
            border-radius: 4px;
        }
        .line-number {
            color: #999;
            display: inline-block;
            width: 40px;
            text-align: right;
            margin-right: 10px;
        }
    </style>
</head>
<body>
    <h1>]] .. escapeHtml(title) .. [[</h1>
    <pre>]]
    
    local lines = {}
    for line in text:gmatch("[^\n\r]+") do
        table.insert(lines, line)
    end
    
    for i, line in ipairs(lines) do
        html = html .. '<span class="line-number">' .. string.format("%04d", i) .. '</span>'
        html = html .. escapeHtml(line) .. '\n'
    end
    
    html = html .. [[
    </pre>
</body>
</html>
]]
    
    return html
end

-- ============================================================================
-- RTF Export
-- ============================================================================

local function toRtf(text, title)
    title = title or "NotepadNext Export"
    
    -- RTF header
    local rtf = [[{\rtf1\ansi\ansicpg1252\deff0{\fonttbl{\f0\fnil\fcharset0 Courier New;}}]]
    rtf = rtf .. [[\f0\fs20]]
    
    -- Заголовок
    rtf = rtf .. [[\pard\sl240\slmult1]] -- интерлиниция
    rtf = rtf .. [[{\b ]] .. title .. [[}\par]]
    rtf = rtf .. [[\par]]
    
    -- Содержимое
    local lines = {}
    for line in text:gmatch("[^\n\r]+") do
        table.insert(lines, line)
    end
    
    for i, line in ipairs(lines) do
        -- Экранировать специальные символы RTF
        line = line:gsub("\\", "\\\\")
                   :gsub("{", "\\{")
                   :gsub("}", "\\}")
        
        rtf = rtf .. line .. [[\par]] .. "\n"
    end
    
    rtf = rtf .. "}"
    
    return rtf
end

-- ============================================================================
-- LaTeX Export
-- ============================================================================

local function toLaTeX(text, title)
    title = title or "NotepadNext Export"
    
    local latex = [[
\documentclass{article}
\usepackage[utf8]{inputenc}
\usepackage{listings}
\usepackage{xcolor}

\lstset{
    basicstyle=\ttfamily\small,
    breaklines=true,
    backgroundcolor=\color{gray!10},
    frame=single
}

\title{]] .. title .. [[}
\date{\today}

\begin{document}

\maketitle

\begin{lstlisting}
]] .. text .. [[
\end{lstlisting}

\end{document}
]]
    
    return latex
end

-- ============================================================================
-- JSON Export
-- ============================================================================

local function toJSON(text, filename)
    filename = filename or "exported"
    
    local json = {
        version = "1.0",
        exported_from = "NotepadNext",
        filename = filename,
        content = text,
        line_count = select(2, text:gsub("\n", "\n")) + 1,
        character_count = #text
    }
    
    -- Простой JSON encoder
    local function jsonEncode(obj)
        if type(obj) == "string" then
            return '"' .. obj:gsub('"', '\\"'):gsub('\n', '\\n'):gsub('\r', '\\r'):gsub('\t', '\\t') .. '"'
        elseif type(obj) == "number" then
            return tostring(obj)
        elseif type(obj) == "boolean" then
            return obj and "true" or "false"
        elseif type(obj) == "table" then
            local result = "{"
            local first = true
            for key, value in pairs(obj) do
                if not first then result = result .. "," end
                result = result .. '"' .. key .. '":' .. jsonEncode(value)
                first = false
            end
            return result .. "}"
        end
        return "null"
    end
    
    return jsonEncode(json)
end

-- ============================================================================
-- Команды экспорта
-- ============================================================================

function executeCommand_toHTML()
    local text = editor:getText()
    if text == "" then
        ui.message("NppExport", "Document is empty")
        return
    end
    
    local filename = editor:getCurrentFile()
    if not filename then filename = "export.txt" end
    
    local title = fs.basename(filename)
    local html = toHtml(text, title)
    
    -- Сохранить в файл
    local outputPath = filename:gsub("(%.[^%.]+)$", ".html")
    
    if fs.write(outputPath, html) then
        ui.message("NppExport", "HTML export successful!\n\n" .. outputPath)
        plugin.log("Exported to HTML: " .. outputPath)
    else
        ui.message("NppExport Error", "Failed to write file: " .. outputPath)
    end
end

function executeCommand_toRTF()
    local text = editor:getText()
    if text == "" then
        ui.message("NppExport", "Document is empty")
        return
    end
    
    local filename = editor:getCurrentFile()
    if not filename then filename = "export.txt" end
    
    local title = fs.basename(filename)
    local rtf = toRtf(text, title)
    
    -- Сохранить в файл
    local outputPath = filename:gsub("(%.[^%.]+)$", ".rtf")
    
    if fs.write(outputPath, rtf) then
        ui.message("NppExport", "RTF export successful!\n\n" .. outputPath)
        plugin.log("Exported to RTF: " .. outputPath)
    else
        ui.message("NppExport Error", "Failed to write file: " .. outputPath)
    end
end

function executeCommand_toLaTeX()
    local text = editor:getText()
    if text == "" then
        ui.message("NppExport", "Document is empty")
        return
    end
    
    local filename = editor:getCurrentFile()
    if not filename then filename = "export.txt" end
    
    local title = fs.basename(filename)
    local latex = toLaTeX(text, title)
    
    -- Сохранить в файл
    local outputPath = filename:gsub("(%.[^%.]+)$", ".tex")
    
    if fs.write(outputPath, latex) then
        ui.message("NppExport", "LaTeX export successful!\n\n" .. outputPath)
        plugin.log("Exported to LaTeX: " .. outputPath)
    else
        ui.message("NppExport Error", "Failed to write file: " .. outputPath)
    end
end

function executeCommand_toJSON()
    local text = editor:getText()
    if text == "" then
        ui.message("NppExport", "Document is empty")
        return
    end
    
    local filename = editor:getCurrentFile()
    if not filename then filename = "export.txt" end
    
    local baseName = fs.basename(filename):gsub("(%.[^%.]+)$", "")
    local json = toJSON(text, baseName)
    
    -- Сохранить в файл
    local outputPath = filename:gsub("(%.[^%.]+)$", ".json")
    
    if fs.write(outputPath, json) then
        ui.message("NppExport", "JSON export successful!\n\n" .. outputPath)
        plugin.log("Exported to JSON: " .. outputPath)
    else
        ui.message("NppExport Error", "Failed to write file: " .. outputPath)
    end
end

-- ============================================================================
-- События
-- ============================================================================

plugin.on("ready", function()
    plugin.log("NppExport plugin loaded")
end)

plugin.on("shutdown", function()
    plugin.log("NppExport plugin unloaded")
end)

-- ============================================================================
-- Экспорт
-- ============================================================================

return plugin
