# Анализ системы плагинов: Notepad++ vs NotepadNext

## 1. Архитектура Notepad++

### 1.1 Основные характеристики
- **Тип плагинов**: Нативные DLL (C/C++), загружаемые динамически
- **ОС**: Только Windows
- **Архитектуры**: x86, x64, ARM64
- **Взаимодействие**: Windows Message API
- **Безопасность**: Версионная совместимость, проверка подписей

### 1.2 Plugin Interface (NPAPI)

Каждый плагин должен экспортировать следующие функции:

```c
extern "C" __declspec(dllexport) void setInfo(NppData);          // инициализация
extern "C" __declspec(dllexport) const wchar_t * getName();       // имя плагина  
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *); // команды
extern "C" __declspec(dllexport) void beNotified(SCNotification *); // события
extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM); // сообщения
extern "C" __declspec(dllexport) BOOL isUnicode();                // поддержка Unicode
```

### 1.3 Ключевые структуры данных

#### NppData - информация о главном окне
```cpp
struct NppData {
    HWND _nppHandle;           // главное окно
    HWND _scintillaMainHandle; // основной редактор
    HWND _scintillaSecondHandle; // вторичный редактор
};
```

#### FuncItem - команда плагина
```cpp
struct FuncItem {
    wchar_t _itemName[64];     // название в меню
    PFUNCPLUGINCMD _pFunc;     // указатель на функцию
    int _cmdID;                // ID команды
    bool _init2Check;          // галочка
    ShortcutKey *_pShKey;      // горячая клавиша
};
```

### 1.4 Структура директорий

```
Notepad++/
├── plugins/
│   ├── PluginName/
│   │   ├── PluginName.dll
│   │   └── PluginName.xml         (конфигурация)
│   └── Config/
│       ├── nppPluginList.dll      (подписанный реестр)
│       └── PluginName/
│           └── PluginName.xml
└── updater/
    └── gup.exe                    (автоновление)
```

### 1.5 Система обмена сообщениями (NPAPI)

Плагины общаются с приложением через Windows Messages в диапазонах:

- **`NPPM_*`** - команды Notepad++
- **`NPPN_*`** - события (READY, FILELOAD, FILESAVE, SHUTDOWN и т.д.)
- **`WM_*`** - стандартные Windows сообщения через Scintilla

#### Примеры команд NPAPI:
```cpp
NPPM_DOOPEN              // открыть файл
NPPM_SAVECURRENTFILE     // сохранить
NPPM_DMMREGASDCKDLG      // пристыковать диалог
NPPM_ALLOCATECMDID       // получить ID команды
NPPM_MSGTOPLUGIN         // плагин-плагин сообщение
NPPM_GETPLUGINSCONFIGDIR // папка конфигурации
```

### 1.6 Система версионности

Плагины указывают совместимость:
```json
"npp-compatible-versions": "[6.9, 8.5]"  // от v6.9 до v8.5
```

---

## 2. Архитектура NotepadNext

### 2.1 Основные характеристики
- **Тип расширений**: Lua scripts
- **ОС**: Cross-platform (Windows, Linux, macOS)
- **Взаимодействие**: Lua API + bindings к Scintilla
- **Модель**: Embedded Lua VM, встроенная в процесс приложения
- **Язык**: Lua 5.3

### 2.2 Lua Extension System

Реализована в [LuaExtension.cpp](src/NotepadNext/LuaExtension.cpp) как singleton:

```cpp
class LuaExtension final {
    static LuaExtension &Instance();           // singleton
    bool Initialise(lua_State *L, ScintillaNext *editor_);
    bool RunString(const char *s);             // выполнить код
    bool OnExecute(const char *s);             // callback
    void CallShortcut(int id);                 // вызвать команду
};
```

### 2.3 Bindings к Scintilla

Lua получает доступ к Scintilla через `editor` object:

```lua
local L = {}

function OnStyleNeeded()
    editor:styleSetFore(50, 0xFF0000)  -- красный текст
    editor:setLexer("cpp")             -- C++ подсветка
end

function OnKey()
    editor:insertText("Hello")
end
```

Все IFace методы Scintilla доступны через динамическое разрешение имён.

### 2.4 Структура файлов языков

```
NotepadNext/
├── src/NotepadNext/
│   ├── LuaExtension.h/cpp      (основная система)
│   └── languages/
│       ├── cpp.lua
│       ├── python.lua
│       ├── javascript.lua
│       └── ...
│
└── src/lua/                    (Lua VM engine)
    ├── src/
    │   ├── lua.h
    │   ├── lapi.c
    │   └── ...
    └── ...
```

### 2.5 Поддерживаемые callbacks

В коде указаны (закомментированы) события:

```cpp
// Scintilla события
OnStyle, OnChar, OnSavePointReached, OnUpdateUI, OnMarginClick, ...

// Notepad++ события (не активированы)
OnReady, OnBeforeClose, OnOpen, OnBeforeSave, OnSave, OnShutdown, ...
```

---

## 3. Сравнительный анализ

| Параметр | Notepad++ | NotepadNext |
|----------|-----------|------------|
| **Тип плагинов** | Native DLL (C/C++) | Lua scripts |
| **ОС** | Windows only | Cross-platform |
| **Безопасность** | Низкая (прямой доступ к памяти) | Высокая (sandbox Lua) |
| **Производительность** | Высокая (native code) | Хорошая (Lua JIT capable) |
| **Простота разработки** | Сложная (C/C++, WinAPI) | Простая (Lua script) |
| **Версионность** | Версионная совместимость | Не реализована |
| **Auto-update** | Встроенная (gup.exe) | Не реализована |
| **Plugin registry** | Подписанный nppPluginList.dll | Не реализована |
| **Меню интеграция** | Автоматическая | Ручная через Lua |
| **Горячие клавиши** | Через FuncItem структуру | Не реализовано |
| **Docking** | NPPM_DMMREGASDCKDLG | Не реализовано |
| **Plugin-plugin IPC** | NPPM_MSGTOPLUGIN | Не реализовано |

---

## 4. Гибридный подход для NotepadNext

Рекомендуется реализовать **двухуровневую систему**:
1. **Lua extension framework** (основной, уже есть)
2. **C++ Plugin API** (опциональный, для высокопроизводительных модулей)

### 4.1 Улучшения Lua Extension System

#### A. Система нагрузки плагинов

Структура директорий:
```
$HOME/.local/share/NotepadNext/      (Linux)
$HOME/Library/Application Support/NotepadNext/  (macOS)
%APPDATA%/NotepadNext/               (Windows)
├── extensions/                      (пользовательские Lua скрипты)
│   ├── plugin1/
│   │   ├── init.lua                (точка входа)
│   │   ├── manifest.json           (метаданные)
│   │   └── lib/
│   │       └── helper.lua
│   └── plugin2/...
├── config/                          (конфигурация плагинов)
│   └── plugin1.json
└── cache/                           (кэш, plugins.json)
    └── plugins.json
```

#### B. Формат manifest.json

```json
{
  "name": "ExamplePlugin",
  "version": "1.0.0",
  "description": "Example Notepad++ plugin adaptation",
  "author": "Your Name",
  "license": "GPL-3.0",
  "homepage": "https://github.com/example/plugin",
  "entry": "init.lua",
  "nnp-compatible-versions": "[0.1.0, 1.0.0]",
  "permissions": ["editor", "filesystem", "ui"],
  "commands": [
    {
      "id": "example.hello",
      "title": "Say Hello",
      "category": "Example",
      "keybinding": "ctrl+shift+h"
    }
  ],
  "dependencies": []
}
```

#### C. Plugin Lifecycle API (Lua)

```lua
-- api.lua - предоставляемый API

-- === Информация о плагине ===
plugin.info() → {name, version, author}
plugin.getConfig() → table
plugin.saveConfig(table)

-- === События (callbacks) ===
plugin.on('ready', function() ... end)
plugin.on('beforeOpen', function(file) ... end)
plugin.on('afterOpen', function(file) ... end)
plugin.on('beforeSave', function(file) ... end)
plugin.on('afterSave', function(file) ... end)
plugin.on('beforeClose', function(file) ... end)
plugin.on('afterClose', function(file) ... end)
plugin.on('shutdown', function() ... end)

-- === Команды меню ===
plugin.registerCommand('plugin.commandId', function()
    --  реализация
end)

-- === UI элементы ===
ui.dockBar(name, widget)     -- пристыковать диалог
ui.message(title, text)      -- диалог
ui.input(prompt)             -- ввод
ui.select(items)             -- выбор

-- === Файловая система ===
fs.read(path) → string
fs.write(path, data) → bool
fs.listdir(path) → []string
fs.mkdir(path) → bool

-- === Редактор ===
editor.getText() → string
editor.setText(string)
editor.getSelectedText() → string
editor.insert(text)
editor.find(pattern) → []position
editor.replace(pattern, replacement) → int

-- === Настройки ===
settings.get(key) → value
settings.set(key, value)
```

### 4.2 Реализация Plugin Manager

[Файл: src/NotepadNext/PluginManager.h/cpp]

```cpp
class PluginManager {
public:
    static PluginManager &Instance();
    
    // Управление жизненным циклом
    void loadPluginsFromDirectory(const QString &path);
    void unloadPlugin(const QString &name);
    void reloadPlugin(const QString &name);
    
    // Выполнение команд плагина
    void executeCommand(const QString &pluginId);
    
    // События
    void notifyReady();
    void notifyFileLoaded(const QString &filename);
    void notifyFileSaved(const QString &filename);
    void notifyBeforeClose(const QString &filename);
    
    // Информация о плагинах
    QStringList getLoadedPlugins();
    QStringList getFailedPlugins();
    QString getPluginVersion(const QString &name);
    
private:
    struct PluginInfo {
        QString name;
        QString version;
        QString path;
        lua_State *L;
        bool enabled;
        QString error;
    };
    
    QMap<QString, PluginInfo> loadedPlugins;
};
```

### 4.3 Расширение LuaExtension

```cpp
// scripts/api.lua - встроенный модуль
void LuaExtension::RegisterPluginAPI(lua_State *L) {
    // plugin.* функции
    // ui.* функции  
    // fs.* функции
    // editor.* функции (уже есть)
    // settings.* функции
}
```

---

## 5. Адаптация функций Notepad++

### 5.1 Функции NPAPI → Lua API

| Notepad++ NPAPI | NotepadNext Lua | Статус |
|-----------------|-----------------|--------|
| `NPPM_DOOPEN` | `editor.open(path)` | ✓ Добавить |
| `NPPM_SAVECURRENT` | `editor.save()` | ✓ Добавить |
| `NPPM_DMMREGASDCKDLG` | `ui.dockBar(name, widget)` | ✓ Добавить |
| `NPPM_ALLOCATECMDID` | Автоматический (по ID в manifest) | ✓ Реализовано |
| `NPPM_MSGTOPLUGIN` | `plugin.call('name', 'func', args)` | ✓ Добавить |
| `NPPM_GETPLUGINHOMEPATH` | `plugin.getRootPath()` | ✓ Добавить |
| `NPPN_READY` | `plugin.on('ready')` | ✓ Добавить |
| `NPPN_FILEBEFORELOAD` | `plugin.on('beforeOpen')` | ✓ Добавить |
| `NPPN_FILEAFTERSAVE` | `plugin.on('afterSave')` | ✓ Добавить |

### 5.2 Функции Scintilla → Lua

Уже реализовано через IFaceTable:
```lua
editor:styleSetFore(index, color)
editor:setLexer(name)
editor:getText()
editor:setText(text)
editor:insertText(text)
editor:append(text)
-- и 1000+ других методов
```

### 5.3 Горячие клавиши (Keybindings)

Из manifest.json автоматически регистрируются:
```json
"commands": [{
    "id": "example.hello",
    "keybinding": "ctrl+shift+h"
}]
```

В коде НЕ НУЖНА ручная регистрация, как в Notepad++.

---

## 6. Версионная совместимость

### 6.1 Версионная проверка

manifest.json:
```json
"nnp-compatible-versions": "[0.1.0, 1.0.0]"
```

PluginManager проверяет при загрузке:
```cpp
bool isCompatible(const QString &pluginVersion, const QString &appVersion) {
    // парс строк [min, max]
    // проверка appVersion >= min && appVersion <= max
}
```

### 6.2 Система уведомлений (Changelog)

plugins.json в кэше:
```json
{
  "status": "success",
  "version": "1",
  "plugins": [
    {
      "folder-name": "plugin1",
      "version": "1.0.0",
      "nnp-compatible-versions": "[0.1.0, 1.0.0]",
      "changelog": "v1.0.0: Initial release"
    }
  ]
}
```

---

## 7. План реализации (Roadmap)

### Фаза 1: Core Plugin Manager (2-3 недели)
- [ ] `PluginManager` класс
- [ ] Загрузка Lua скриптов из директорий
- [ ] Парс manifest.json
- [ ] Регистрация команд плагинов в меню

### Фаза 2: Plugin API (3-4 недели)
- [ ] `plugin.*` API (info, config, registerCommand)
- [ ] `plugin.on()` события (ready, fileOpen, fileSave, shutdown)
- [ ] `ui.*` API (message, dockBar, input)
- [ ] `fs.*` API (read, write, listdir)
- [ ] `settings.*` API

### Фаза 3: Advanced Features (2-3 недели)
- [ ] plugin-to-plugin IPC (`plugin.call()`)
- [ ] Keybindings из manifest
- [ ] Auto-update система (check updates, download, install)
- [ ] Plugin registry (plugins.json кэш)

### Фаза 4: Compatibility & Polish (1-2 недели)
- [ ] Версионная совместимость
- [ ] Error handling и логирование
- [ ] Plugin sandbox/permissions system
- [ ] Documentation и примеры

---

## 8. Пример: Адаптация плагина HexViewer

### Notepad++ Binary Plugin (DLL)
```c
extern "C" __declspec(dllexport) void setInfo(NppData data) {
    // сохранить handles
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *sc) {
    if (sc->nmhdr.code == NPPN_READY) {
        // инициализация
    }
}

// FuncItem с командой "Show Hex View"
FuncItem funcItems[] = {
    {L"Show Hex View", showHexView, 0, false, NULL}
};
```

### NotepadNext Lua Plugin
```lua
-- plugins/hexviewer/init.lua

local plugin = {}

function plugin:onReady()
    -- создать UI для hex viewer
    local hexPanel = createHexViewerPanel()
    ui.dockBar("Hex Viewer", hexPanel)
end

function plugin:onEditorChange()
    -- обновить hex view при изменении текста
    local text = editor:getText()
    updateHexDisplay(text)
end

-- Регистрация команды
plugin:registerCommand({
    id = "hexviewer.show",
    title = "Show Hex View",
    category = "Tools",
    execute = function()
        ui.dockBar("Hex Viewer"):show()
    end
})

return plugin
```

manifest.json:
```json
{
  "name": "HexViewer",
  "version": "1.0.0",
  "description": "Hex viewer for Notepad Next",
  "author": "Your Name",
  "entry": "init.lua",
  "commands": [{
    "id": "hexviewer.show",
    "title": "Show Hex View",
    "keybinding": "ctrl+shift+x"
  }]
}
```

---

## 9. Ключевые различия при адаптации

### Что нужно изменить от Notepad++:
1. **Язык**: C/C++ → Lua
2. **Threading**: Windows Message Queue → Qt Signals/Slots
3. **UI**: Resource scripts → Qt QML/Widgets
4. **File paths**: Windows paths → Platform-independent
5. **Registry/Config**: Windows Registry → JSON/TOML files
6. **Compilation**: MSVC only → Portable Lua

### Что остаётся похожим:
1. **API концепция**: Commands + Events
2. **Menu integration**: Плагины добавляют команды в меню
3. **Hotkeys**: Регистрация горячих клавиш
4. **Docking**: Пристыкование диалогов
5. **File events**: Open, Save, Close events

---

## 10. Примеры встроенных плагинов (из Notepad++)

Кандидаты для портирования:

1. **TextFX** - трансформация текста
2. **Compare** - сравнение файлов
3. **HexViewer** - просмотр в hex формате
4. **NppExec** - выполнение команд
5. **Spell Checker** - проверка орфографии
6. **Multi-clipboard** - расширенный clipboard

Все могут быть реализованы на Lua с лучшей кроссплатформенностью.

---

## Заключение

NotepadNext имеет преимущество кроссплатформенности с Lua-системой. Для полной совместимости с экосистемой Notepad++ рекомендуется:

1. **Расширить Lua API** в соответствии с предложенной архитектурой
2. **Реализовать PluginManager** для управления жизненным циклом
3. **Создать Plugin Registry** для дополнительных плагинов
4. **Добавить горячие клавиши** из manifest.json
5. **Опционально**: C++ Plugin API для высокопроизводительных модулей

Это позволит привлечь разработчиков Notepad++ плагинов, адаптировав их под кроссплатформенную Lua базу NotepadNext.
