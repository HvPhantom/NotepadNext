#pragma once

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <memory>

struct lua_State;
class ScintillaNext;

/**
 * @class PluginInfo
 * @brief Информация о загруженном плагине
 */
struct PluginInfo {
    QString name;           // имя плагина
    QString version;        // версия
    QString description;    // описание
    QString author;         // автор
    QString path;           // путь к плагину
    lua_State *luaState;    // Lua VM для плагина
    bool enabled;           // активирован ли
    QString error;          // сообщение об ошибке при загрузке
    
    QJsonObject manifest;   // manifest.json
    QMap<QString, QString> config; // конфигурация плагина
};

/**
 * @class PluginManager
 * @brief Менеджер плагинов - управляет загрузкой, выполнением и событиями плагинов
 * 
 * Singleton: используйте PluginManager::instance()
 * 
 * Примечание: Построен на основе NPAPI плагинов Notepad++,
 * но использует Lua для кроссплатформенности
 */
class PluginManager {
public:
    static PluginManager &instance();
    
    /**
     * @brief Инициализировать менеджер плагинов
     * @param editor указатель на главный редактор
     * @return в случае успеха возвращает true
     */
    bool initialize(ScintillaNext *editor);
    
    /**
     * @brief Завершить менеджер плагинов (выгрузить все плагины)
     */
    void finalize();
    
    // === Загрузка и управление плагинами ===
    
    /**
     * @brief Загрузить все плагины из директории
     * @param path путь к директории с плагинами
     * 
     * Ищет все поддиректории, содержащие manifest.json и init.lua
     */
    void loadPluginsFromDirectory(const QString &path);
    
    /**
     * @brief Загрузить отдельный плагин
     * @param path путь к корневой директории плагина
     * @return в случае успеха возвращает true
     */
    bool loadPlugin(const QString &path);
    
    /**
     * @brief Выгрузить плагин
     * @param pluginName имя плагина
     */
    void unloadPlugin(const QString &pluginName);
    
    /**
     * @brief Перезагрузить плагин
     * @param pluginName имя плагина
     */
    void reloadPlugin(const QString &pluginName);
    
    // === Выполнение команд ===
    
    /**
     * @brief Выполнить команду плагина
     * @param commandId ID команды (формат: pluginName.commandId)
     * @return в случае успеха возвращает true
     */
    bool executeCommand(const QString &commandId);
    
    /**
     * @brief Вызвать функцию из плагина (плагин-плагин коммуникация)
     * @param pluginName имя плагина
     * @param functionName имя функции
     * @param args аргументы функции
     * @return результат выполнения
     */
    QString callPluginFunction(const QString &pluginName, 
                              const QString &functionName,
                              const QStringList &args = QStringList());
    
    // === События плагинов (аналог NPPN_* из Notepad++) ===
    
    /**
     * @brief Уведомить плагины о готовности приложения
     * (аналог NPPN_READY)
     */
    void notifyReady();
    
    /**
     * @brief Уведомить плагины перед открытием файла
     * @param filename путь к файлу
     * (аналог NPPN_FILEBEFORELOAD)
     */
    void notifyBeforeFileOpen(const QString &filename);
    
    /**
     * @brief Уведомить плагины после открытия файла
     * @param filename путь к файлу
     * (аналог NPPN_FILEAFTERLOAD)
     */
    void notifyAfterFileOpen(const QString &filename);
    
    /**
     * @brief Уведомить плагины перед сохранением файла
     * @param filename путь к файлу
     * (аналог NPPN_FILEBEFORESAVE)
     */
    void notifyBeforeFileSave(const QString &filename);
    
    /**
     * @brief Уведомить плагины после сохранения файла
     * @param filename путь к файлу
     * (аналог NPPN_FILEAFTERSAVE)
     */
    void notifyAfterFileSave(const QString &filename);
    
    /**
     * @brief Уведомить плагины перед закрытием файла
     * @param filename путь к файлу
     * (аналог NPPN_FILEBEFORECLOSE)
     */
    void notifyBeforeFileClose(const QString &filename);
    
    /**
     * @brief Уведомить плагины после закрытия файла
     * @param filename путь к файлу
     * (аналог NPPN_FILEAFTERCLOSE)
     */
    void notifyAfterFileClose(const QString &filename);
    
    /**
     * @brief Уведомить плагины перед завершением приложения
     * (аналог NPPN_SHUTDOWN)
     */
    void notifyShutdown();
    
    // === Информация о плагинах ===
    
    /**
     * @brief Получить список загруженных плагинов
     * @return список имён загруженных плагинов
     */
    QStringList getLoadedPlugins() const;
    
    /**
     * @brief Получить список плагинов с ошибками загрузки
     * @return список имён и ошибок
     */
    QMap<QString, QString> getFailedPlugins() const;
    
    /**
     * @brief Получить информацию о плагине
     * @param pluginName имя плагина
     * @return информацию о плагине, или nullptr если не найден
     */
    const PluginInfo *getPluginInfo(const QString &pluginName) const;
    
    /**
     * @brief Получить версию плагина
     * @param pluginName имя плагина
     * @return версию плагина, или пустую строку если не найден
     */
    QString getPluginVersion(const QString &pluginName) const;
    
    /**
     * @brief Проверить совместимость версий
     * @param pluginVersion версия плагина из manifest
     * @param appVersion текущая версия приложения
     * @return true если версии совместимы
     * 
     * Формат версии в manifest: "nnp-compatible-versions": "[0.1.0, 1.0.0]"
     */
    bool isVersionCompatible(const QString &pluginVersion, 
                            const QString &appVersion) const;
    
    // === Plugin API регистрация (внутреннее использование) ===
    
    /**
     * @brief Зарегистрировать встроенный Plugin API в Lua
     * @param L Lua state
     * 
     * Регистрирует следующие API:
     * - plugin.* (info, config, registerCommand, on, call)
     * - ui.* (message, dockBar, input, select)
     * - fs.* (read, write, listdir, mkdir)
     * - settings.* (get, set)
     */
    void registerPluginAPI(lua_State *L);
    
private:
    // === Приватные методы ===
    
    /**
     * @brief Парсить manifest.json плагина
     * @param path путь к плагину
     * @return распарсенный JSON объект
     */
    QJsonObject parseManifest(const QString &path);
    
    /**
     * @brief Создать Lua state для плагина и загрузить init.lua
     * @param pluginPath путь к плагину
     * @param manifest manifest.json
     * @return Lua state, или nullptr если ошибка
     */
    lua_State *createPluginLuaState(const QString &pluginPath,
                                    const QJsonObject &manifest);
    
    /**
     * @brief Загрузить конфигурацию плагина
     * @param pluginName имя плагина
     * @return конфигурация плагина
     */
    QMap<QString, QString> loadPluginConfig(const QString &pluginName);
    
    /**
     * @brief Сохранить конфигурацию плагина
     * @param pluginName имя плагина
     * @param config конфигурация
     */
    void savePluginConfig(const QString &pluginName,
                         const QMap<QString, QString> &config);
    
    /**
     * @brief Вызвать Lua функцию плагина
     * @param L Lua state плагина
     * @param functionName имя функции
     * @return результат выполнения в виде строки
     */
    QString callLuaFunction(lua_State *L, const QString &functionName);
    
    /**
     * @brief Трансляция событий файла для всех плагинов
     * @param eventName имя события (afterFileOpen, beforeFileSave и т.д.)
     * @param filename путь к файлу
     */
    void broadcastFileEvent(const QString &eventName, const QString &filename);
    
    /**
     * @brief Получить путь к файлу конфигурации плагина
     * @param pluginName имя плагина
     * @return полный путь к JSON файлу конфигурации
     */
    QString getPluginConfigPath(const QString &pluginName);
    
    // === Члены класса ===
    
    ScintillaNext *editor;                      // указатель на редактор
    QMap<QString, std::shared_ptr<PluginInfo>> plugins;  // загруженные плагины
    QMap<QString, QString> failedPlugins;       // плагины не загруженные
    
    // Singleton
    PluginManager() = default;
    ~PluginManager();
    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;
};
