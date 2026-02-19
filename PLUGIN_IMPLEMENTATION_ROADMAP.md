# Реализация системы плагинов для NotepadNext - Технический план

**Документ**: Техническая спецификация и план реализации  
**Дата**: 19 февраля 2026  
**Версия**: 1.0  
**Статус**: Для обсуждения и утверждения

---

## 1. Резюме исследования

### 1.1 Анализ текущих систем

#### Notepad++ Plugin System (NPAPI)
- **Язык**: C/C++
- **Формат**: Native DLL плагины
- **ОС**: Windows only
- **Модель**: Динамическая загрузка DLL, callback функции
- **Преимущества**: Высокая производительность, полный доступ к системе
- **Недостатки**: Низкая безопасность, сложность разработки, платформо-зависимость

#### NotepadNext Lua Extension System
- **Язык**: Lua 5.3
- **Формат**: Lua scripts
- **ОС**: Cross-platform (Windows, Linux, macOS)
- **Модель**: Embedded Lua VM, вызов Lua функций
- **Преимущества**: Кроссплатформа, безопасность, простота разработки
- **Недостатки**: Производительность ниже нативного кода, нет пока plugin manager

### 1.2 Вывод

NotepadNext имеет **идеальный фундамент** для системы плагинов - встроенный Lua VM и IFace bindings к Scintilla. Требуется **расширение** текущей системы с добавлением:

1. **Plugin Manager** - управление жизненным циклом плагинов
2. **Plugin Registry** - каталог и версионность
3. **Plugin API** - полный API для работы с приложением
4. **Auto-update** - автоновление плагинов

---

## 2. Архитектурные рекомендации

### 2.1 Гибридный подход (Рекомендуется)

```
┌─────────────────────────────────────────────────┐
│         NotepadNext Application                 │
├─────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────┐  │
│  │      Plugin Manager (C++)                │  │
│  │  - Loader & Lifecycle                    │  │
│  │  - Registry & Discovery                  │  │
│  │  - Version Compatibility                 │  │
│  └──────────────────────────────────────────┘  │
│                    │                             │
│  ┌─────────────────┴──────────────────────────┐ │
│  │                                            │ │
│  v                                            v │
│ ┌──────────────────┐        ┌──────────────┐  │
│ │  Lua Plugins     │        │ C++ Plugins  │  │
│ │ (Primary)        │        │ (Optional)   │  │
│ │                  │        │              │  │
│ │ • init.lua       │        │ • Native DLL │  │
│ │ • manifest.json  │        │ • TBD Format │  │
│ │ • lib/           │        │              │  │
│ └──────────────────┘        └──────────────┘  │
│                                                 │
│  Plugin API                                     │
│  ┌──────────────────────────────────────────┐  │
│  │ plugin.* ui.* fs.* settings.* editor.*   │  │
│  └──────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

### 2.2 Три уровня реализации

**Фаза 1: Core (2-3 недели)**
- Plugin Manager: загрузка, выгрузка, события
- Базовая API: editor, ui.message, fs.read/write

**Фаза 2: Enhancement (3-4 недели)**
- Полная API: ui, fs, settings, plugin
- Keybindings и menu integration
- Plugin registry

**Фаза 3: Polish (2-3 недели)**
- Auto-update система
- Plugin repository
- Документация и примеры

---

## 3. Запуск: Компоненты для реализации

### 3.1 Core Component: PluginManager (C++)

**Файлы для создания**:
- `src/NotepadNext/PluginManager.h`  (уже создан - [PluginManager.h](../src/NotepadNext/PluginManager.h))
- `src/NotepadNext/PluginManager.cpp`  (требуется)
- `src/NotepadNext/PluginAPI.h`  (требуется)
- `src/NotepadNext/PluginAPI.cpp`  (требуется)

**Классы:**

```cpp
class PluginManager {
    // Singleton
    static PluginManager &instance();
    
    // Lifecycle
    bool initialize(ScintillaNext *editor);
    void finalize();
    
    // Loading
    void loadPluginsFromDirectory(const QString &path);
    bool loadPlugin(const QString &path);
    void unloadPlugin(const QString &pluginName);
    void reloadPlugin(const QString &pluginName);
    
    // Execution
    bool executeCommand(const QString &commandId);
    QString callPluginFunction(const QString &pluginName, 
                              const QString &functionName,
                              const QStringList &args);
    
    // Events (Broadcasting)
    void notifyReady();
    void notifyBeforeFileOpen(const QString &filename);
    void notifyAfterFileOpen(const QString &filename);
    void notifyBeforeFileSave(const QString &filename);
    void notifyAfterFileSave(const QString &filename);
    void notifyBeforeFileClose(const QString &filename);
    void notifyAfterFileClose(const QString &filename);
    void notifyShutdown();
    
    // Info
    QStringList getLoadedPlugins() const;
    QMap<QString, QString> getFailedPlugins() const;
    const PluginInfo *getPluginInfo(const QString &name) const;
    QString getPluginVersion(const QString &name) const;
    bool isVersionCompatible(const QString &pluginVersion,
                            const QString &appVersion) const;
};
```

### 3.2 Plugin API Module

**Регистрируемые Lua API**:

#### A. `plugin.*` - Метаинформация и управление

```lua
-- Информация о плагине
plugin.info() → { name, version, author, description }
plugin.name() → string
plugin.version() → string

-- Конфигурация
plugin.getConfig() → table
plugin.saveConfig(table)
plugin.getRootPath() → string

-- Команды
plugin.registerCommand(spec)  
plugin.registerPublicFunction(name, func)
plugin.call(pluginName, functionName, args) → string

-- События
plugin.on(eventName, callback)

-- Utilities
plugin.isFirstRun() → boolean
plugin.log(message)
plugin.logError(message)
```

#### B. `editor.*` - Работа с редактором

```lua
-- Уже реализовано через IFaceTable:
editor:getText() → string
editor:setText(string)
editor:getSelectedText() → string
editor:insertText(string)
editor:replaceSelection(string)
editor:setLexer(string)

-- Требуется добавить:
editor:getCurrentFile() → string
editor:getFileList() → table
editor:switchToFile(filename) → boolean
editor:openFile(filename) → boolean
editor:saveFile() → boolean
editor:closeFile(filename) → boolean
editor:findReplace(pattern, replacement, flags) → integer
editor:getLineCount() → integer
editor:getLine(lineNum) → string
editor:appendLine(text) → boolean
```

#### C. `ui.*` - User Interface

```lua
-- Диалоги
ui.message(title, message) → nil
ui.confirm(title, message) → boolean
ui.input(label, default) → string | nil
ui.select(items, default) → string | nil
ui.multiSelect(items) → table | nil

-- Docking
ui.dockBar(name, widget) → widget

-- Текущее состояние
ui.getClipboard() → string
ui.setClipboard(string) → boolean
ui.getSelectedText() → string
```

#### D. `fs.*` - Файловая система

```lua
-- Чтение/Запись
fs.read(path) → string | nil
fs.write(path, data) → boolean
fs.append(path, data) → boolean
fs.delete(path) → boolean

-- Навигация
fs.exists(path) → boolean
fs.isFile(path) → boolean
fs.isDirectory(path) → boolean
fs.mkdir(path) → boolean
fs.listdir(path) → table | nil
fs.realpath(path) → string
fs.basename(path) → string
fs.dirname(path) → string
fs.join(...) → string

-- Поиск
fs.glob(pattern) → table
fs.find(startPath, pattern) → table
```

#### E. `settings.*` - Настройки приложения

```lua
-- Сохраняемые параметры
settings.get(key) → value
settings.set(key, value) → boolean
settings.has(key) → boolean
settings.remove(key) → boolean
settings.getAll() → table
settings.clear() → boolean

-- Наблюдение
settings.watch(key, callback)
settings.unwatch(key)
```

### 3.3 Manifest Schema и Validation

**manifest.json** структура (JSON Schema):

```json
{
  "type": "object",
  "required": ["name", "version", "entry"],
  "properties": {
    "name": { "type": "string" },
    "version": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+$" },
    "description": { "type": "string" },
    "author": { "type": "string" },
    "license": { "type": "string" },
    "homepage": { "type": "string", "format": "uri" },
    "repository": { "type": "object" },
    "entry": { "type": "string" },
    "nnp-compatible-versions": {
      "type": "string",
      "pattern": "^\\[[\\d.]+, [\\d.]+\\]$"
    },
    "commands": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["id", "title"],
        "properties": {
          "id": { "type": "string", "pattern": "^[a-z0-9.-]+$" },
          "title": { "type": "string" },
          "category": { "type": "string" },
          "description": { "type": "string" },
          "keybinding": { "type": "string" },
          "icon": { "type": "string" }
        }
      }
    },
    "permissions": {
      "type": "array",
      "items": { "type": "string" }
    }
  }
}
```

---

## 4. План реализации (Roadmap)

### Этап 1: Plugin Manager Core (Неделя 1-2)

**Задачи**:
- [ ] Реализовать `PluginManager` класс
- [ ] Парсинг manifest.json (с валидацией)
- [ ] Загрузка Lua скриптов в отдельные states
- [ ] Базовое управление жизненным циклом
- [ ] Unit тесты для базовой функциональности

**Файлы**:
- [ ] `src/NotepadNext/PluginManager.cpp`
- [ ] `src/NotepadNext/PluginAPI.h/cpp`
- [ ] `tests/PluginManagerTest.cpp`

**Результат**: Плагины загружаются, выполняются команды, но нет API.

### Этап 2: Plugin API Implementation (Неделя 3-4)

**Задачи**:
- [ ] Реализовать `plugin.*` API
- [ ] Реализовать `ui.*` API (исключая docking)
- [ ] Реализовать `fs.*` API
- [ ] Реализовать `settings.*` API
- [ ] Расширить `editor.*` API

**Файлы**:
- [ ] `src/NotepadNext/PluginAPI.cpp` (основная реализация)
- [ ] Пример плагина: [SamplePlugin](../examples/SamplePlugin/)

**Результат**: Плагины имеют полный доступ к функциональности приложения.

### Этап 3: Advanced Features (Неделя 5-6)

**Задачи**:
- [ ] Keybinding регистрация из manifest
- [ ] Plugin-to-plugin IPC через `plugin.call()`
- [ ] Menu integration (автоматическое добавление в меню)
- [ ] Version compatibility checking
- [ ] Plugin registry (plugins.json)

**Результат**: Полнофункциональная система плагинов, аналог Notepad++.

### Этап 4: Polish & Documentation (Неделя 7)

**Задачи**:
- [ ] Error handling и logging
- [ ] Performance optimization
- [ ] Documentation (API reference, tutorials)
- [ ] Example plugins porting (TextFX, Compare)
- [ ] Тестирование и баг фиксинг

**Результат**: Production-ready система, готовая к использованию.

---

## 5. Примеры адаптированных плагинов

### 5.1 Candidate Plugins для Портирования

1. **TextFX** (Трансформация текста)
   - Сложность: ⭐ Низкая
   - Ожидаемое время: 2 дня
   - Функции: UPPERCASE, lowercase, Sort, Remove duplicates

2. **Compare** (Сравнение файлов)
   - Сложность: ⭐⭐ Средняя
   - Ожидаемое время: 3-4 дня
   - Функции: Side-by-side compare, Diff viewer

3. **NppExec** (Выполнение команд)
   - Сложность: ⭐⭐ Средняя
   - Ожидаемое время: 3-4 дня
   - Функции: Execute shell commands, Capture output

4. **HexViewer** (Просмотр в hex)
   - Сложность: ⭐ Низкая
   - Ожидаемое время: 2 дня
   - Функции: Hex dump, ASCII preview

5. **Spell Checker** (Проверка орфографии)
   - Сложность: ⭐⭐⭐ Высокая
   - Ожидаемое время: 1 неделя
   - Функции: Словарь, подсчёт ошибок

---

## 6. Интеграция с существующим кодом

### 6.1 Интеграция с LuaExtension

**Текущее состояние**:
- `LuaExtension::Instance()` - singleton Lua VM
- Методы: `Initialise()`, `RunString()`, `OnExecute()`, `CallShortcut()`
- Доступ к `editor` object через IFaceTable

**Требуемые изменения**:

```cpp
// В NotepadNext/MainWindow (или главный класс приложения)

void MainWindow::onApplicationReady() {
    // Инициализировать Plugin Manager
    PluginManager::instance().initialize(editor);
    PluginManager::instance().loadPluginsFromDirectory(pluginsPath);
    PluginManager::instance().notifyReady();
}

void MainWindow::onFileOpened(const QString &filename) {
    PluginManager::instance().notifyAfterFileOpen(filename);
}

void MainWindow::onFileSaved(const QString &filename) {
    PluginManager::instance().notifyAfterFileSave(filename);
}

void MainWindow::onApplicationClosing() {
    PluginManager::instance().notifyShutdown();
    PluginManager::instance().finalize();
}
```

### 6.2 Интеграция с Menu

**Находится в**: Plugins → [каждый плагин получает подменю]

```cpp
// В MainWindow::setupMenu()

// Автоматически создаётся при loadPlugin():
for (const auto &command : plugin->commands) {
    QAction *action = new QAction(command.title);
    connect(action, &QAction::triggered, [command]() {
        PluginManager::instance().executeCommand(command.id);
    });
    pluginsMenu->addAction(action);
}
```

---

## 7. Тестирование и QA

### 7.1 Unit Tests

```cpp
// tests/PluginManagerTest.cpp

TEST_F(PluginManagerTest, LoadValidPlugin) {
    EXPECT_TRUE(manager.loadPlugin(validPluginPath));
}

TEST_F(PluginManagerTest, FailOnInvalidManifest) {
    EXPECT_FALSE(manager.loadPlugin(invalidManifestPath));
}

TEST_F(PluginManagerTest, VersionCompatibilityCheck) {
    EXPECT_TRUE(manager.isVersionCompatible("[0.1.0, 1.0.0]", "0.5.0"));
    EXPECT_FALSE(manager.isVersionCompatible("[0.1.0, 0.9.0]", "1.0.0"));
}

TEST_F(PluginManagerTest, CommandExecution) {
    manager.loadPlugin(samplePluginPath);
    EXPECT_TRUE(manager.executeCommand("sample.command1"));
}

TEST_F(PluginManagerTest, EventBroadcasting) {
    manager.loadPlugin(samplePluginPath);
    EXPECT_NO_THROW(manager.notifyReady());
    EXPECT_NO_THROW(manager.notifyAfterFileOpen("test.txt"));
}
```

### 7.2 Integration Tests

- Загрузка multiple плагинов
- Plugin lifecycle (load → execute → unload)
- Event propagation
- IPC между плагинами
- Memory leak detection

### 7.3 Example Plugin Tests

- TextFX плагин: проверка всех трансформаций
- Compare плагин: проверка корректности сравнения
- HexViewer плагин: проверка hex dump вывода

---

## 8. Документация

### 8.1 API Reference

**Файлы**:
- `docs/PLUGIN_API.md` - Полный API справочник
- `docs/PLUGIN_MANIFEST.md` - Формат manifest.json

### 8.2 Tutorials

- `docs/TUTORIAL_1_HELLO_WORLD.md` - Первый плагин
- `docs/TUTORIAL_2_TEXT_TRANSFORMATION.md` - Работа с текстом
- `docs/TUTORIAL_3_FILE_OPERATIONS.md` - Файловая система
- `docs/TUTORIAL_4_UI_DIALOGS.md` - User Interface

### 8.3 Examples

- `examples/HelloWorld/` - Минимальный плагин
- `examples/TextTransform/` - Трансформация текста
- `examples/DockablePanel/` - Пристыкованная панель (когда будет реализовано)

---

## 9. Риски и Mitigation

| Риск | Вероятность | Impact | Mitigation |
|------|------------|--------|-----------|
| API complexity | Средняя | Высокий | Iterative design, community feedback |
| Performance issues | Низкая | Средний | Profiling, optimization |
| Version compatibility | Средняя | Средний | Rigorous testing, changelog |
| Security issues | Низкая | Высокий | Sandbox, permissions system |
| Documentation gaps | Средняя | Низкий | Community wiki, examples |

---

## 10. Успешные критерии

✅ **Фаза 1 завершена**:
- Plugin Manager загружает и выгружает плагины
- Базовая API работает
- Пример плагина успешно загружается та выполняется

✅ **Фаза 2 завершена**:
- Полная Plugin API реализована
- TextFX плагин работает полностью
- Нет production-breaking bugs

✅ **Фаза 3 завершена**:
- Версионная совместимость работает
- Compare плагин портирован успешно
- Auto-update система функциональна

✅ **Фаза 4 завершена**:
- Документация полна и актуальна
- 3-5 плагинов успешно портировано
- Community может разрабатывать плагины

---

## 11. Ресурсы для разработки

### Существующая документация:
- [PLUGIN_SYSTEM_ANALYSIS.md](../PLUGIN_SYSTEM_ANALYSIS.md) - Полный анализ систем Notepad++ и NotepadNext
- [PLUGIN_ADAPTATION_GUIDE.md](../PLUGIN_ADAPTATION_GUIDE.md) - Руководство по адаптации плагинов
- [SamplePlugin](../examples/SamplePlugin/) - Пример плагина с manifest.json и init.lua

### Справочники:
- Notepad++ Plugin Interface: [PluginInterface.h](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/MISC/PluginsManager/PluginInterface.h)
- Lua 5.3 Manual: https://www.lua.org/manual/5.3/
- Qt Documentation: https://doc.qt.io/

---

## Заключение

NotepadNext имеет **отличный потенциал** стать мощным инструментом для разработчиков благодаря:

1. **Кроссплатформенности** - Lua скрипты работают везде
2. **Простоте разработки** - Lua проще, чем C++ с WinAPI
3. **Встроенному Lua VM** - Быстрая интеграция с приложением
4. **Наличию IFace bindings** - Полный доступ к Scintilla

Предложенная архитектура обеспечит:
- Совместимость с экосистемой Notepad++
- Безопасность через Lua sandbox
- Простоту портирования плагинов
- Возможность развития сообщества

**Рекомендация**: Начать с **Фазы 1** (2-3 недели), получить feedback, затем продолжить развитие.

---

**Документ подготовлен**: 19 февраля 2026  
**Статус**: Готов к обсуждению и утверждению командой разработки
