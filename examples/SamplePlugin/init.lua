-- ============================================================================
-- ExamplePlugin для NotepadNext
-- ============================================================================
-- 
-- Это базовый шаблон плагина, адаптированный из системы плагинов Notepad++.
-- Демонстрирует основные возможности Plugin API.
--
-- Структура:
-- 1. Инициализация плагина
-- 2. Регистрация команд меню
-- 3. Обработка событий
-- 4. Работа с конфигурацией
-- 5. Взаимодействие с UI и редактором
--
-- ============================================================================

local plugin = {
    name = "ExamplePlugin",
    version = "1.0.0",
    author = "Your Name"
}

-- ============================================================================
-- ЧАСТЬ 1: Инициализация и конфигурация
-- ============================================================================

-- Загрузить конфигурацию плагина из хранилища (аналог Notepad++ getConfig)
local config = plugin.getConfig()

-- Установить значения по умолчанию
if config.greeting == nil then
    config.greeting = "Hello, NotepadNext!"
    plugin.saveConfig(config)
end

-- ============================================================================
-- ЧАСТЬ 2: Регистрация команд (аналог FuncItem[] в Notepad++ DLL)
-- ============================================================================

-- Команда 1: Простое диалоговое окно
plugin.registerCommand({
    id = "example.hello",
    title = "Say Hello",
    execute = function()
        -- Вывести диалог с приветствием (аналог MessageBox)
        ui.message("Greeting", config.greeting)
    end
})

-- Команда 2: Трансформация текста
plugin.registerCommand({
    id = "example.transform",
    title = "Transform Selection",
    execute = function()
        local selected = editor:getSelectedText()
        if selected == "" then
            ui.message("Info", "Please select some text first")
            return
        end
        
        -- Выбрать трансформацию
        local options = {"UPPERCASE", "lowercase", "Capitalize", "Reverse"}
        local choice = ui.select("Choose transformation", options)
        
        local result = selected
        if choice == "UPPERCASE" then
            result = selected:upper()
        elseif choice == "lowercase" then
            result = selected:lower()
        elseif choice == "Capitalize" then
            result = selected:sub(1, 1):upper() .. selected:sub(2):lower()
        elseif choice == "Reverse" then
            result = string.reverse(selected)
        end
        
        editor:replaceSelection(result)
    end
})

-- Команда 3: Работа с файловой системой
plugin.registerCommand({
    id = "example.saveLog",
    title = "Save Plugin Log",
    execute = function()
        local logPath = plugin.getRootPath() .. "/debug.log"
        local logContent = "Plugin debug log\n"
        logContent = logContent .. "Greeting: " .. config.greeting .. "\n"
        logContent = logContent .. "Timestamp: " .. os.date("%Y-%m-%d %H:%M:%S") .. "\n"
        
        if fs.write(logPath, logContent) then
            ui.message("Success", "Log saved to:\n" .. logPath)
        else
            ui.message("Error", "Failed to write log")
        end
    end
})

-- ============================================================================
-- ЧАСТЬ 3: Обработка событий (аналог beNotified() и NPPN_* в Notepad++)
-- ============================================================================

-- Событие: Готовность приложения (аналог NPPN_READY)
plugin.on("ready", function()
    print("[ExamplePlugin] Notepad Next is ready!")
    
    -- Инициализировать UI элементы
    if plugin.isFirstRun() then
        ui.message("Welcome", "ExamplePlugin loaded successfully!")
    end
end)

-- Событие: Перед открытием файла (аналог NPPN_FILEBEFORELOAD)
plugin.on("beforeFileOpen", function(filename)
    print("[ExamplePlugin] Opening file: " .. filename)
end)

-- Событие: После открытия файла (аналог NPPN_FILEAFTERLOAD)
plugin.on("afterFileOpen", function(filename)
    print("[ExamplePlugin] File opened: " .. filename)
    
    -- Пример: определить тип файла и задать лексер
    if filename:match("%.py$") then
        editor:setLexer("python")
    elseif filename:match("%.cpp$") or filename:match("%.h$") then
        editor:setLexer("cpp")
    end
end)

-- Событие: Перед сохранением файла (аналог NPPN_FILEBEFORESAVE)
plugin.on("beforeFileSave", function(filename)
    print("[ExamplePlugin] Saving file: " .. filename)
    
    -- Пример: удалить trailing whitespace перед сохранением
    removeTrailingWhitespace()
end)

-- Событие: После сохранения файла (аналог NPPN_FILEAFTERSAVE)
plugin.on("afterFileSave", function(filename)
    print("[ExamplePlugin] File saved: " .. filename)
end)

-- Событие: Перед закрытием файла (аналог NPPN_FILEBEFORECLOSE)
plugin.on("beforeFileClose", function(filename)
    print("[ExamplePlugin] Closing file: " .. filename)
end)

-- Событие: После закрытия файла (аналог NPPN_FILEAFTERCLOSE)
plugin.on("afterFileClose", function(filename)
    print("[ExamplePlugin] File closed: " .. filename)
end)

-- Событие: Завершение приложения (аналог NPPN_SHUTDOWN)
plugin.on("shutdown", function()
    print("[ExamplePlugin] Notepad Next is shutting down")
    -- Сохранить конфигурацию, очистить ресурсы
    plugin.saveConfig(config)
end)

-- ============================================================================
-- ЧАСТЬ 4: Вспомогательные функции
-- ============================================================================

-- Удалить пробелы в конце строк
function removeTrailingWhitespace()
    local text = editor:getText()
    local lines = string.split(text, "\n")
    
    for i, line in ipairs(lines) do
        lines[i] = line:gsub("%s+$", "")
    end
    
    editor:setText(table.concat(lines, "\n"))
end

-- Вспомогательная функция: строка.split
if not string.split then
    function string.split(str, sep)
        local result = {}
        for part in str:gmatch("[^" .. sep .. "]+") do
            table.insert(result, part)
        end
        return result
    end
end

-- ============================================================================
-- ЧАСТЬ 5: Plugin-to-Plugin коммуникация (аналог NPPM_MSGTOPLUGIN)
-- ============================================================================

-- Зарегистрировать публичную функцию, которую могут вызвать другие плагины
plugin.registerPublicFunction("greet", function(name)
    return "Hello, " .. (name or "Unknown") .. "!"
end)

-- Вызвать функцию из другого плагина
local otherResult = plugin.call("OtherPlugin", "someFunction", {"arg1", "arg2"})
if otherResult then
    print("Other plugin returned: " .. otherResult)
end

-- ============================================================================
-- ЧАСТЬ 6: Расширенная конфигурация
-- ============================================================================

-- Получить конфигурацию с типизацией
function getConfigValue(key, default, vtype)
    local value = config[key]
    if value == nil then
        return default
    end
    
    if vtype == "boolean" then
        return value == true or value == "true"
    elseif vtype == "number" then
        return tonumber(value) or default
    else
        return tostring(value)
    end
end

-- Установить конфигурацию с типизацией
function setConfigValue(key, value)
    config[key] = value
    plugin.saveConfig(config)
end

-- ============================================================================
-- ЧАСТЬ 7: Дебаг функции (удалить в продакшене)
-- ============================================================================

if getConfigValue("debug", false, "boolean") then
    plugin.registerCommand({
        id = "example.debug",
        title = "[DEBUG] Show Info",
        execute = function()
            local info = "Plugin Info:\n"
            info = info .. "Name: " .. plugin.name .. "\n"
            info = info .. "Version: " .. plugin.version .. "\n"
            info = info .. "Root: " .. plugin.getRootPath() .. "\n"
            info = info .. "Config: " .. table.tostring(config) .. "\n"
            ui.message("Debug Info", info)
        end
    })
end

-- ============================================================================
-- ЭКСПОРТ
-- ============================================================================

return plugin
