# Руководство по адаптации плагинов Notepad++ к NotepadNext

## Введение

Это руководство описывает, как портировать существующие плагины Notepad++ (написанные на C/C++) на кроссплатформенные плагины NotepadNext (написанные на Lua).

**Версия документа**: 1.0  
**Дата**: февраль 2026  
**Совместимость**: NotepadNext 0.1.0+

---

## Содержание

1. [Сравнение подходов](#сравнение-подходов)
2. [Пошаговое руководство адаптации](#пошаговое-руководство-адаптации)
3. [Примеры адаптации популярных плагинов](#примеры-адаптации-популярных-плагинов)
4. [Синтаксис и утилиты Lua для плагинов](#синтаксис-и-утилиты-lua-для-плагинов)
5. [Часто задаваемые вопросы](#часто-задаваемые-вопросы)

---

## Сравнение подходов

### Notepad++ (C/C++ DLL)

```c
// DLL плагин Notepad++
extern "C" __declspec(dllexport) void setInfo(NppData nppData) {
    // сохранить handles
    g_nppHandle = nppData._nppHandle;
    g_sciHandle = nppData._scintillaMainHandle;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
    if (notifyCode->nmhdr.code == NPPN_READY) {
        // инициализация
        MessageBox(g_nppHandle, L"Plugin loaded!", L"Info", MB_OK);
    }
}

// Команда в меню
void ShowHelloDialog() {
    MessageBox(g_nppHandle, L"Hello!", L"Greeting", MB_OK);
}

FuncItem funcItems[] = {
    {L"Show Dialog", ShowHelloDialog, 0, false, nullptr}
};
```

### NotepadNext (Lua)

```lua
-- init.lua плагина NotepadNext
local plugin = {}

plugin.on("ready", function()
    print("Plugin loaded!")
end)

plugin.registerCommand({
    id = "myplugin.hello",
    title = "Show Dialog",
    execute = function()
        ui.message("Greeting", "Hello!")
    end
})

return plugin
```

---

## Пошаговое руководство адаптации

### Шаг 1: Подготовка структуры проекта

```bash
mkdir -p MyPlugin/{src,docs,examples}
cd MyPlugin

# Основные файлы
touch manifest.json    # метаданные плагина
touch init.lua         # точка входа
touch README.md        # документация
```

Структура:
```
MyPlugin/
├── manifest.json          (метаданные)
├── init.lua              (основной код)
├── lib/                  (вспомогательные модули)
│   ├── utils.lua
│   └── config.lua
├── ui/                   (UI элементы)
│   └── dialogs.lua
├── examples/             (примеры использования)
└── docs/                 (документация)
```

### Шаг 2: Создать manifest.json

```json
{
  "name": "MyPlugin",
  "version": "1.0.0",
  "description": "Моё портированное расширение из Notepad++",
  "author": "Your Name",
  "license": "GPL-3.0",
  "homepage": "https://github.com/yourname/nnp-myplugin",
  "entry": "init.lua",
  
  "nnp-compatible-versions": "[0.1.0, 1.0.0]",
  
  "commands": [
    {
      "id": "myplugin.command1",
      "title": "Command 1",
      "category": "MyPlugin",
      "keybinding": "ctrl+shift+m"
    }
  ],
  
  "permissions": ["editor", "ui", "filesystem.read", "filesystem.write"]
}
```

### Шаг 3: Перевести инициализацию плагина

| Notepad++ | NotepadNext | Примечание |
|-----------|------------|-----------|
| `setInfo(NppData)` | `plugin:initialize()` | Вызывается автоматически |
| `getName()` | manifest.json `name` поле | Не нужна ручная регистрация |
| `getFuncsArray()` | `plugin.registerCommand()` | Для каждой команды |
| `isUnicode()` | (вмущено) | Lua работает с UTF-8 |
| `beNotified()` | `plugin.on(event, callback)` | Подписка на события |

**ДО** (Notepad++ C++):
```c
const wchar_t *getName() {
    return L"My Plugin";
}

FuncItem funcItems[] = {
    {L"Command A", funcA, 0, false, nullptr},
    {L"Command B", funcB, 0, false, nullptr},
};

int nbFunc = 2;

FuncItem *getFuncsArray(int *nbF) {
    *nbF = nbFunc;
    return funcItems;
}
```

**ПОСЛЕ** (NotepadNext Lua):
```lua
-- manifest.json
{
  "name": "My Plugin",
  "commands": [
    {"id": "myplugin.commandA", "title": "Command A"},
    {"id": "myplugin.commandB", "title": "Command B"}
  ]
}

-- init.lua
local plugin = {}

plugin.registerCommand({
    id = "myplugin.commandA",
    title = "Command A",
    execute = function()
        -- реализация
    end
})

plugin.registerCommand({
    id = "myplugin.commandB",
    title = "Command B",
    execute = function()
        -- реализация
    end
})

return plugin
```

### Шаг 4: Перевести события

| Notepad++ Event | NotepadNext Lua | 
|-----------------|-----------------|
| `NPPN_READY` | `plugin.on("ready")` |
| `NPPN_FILEBEFORELOAD` | `plugin.on("beforeFileOpen", filename)` |
| `NPPN_FILEAFTERLOAD` | `plugin.on("afterFileOpen", filename)` |
| `NPPN_FILEBEFORESAVE` | `plugin.on("beforeFileSave", filename)` |
| `NPPN_FILEAFTERSAVE` | `plugin.on("afterFileSave", filename)` |
| `NPPN_FILEBEFORECLOSE` | `plugin.on("beforeFileClose", filename)` |
| `NPPN_SHUTDOWN` | `plugin.on("shutdown")` |
| `SCN_*` scintilla | `plugin.on("editor.*")` | (планируется) |

**ДО** (Notepad++ C++):
```c
void beNotified(SCNotification *notifyCode) {
    switch(notifyCode->nmhdr.code) {
        case NPPN_READY:
            // инициализация
            break;
        case NPPN_FILEAFTERLOAD:
            // файл открыт
            handleFileOpen(notifyCode->nmhdr.hwndFrom);
            break;
        case NPPN_FILEAFTERSAVE:
            // файл сохранен
            handleFileSave();
            break;
        case NPPN_SHUTDOWN:
            // завершение
            cleanup();
            break;
    }
}
```

**ПОСЛЕ** (NotepadNext Lua):
```lua
plugin.on("ready", function()
    -- инициализация
end)

plugin.on("afterFileOpen", function(filename)
    handleFileOpen(filename)
end)

plugin.on("afterFileSave", function(filename)
    handleFileSave()
end)

plugin.on("shutdown", function()
    cleanup()
end)
```

### Шаг 5: Перевести работу с редактором

| Notepad++ (NPAPI/Scintilla) | NotepadNext Lua |
|-----|-----|
| `SendMessage(g_sciHandle, SCI_GETTEXT, ...)` | `editor:getText()` |
| `SendMessage(g_sciHandle, SCI_SETTEXT, ...)` | `editor:setText(text)` |
| `SendMessage(g_sciHandle, SCI_GETSELTEXT, ...)` | `editor:getSelectedText()` |
| `SendMessage(g_sciHandle, SCI_GETLENGHT, ...)` | `string.len(editor:getText())` |
| `SendMessage(g_sciHandle, SCI_INSERTTEXT, ...)` | `editor:insertText(text)` |
| `SendMessage(g_sciHandle, SCI_SETLEXER, SCLEX_CPP, 0)` | `editor:setLexer("cpp")` |

**ДО** (Notepad++ C++):
```c
// Получить текст
int length = SendMessage(g_sciHandle, SCI_GETLENGTH, 0, 0);
char *buffer = new char[length + 1];
SendMessage(g_sciHandle, SCI_GETTEXT, length + 1, (LPARAM)buffer);

// Установить текст
SendMessage(g_sciHandle, SCI_SETTEXT, 0, (LPARAM)"New text");

// Получить выделение
int selLength = SendMessage(g_sciHandle, SCI_GETSELTEXT, 0, 0);
char *sel = new char[selLength + 1];
SendMessage(g_sciHandle, SCI_GETSELTEXT, 0, (LPARAM)sel);
```

**ПОСЛЕ** (NotepadNext Lua):
```lua
-- Получить текст
local text = editor:getText()

-- Установить текст
editor:setText("New text")

-- Получить выделение
local selected = editor:getSelectedText()

-- Заменить выделение
editor:replaceSelection("replacement")

-- Установить лексер
editor:setLexer("cpp")
```

### Шаг 6: Перевести диалоги и UI

| Notepad++ | NotepadNext | Примечание |
|-----------|-----------|-----------|
| `MessageBox()` | `ui.message(title, text)` | Простой диалог |
| User dialog resource | `ui.dialog()` (планируется) | Qt-based диалоги |
| Toolbar | тулбар приложения | Через регистрацию команд |
| Docking window | `ui.dockBar(name, widget)` | Пристыковка диалогов |

**ДО** (Notepad++ C++):
```c
void ShowDialog() {
    MessageBox(g_nppHandle, 
               L"This is a message", 
               L"Title", 
               MB_OK | MB_ICONINFORMATION);
}

int result = MessageBox(g_nppHandle, 
                       L"Continue?", 
                       L"Confirm", 
                       MB_YESNO);
```

**ПОСЛЕ** (NotepadNext Lua):
```lua
function showDialog()
    ui.message("Title", "This is a message")
end

local result = ui.confirm("Title", "Continue?")
if result then
    -- пользователь нажал Да
end
```

### Шаг 7: Перевести работу с файлами

| Notepad++ | NotepadNext | 
|-----------|-----------|
| `CreateFileA()` | `fs.write()` |
| `ReadFile()` | `fs.read()` |
| `GetFullPathName()` | `fs.getAbsolutePath()` |

**ДО** (Notepad++ C++):
```c
HANDLE hFile = CreateFileA(
    filename, 
    GENERIC_READ, 
    FILE_SHARE_READ, 
    nullptr, 
    OPEN_EXISTING, 
    FILE_ATTRIBUTE_NORMAL, 
    nullptr);

// читать данные
DWORD bytesRead;
char buffer[4096];
ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, nullptr);
CloseHandle(hFile);
```

**ПОСЛЕ** (NotepadNext Lua):
```lua
-- Читать файл
local content = fs.read(filename)
if content then
    print("File content: " .. content)
end

-- Написать файл
if fs.write(filename, "data") then
    print("File saved")
end

-- Список файлов
local files = fs.listdir(dirname)
for _, file in ipairs(files) do
    print(file)
end
```

---

## Примеры адаптации популярных плагинов

### Пример 1: TextFX (трансформация текста)

**Функции TextFX**:
- Преобразовать в UPPERCASE
- Преобразовать в lowercase
- Сортировать строки
- Удалить duplikaty
- Уникальные строки

**Адаптация для NotepadNext**:

```lua
-- init.lua
local plugin = {}

local function transformToUppercase()
    local selected = editor:getSelectedText()
    if selected ~= "" then
        editor:replaceSelection(selected:upper())
    end
end

local function transformToLowercase()
    local selected = editor:getSelectedText()
    if selected ~= "" then
        editor:replaceSelection(selected:lower())
    end
end

local function sortLines()
    local selected = editor:getSelectedText()
    if selected == "" then return end
    
    local lines = {}
    for line in selected:gmatch("[^\n]+") do
        table.insert(lines, line)
    end
    
    table.sort(lines)
    editor:replaceSelection(table.concat(lines, "\n"))
end

local function removeLineEndings()
    local selected = editor:getSelectedText()
    if selected == "" then return end
    
    -- Удалить пробелы в конце каждой строки
    local result = selected:gsub("%s+$", "")
    editor:replaceSelection(result)
end

local function removeBlankLines()
    local text = editor:getSelectedText()
    if text == "" then
        text = editor:getText()
    end
    
    local lines = {}
    for line in text:gmatch("[^\n]+") do
        if line:match("%S") then
            table.insert(lines, line)
        end
    end
    
    editor:replaceSelection(table.concat(lines, "\n"))
end

-- Регистрация команд
plugin.registerCommand({
    id = "textfx.uppercase",
    title = "To UPPERCASE",
    category = "TextFX",
    execute = transformToUppercase
})

plugin.registerCommand({
    id = "textfx.lowercase",
    title = "To lowercase",
    category = "TextFX",
    execute = transformToLowercase
})

plugin.registerCommand({
    id = "textfx.sortlines",
    title = "Sort Lines",
    category = "TextFX",
    execute = sortLines
})

return plugin
```

### Пример 2: Compare (сравнение файлов)

```lua
-- init.lua
local plugin = {}

local comparePath1 = nil
local comparePath2 = nil

plugin.registerCommand({
    id = "compare.selectFirst",
    title = "Compare: Select First File",
    execute = function()
        local filename = editor:getCurrentFile()
        if filename then
            comparePath1 = filename
            ui.message("Compare", "First file selected: " .. filename)
        end
    end
})

plugin.registerCommand({
    id = "compare.selectSecond",
    title = "Compare: Select Second File",
    execute = function()
        local filename = editor:getCurrentFile()
        if filename then
            comparePath2 = filename
            ui.message("Compare", "Second file selected: " .. filename)
        end
    end
})

plugin.registerCommand({
    id = "compare.compare",
    title = "Compare Files",
    execute = function()
        if not comparePath1 or not comparePath2 then
            ui.message("Error", "Select two files first")
            return
        end
        
        local file1 = fs.read(comparePath1)
        local file2 = fs.read(comparePath2)
        
        if file1 == file2 then
            ui.message("Result", "Files are identical")
        else
            ui.message("Result", "Files are different")
            -- Показать различия...
        end
    end
})

return plugin
```

### Пример 3: HexViewer (просмотр в hex формате)

```lua
-- init.lua
local plugin = {}

local function bytesToHex(data)
    local hex = ""
    for i = 1, #data do
        hex = hex .. string.format("%02X ", string.byte(data, i))
    end
    return hex
end

local function displayHexView()
    local text = editor:getText()
    local truncated = text:sub(1, 256) -- первые 256 байт
    
    local hexView = ""
    local lineNum = 1
    
    for i = 1, #truncated, 16 do
        local chunk = truncated:sub(i, i + 15)
        hexView = hexView .. string.format("%08X: ", i - 1)
        hexView = hexView .. bytesToHex(chunk) .. "\n"
    end
    
    local hexWindow = ui.dialog("Hex Viewer")
    hexWindow:setText(hexView)
    hexWindow:show()
end

plugin.registerCommand({
    id = "hexviewer.show",
    title = "Show Hex View",
    execute = displayHexView
})

plugin.on("afterFileOpen", function(filename)
    print("File opened: " .. filename)
end)

return plugin
```

---

## Синтаксис и утилиты Lua для плагинов

### Встроенные Lua модули в плагинах

```lua
-- editor     - работа с текстом
-- ui         - диалоги и UI
-- fs         - файловая система
-- settings   - сохраняемые параметры
-- plugin     - API плагина
-- os         - системные функции
-- string     - работа со строками
-- table      - работа с таблицами
-- math       - математические функции
```

### Расширенные утилиты для строк

```lua
-- Разделить строку
local parts = string.split("one,two,three", ",")
-- parts = {"one", "two", "three"}

-- Соединить строки
local str = table.concat({"a", "b", "c"}, "-")
-- str = "a-b-c"

-- Trim пробелов
local trimmed = string.trim("  text  ")
-- trimmed = "text"

-- Replace
local result = string.replace("hello world", "world", "lua")
-- result = "hello lua"

-- Pattern matching (regex)
local matched = string.match("test123", "%d+")
-- matched = "123"
```

### Работа с JSON конфигурацией

```lua
local json = require("json")

-- Сохранить конфигурацию как JSON
local config = {
    option1 = "value1",
    option2 = 42,
    nested = { a = 1, b = 2 }
}

local configStr = json.encode(config)
fs.write("config.json", configStr)

-- Загрузить конфигурацию из JSON
local configStr = fs.read("config.json")
local config = json.decode(configStr)
print(config.option1)  -- "value1"
```

---

## Часто задаваемые вопросы

### В: Можно ли использовать C++ код в плагине NotepadNext?

О: Да, но это требует сложной компиляции. Рекомендуется использовать чистый Lua для кроссплатформенности. Если нужна высокая производительность, можно:
1. Написать критичный код на C++
2. Скомпилировать в Lua binding
3. Загрузить в плагин через `require()`

### В: Как отлаживать плагины?

О: Используйте:
```lua
print("Debug message")  -- выведется в консоль
editor:setText("Debug: " .. debug.traceback())  -- выведется в редактор
```

### В: Как сделать плагин cross-platform?

О: Используйте встроенные API:
- `fs.*` для файловых операций (автоматически переводит пути)
- `os.getenv()` для переменных окружения
- Избегайте Windows API (CreateFile, MessageBox и т.д.)

### В: Как взаимодействовать с другими плагинами?

О:
```lua
-- Регистрировать публичную функцию
plugin.registerPublicFunction("myFunc", function(arg)
    return "result"
end)

-- Вызвать функцию из другого плагина
local result = plugin.call("OtherPlugin", "someFunc", {"arg"})
```

### В: Как сохраняется конфигурация плагина?

О: Автоматически в:
- Linux: `~/.local/share/NotepadNext/config/PluginName.json`
- macOS: `~/Library/Application Support/NotepadNext/config/PluginName.json`
- Windows: `%APPDATA%/NotepadNext/config/PluginName.json`

Используйте:
```lua
config = plugin.getConfig()
config.myOption = "value"
plugin.saveConfig(config)
```

### В: Какова максимальная производительность Lua?

О: Lua работает с файлами размером до 100MB без проблем. Для больших файлов (>500MB) рекомендуется обрабатывать по частям:

```lua
local chunkSize = 1024 * 1024  -- 1MB
for i = 0, #text, chunkSize do
    local chunk = text:sub(i, i + chunkSize - 1)
    processChunk(chunk)
end
```

---

## Дополнительные ресурсы

- **Lua Documentation**: https://www.lua.org/manual/5.3/
- **NotepadNext API Docs**: (будет в репозитории)
- **Примеры плагинов**: `/examples/` в репозитории NotepadNext

---

**Последнее обновление**: 19 февраля 2026
