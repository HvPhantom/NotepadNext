-- ============================================================================
-- ComparePlus for NotepadNext
-- ============================================================================
-- Advanced file comparison plugin with diff highlighting and line matching
--
-- Features:
-- - Compare two files side by side
-- - Compare selected text
-- - Highlight differences
-- - Line matching and navigation
-- ============================================================================

local plugin = {
    name = "ComparePlus",
    version = "1.0.0",
    author = "Ported to NotepadNext"
}

-- Конфигурация
local config = {
    firstFile = nil,
    secondFile = nil,
    highlightColor = 0xFF6644,  -- оранжевый
    matchColor = 0xFFFF99,      -- жёлтый
}

-- ============================================================================
-- Утилиты для сравнения текста
-- ============================================================================

local function splitLines(text)
    local lines = {}
    for line in text:gmatch("[^\n\r]+") do
        table.insert(lines, line)
    end
    return lines
end

local function calculateHash(line)
    -- Простой хеш строки для быстрого сравнения
    local hash = 0
    for i = 1, #line do
        hash = (hash * 31 + string.byte(line, i)) % 0xFFFFFFFF
    end
    return hash
end

local function findMatches(lines1, lines2)
    -- Найти совпадающие строки между двумя файлами
    local matches = {}
    local hashes1 = {}
    local hashes2 = {}
    
    -- Создать таблицы хешей
    for i, line in ipairs(lines1) do
        hashes1[calculateHash(line)] = i
    end
    
    for i, line in ipairs(lines2) do
        hashes2[calculateHash(line)] = i
    end
    
    -- Найти пересечения
    for hash1, line1 in pairs(hashes1) do
        if hashes2[hash1] then
            local line2 = hashes2[hash1]
            if lines1[line1] == lines2[line2] then
                table.insert(matches, {line1 = line1, line2 = line2})
            end
        end
    end
    
    table.sort(matches, function(a, b) return a.line1 < b.line1 end)
    return matches
end

local function diffLines(lines1, lines2)
    -- Простой diff алгоритм (можно улучшить до Levenshtein или LCS)
    local diffs = {
        added = {},      -- строки только в lines2
        removed = {},    -- строки только в lines1
        modified = {},   -- строки которые отличаются
    }
    
    local hashes1 = {}
    for i, line in ipairs(lines1) do
        hashes1[calculateHash(line)] = i
    end
    
    -- Найти удалённые и изменённые строки
    for i, line in ipairs(lines1) do
        local hash = calculateHash(line)
        if not hashes1[hash] then
            table.insert(diffs.removed, i)
        end
    end
    
    -- Найти добавленные строки
    for i, line in ipairs(lines2) do
        local hash = calculateHash(line)
        if not hashes1[hash] then
            table.insert(diffs.added, i)
        end
    end
    
    return diffs
end

-- ============================================================================
-- Команда: выбрать первый файл
-- ============================================================================

function executeCommand_selectFirst()
    local filename = editor:getCurrentFile()
    if not filename or filename == "" then
        ui.message("Compare", "Please open a file first")
        return
    end
    
    config.firstFile = filename
    plugin.log("First file selected: " .. filename)
    ui.message("ComparePlus", "First file selected:\n" .. filename)
end

-- ============================================================================
-- Команда: выбрать второй файл
-- ============================================================================

function executeCommand_selectSecond()
    local filename = editor:getCurrentFile()
    if not filename or filename == "" then
        ui.message("Compare", "Please open a file first")
        return
    end
    
    config.secondFile = filename
    plugin.log("Second file selected: " .. filename)
    ui.message("ComparePlus", "Second file selected:\n" .. filename)
end

-- ============================================================================
-- Команда: сравнить два файла
-- ============================================================================

function executeCommand_compare()
    if not config.firstFile or not config.secondFile then
        ui.message("ComparePlus", "Please select two files first:\n" ..
                                  "Compare > Select First File\n" ..
                                  "Compare > Select Second File")
        return
    end
    
    if config.firstFile == config.secondFile then
        ui.message("ComparePlus", "Cannot compare a file with itself")
        return
    end
    
    -- Читать содержимое файлов
    local content1 = fs.read(config.firstFile)
    local content2 = fs.read(config.secondFile)
    
    if not content1 or not content2 then
        ui.message("ComparePlus Error", "Failed to read one or both files")
        return
    end
    
    -- Разделить на строки
    local lines1 = splitLines(content1)
    local lines2 = splitLines(content2)
    
    -- Найти различия
    local diffs = diffLines(lines1, lines2)
    
    -- Показать результаты
    if #diffs.removed == 0 and #diffs.added == 0 and #diffs.modified == 0 then
        ui.message("ComparePlus", "Files are identical ✓")
    else
        local report = string.format("Comparison Results:\n\n" ..
                                   "Lines removed: %d\n" ..
                                   "Lines added: %d\n" ..
                                   "Lines modified: %d\n",
                                   #diffs.removed, #diffs.added, #diffs.modified)
        
        ui.message("ComparePlus", report)
    end
    
    -- TODO: Открыть окно с side-by-side сравнением
    plugin.log("Comparison complete: " .. config.firstFile .. " vs " .. config.secondFile)
end

-- ============================================================================
-- Команда: сравнить выделение
-- ============================================================================

function executeCommand_compareSelections()
    local selected = editor:getSelectedText()
    if selected == "" then
        ui.message("ComparePlus", "Please select text in both files and run this command")
        return
    end
    
    local result = ui.input("Enter text to compare with selection:", "")
    if not result or result == "" then
        return
    end
    
    local lines1 = splitLines(selected)
    local lines2 = splitLines(result)
    
    local diffs = diffLines(lines1, lines2)
    
    local identical = #diffs.removed == 0 and #diffs.added == 0
    if identical then
        ui.message("ComparePlus", "Selections are identical ✓")
    else
        ui.message("ComparePlus", "Selections differ:\n" ..
                                  "Lines in selection 1 only: " .. #diffs.removed .. "\n" ..
                                  "Lines in selection 2 only: " .. #diffs.added)
    end
end

-- ============================================================================
-- События
-- ============================================================================

plugin.on("ready", function()
    plugin.log("ComparePlus plugin loaded")
end)

plugin.on("shutdown", function()
    config.firstFile = nil
    config.secondFile = nil
    plugin.log("ComparePlus plugin unloaded")
end)

-- ============================================================================
-- Экспорт
-- ============================================================================

return plugin
