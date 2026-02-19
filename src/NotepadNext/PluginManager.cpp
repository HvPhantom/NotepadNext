/*
 * This file is part of Notepad Next.
 * Copyright 2019 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PluginManager.h"
#include "ScintillaNext.h"
#include "LuaExtension.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <algorithm>

#include "lua.hpp"

// ============================================================================
// Singleton pattern
// ============================================================================

PluginManager &PluginManager::instance() {
    static PluginManager singleton;
    return singleton;
}

// ============================================================================
// Lifecycle
// ============================================================================

bool PluginManager::initialize(ScintillaNext *editor_) {
    if (editor == editor_) {
        qWarning() << "PluginManager already initialized";
        return false;
    }
    
    editor = editor_;
    if (!editor) {
        qWarning() << "PluginManager: editor is null";
        return false;
    }
    
    qDebug() << "PluginManager initialized";
    return true;
}

void PluginManager::finalize() {
    // Уведомить о shut down
    notifyShutdown();
    
    // Выгрузить все плагины
    QStringList pluginNames = getLoadedPlugins();
    for (const auto &name : pluginNames) {
        unloadPlugin(name);
    }
    
    plugins.clear();
    failedPlugins.clear();
    editor = nullptr;
    
    qDebug() << "PluginManager finalized";
}

// ============================================================================
// Plugin Loading and Management
// ============================================================================

void PluginManager::loadPluginsFromDirectory(const QString &path) {
    QDir pluginsDir(path);
    
    if (!pluginsDir.exists()) {
        qWarning() << "Plugins directory does not exist:" << path;
        return;
    }
    
    // Ищем поддиректории, каждая - один плагин
    QStringList entries = pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const auto &entry : entries) {
        QString pluginPath = pluginsDir.absoluteFilePath(entry);
        if (!loadPlugin(pluginPath)) {
            qWarning() << "Failed to load plugin:" << entry;
        }
    }
}

bool PluginManager::loadPlugin(const QString &path) {
    if (!editor) {
        qWarning() << "PluginManager: editor not initialized";
        return false;
    }
    
    QDir pluginDir(path);
    
    // Проверить наличие manifest.json
    QString manifestPath = pluginDir.absoluteFilePath("manifest.json");
    if (!QFile::exists(manifestPath)) {
        failedPlugins[pluginDir.dirName()] = "manifest.json not found";
        return false;
    }
    
    // Парсить manifest
    QJsonObject manifest = parseManifest(manifestPath);
    if (manifest.isEmpty()) {
        failedPlugins[pluginDir.dirName()] = "Failed to parse manifest.json";
        return false;
    }
    
    QString pluginName = manifest.value("name").toString();
    QString version = manifest.value("version").toString();
    
    if (pluginName.isEmpty()) {
        failedPlugins[pluginDir.dirName()] = "Plugin name is empty";
        return false;
    }
    
    // Проверить версионную совместимость
    QString appVersion = "0.1.0"; // TODO: получить из ApplicationSettings
    QString compatVersions = manifest.value("nnp-compatible-versions").toString();
    
    if (!compatVersions.isEmpty() && !isVersionCompatible(compatVersions, appVersion)) {
        QString error = QString("Version incompatible: plugin requires %1, app is %2")
            .arg(compatVersions, appVersion);
        failedPlugins[pluginName] = error;
        return false;
    }
    
    // Проверить точку входа
    QString entryFile = manifest.value("entry").toString("init.lua");
    QString entryPath = pluginDir.absoluteFilePath(entryFile);
    if (!QFile::exists(entryPath)) {
        failedPlugins[pluginName] = QString("Entry file not found: %1").arg(entryFile);
        return false;
    }
    
    // Создать Lua state для плагина
    lua_State *L = createPluginLuaState(path, manifest);
    if (!L) {
        failedPlugins[pluginName] = "Failed to create Lua state";
        return false;
    }
    
    // Создать информацию о плагине
    auto pluginInfo = std::make_shared<PluginInfo>();
    pluginInfo->name = pluginName;
    pluginInfo->version = version;
    pluginInfo->description = manifest.value("description").toString();
    pluginInfo->author = manifest.value("author").toString();
    pluginInfo->path = path;
    pluginInfo->luaState = L;
    pluginInfo->enabled = true;
    pluginInfo->manifest = manifest;
    pluginInfo->config = loadPluginConfig(pluginName);
    
    // Сохранить информацию
    plugins[pluginName] = pluginInfo;
    
    // Регистрировать команды из manifest
    QJsonArray commands = manifest.value("commands").toArray();
    for (const auto &cmdValue : commands) {
        QJsonObject cmd = cmdValue.toObject();
        QString cmdId = cmd.value("id").toString();
        QString title = cmd.value("title").toString();
        
        if (!cmdId.isEmpty() && !title.isEmpty()) {
            // TODO: Добавить команду в меню
            qDebug() << "Registered command:" << cmdId << "-" << title;
        }
    }
    
    qDebug() << "Plugin loaded successfully:" << pluginName;
    return true;
}

void PluginManager::unloadPlugin(const QString &pluginName) {
    auto it = plugins.find(pluginName);
    if (it == plugins.end()) {
        qWarning() << "Plugin not found:" << pluginName;
        return;
    }
    
    auto info = it.value();
    
    if (info->luaState) {
        // Сохранить конфиг перед выгрузкой
        savePluginConfig(pluginName, info->config);
        
        lua_close(info->luaState);
        info->luaState = nullptr;
    }
    
    plugins.erase(it);
    qDebug() << "Plugin unloaded:" << pluginName;
}

void PluginManager::reloadPlugin(const QString &pluginName) {
    auto it = plugins.find(pluginName);
    if (it == plugins.end()) {
        qWarning() << "Plugin not found:" << pluginName;
        return;
    }
    
    QString path = it.value()->path;
    unloadPlugin(pluginName);
    loadPlugin(path);
}

// ============================================================================
// Command Execution
// ============================================================================

bool PluginManager::executeCommand(const QString &commandId) {
    // Формат commandId: "pluginName.commandName"
    QStringList parts = commandId.split('.');
    if (parts.size() < 2) {
        qWarning() << "Invalid command ID format:" << commandId;
        return false;
    }
    
    QString pluginName = parts[0];
    QString cmdName = parts[1];
    
    auto it = plugins.find(pluginName);
    if (it == plugins.end()) {
        qWarning() << "Plugin not found:" << pluginName;
        return false;
    }
    
    lua_State *L = it.value()->luaState;
    if (!L) {
        qWarning() << "Plugin Lua state is null:" << pluginName;
        return false;
    }
    
    // Получить функцию из Lua глобального пространства
    QString funcName = QString("executeCommand_%1").arg(cmdName);
    
    lua_getglobal(L, funcName.toStdString().c_str());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        qWarning() << "Command function not found:" << funcName;
        return false;
    }
    
    // Выполнить функцию
    int result = lua_pcall(L, 0, 0, 0);
    if (result != LUA_OK) {
        const char *errMsg = lua_tostring(L, -1);
        qWarning() << "Error executing command:" << (errMsg ? errMsg : "unknown");
        lua_pop(L, 1);
        return false;
    }
    
    return true;
}

QString PluginManager::callPluginFunction(const QString &pluginName, 
                                         const QString &functionName,
                                         const QStringList &args) {
    auto it = plugins.find(pluginName);
    if (it == plugins.end()) {
        qWarning() << "Plugin not found:" << pluginName;
        return QString();
    }
    
    lua_State *L = it.value()->luaState;
    if (!L) {
        qWarning() << "Plugin Lua state is null:" << pluginName;
        return QString();
    }
    
    // Получить функцию
    lua_getglobal(L, functionName.toStdString().c_str());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        qWarning() << "Function not found:" << functionName;
        return QString();
    }
    
    // Передать аргументы
    for (const auto &arg : args) {
        lua_pushstring(L, arg.toStdString().c_str());
    }
    
    // Выполнить
    int result = lua_pcall(L, args.size(), 1, 0);
    if (result != LUA_OK) {
        const char *errMsg = lua_tostring(L, -1);
        qWarning() << "Error calling function:" << (errMsg ? errMsg : "unknown");
        lua_pop(L, 1);
        return QString();
    }
    
    // Получить результат
    QString returnValue;
    if (lua_isstring(L, -1)) {
        returnValue = QString::fromUtf8(lua_tostring(L, -1));
    }
    lua_pop(L, 1);
    
    return returnValue;
}

// ============================================================================
// Events (Broadcasting to all plugins)
// ============================================================================

void PluginManager::notifyReady() {
    for (auto &entry : plugins) {
        lua_State *L = entry.second->luaState;
        if (!L) continue;
        
        // Вызвать plugin.on('ready') обработчик
        lua_getglobal(L, "__plugin_event_ready");
        if (lua_isfunction(L, -1)) {
            int result = lua_pcall(L, 0, 0, 0);
            if (result != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                qWarning() << "Error in plugin event handler:" << (err ? err : "");
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }
    }
}

void PluginManager::notifyBeforeFileOpen(const QString &filename) {
    broadcastFileEvent("beforeFileOpen", filename);
}

void PluginManager::notifyAfterFileOpen(const QString &filename) {
    broadcastFileEvent("afterFileOpen", filename);
}

void PluginManager::notifyBeforeFileSave(const QString &filename) {
    broadcastFileEvent("beforeFileSave", filename);
}

void PluginManager::notifyAfterFileSave(const QString &filename) {
    broadcastFileEvent("afterFileSave", filename);
}

void PluginManager::notifyBeforeFileClose(const QString &filename) {
    broadcastFileEvent("beforeFileClose", filename);
}

void PluginManager::notifyAfterFileClose(const QString &filename) {
    broadcastFileEvent("afterFileClose", filename);
}

void PluginManager::notifyShutdown() {
    for (auto &entry : plugins) {
        lua_State *L = entry.second->luaState;
        if (!L) continue;
        
        lua_getglobal(L, "__plugin_event_shutdown");
        if (lua_isfunction(L, -1)) {
            int result = lua_pcall(L, 0, 0, 0);
            if (result != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                qWarning() << "Error in shutdown handler:" << (err ? err : "");
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }
    }
}

void PluginManager::broadcastFileEvent(const QString &eventName, const QString &filename) {
    for (auto &entry : plugins) {
        lua_State *L = entry.second->luaState;
        if (!L) continue;
        
        // Вызвать __plugin_event_<eventName>(filename)
        QString handlerName = QString("__plugin_event_%1").arg(eventName);
        lua_getglobal(L, handlerName.toStdString().c_str());
        
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, filename.toStdString().c_str());
            int result = lua_pcall(L, 1, 0, 0);
            if (result != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                qWarning() << "Error in event handler" << eventName << ":" << (err ? err : "");
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }
    }
}

// ============================================================================
// Plugin Information
// ============================================================================

QStringList PluginManager::getLoadedPlugins() const {
    QStringList result;
    for (const auto &entry : plugins) {
        result.append(entry.first);
    }
    return result;
}

QMap<QString, QString> PluginManager::getFailedPlugins() const {
    return failedPlugins;
}

const PluginInfo *PluginManager::getPluginInfo(const QString &pluginName) const {
    auto it = plugins.find(pluginName);
    if (it == plugins.end()) {
        return nullptr;
    }
    return it.value().get();
}

QString PluginManager::getPluginVersion(const QString &pluginName) const {
    auto info = getPluginInfo(pluginName);
    if (!info) {
        return QString();
    }
    return info->version;
}

bool PluginManager::isVersionCompatible(const QString &pluginVersion, 
                                       const QString &appVersion) const {
    // Формат: "[0.1.0, 1.0.0]"
    // Извлечь min и max версии
    
    if (!pluginVersion.startsWith("[") || !pluginVersion.endsWith("]")) {
        return true; // нет ограничений версии
    }
    
    QString inner = pluginVersion.mid(1, pluginVersion.length() - 2);
    QStringList versions = inner.split(',');
    
    if (versions.size() != 2) {
        return true;
    }
    
    QString minV = versions[0].trimmed();
    QString maxV = versions[1].trimmed();
    
    // Простое сравнение версий (нужно улучшить для более сложных версий)
    return appVersion >= minV && appVersion <= maxV;
}

// ============================================================================
// Plugin API Registration (в отдельном файле PluginAPI.cpp)
// ============================================================================

void PluginManager::registerPluginAPI(lua_State *L) {
    // Это будет реализовано в PluginAPI.cpp
    // Пока используется базовая регистрация в LuaExtension
}

// ============================================================================
// Приватные методы
// ============================================================================

QJsonObject PluginManager::parseManifest(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open manifest file:" << path;
        return QJsonObject();
    }
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON in manifest:" << path;
        return QJsonObject();
    }
    
    return doc.object();
}

lua_State *PluginManager::createPluginLuaState(const QString &pluginPath,
                                               const QJsonObject &manifest) {
    lua_State *L = luaL_newstate();
    if (!L) {
        qWarning() << "Failed to create Lua state";
        return nullptr;
    }
    
    luaL_openlibs(L);
    
    // Загрузить init.lua
    QString entryFile = manifest.value("entry").toString("init.lua");
    QString entryPath = QDir(pluginPath).absoluteFilePath(entryFile);
    
    int result = luaL_loadfile(L, entryPath.toStdString().c_str());
    if (result != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        qWarning() << "Failed to load plugin script:" << (err ? err : "unknown");
        lua_pop(L, 1);
        lua_close(L);
        return nullptr;
    }
    
    // Выполнить скрипт
    result = lua_pcall(L, 0, 0, 0);
    if (result != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        qWarning() << "Failed to execute plugin script:" << (err ? err : "unknown");
        lua_pop(L, 1);
        lua_close(L);
        return nullptr;
    }
    
    return L;
}

QMap<QString, QString> PluginManager::loadPluginConfig(const QString &pluginName) {
    QString configPath = getPluginConfigPath(pluginName);
    QFile file(configPath);
    
    QMap<QString, QString> config;
    
    if (!file.exists()) {
        return config;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isObject()) {
        return config;
    }
    
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        config[it.key()] = it.value().toString();
    }
    
    return config;
}

void PluginManager::savePluginConfig(const QString &pluginName,
                                    const QMap<QString, QString> &config) {
    QString configPath = getPluginConfigPath(pluginName);
    
    // Создать директорию если её нет
    QDir configDir = QFileInfo(configPath).dir();
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    
    // Создать JSON объект
    QJsonObject obj;
    for (auto it = config.begin(); it != config.end(); ++it) {
        obj[it.key()] = it.value();
    }
    
    QJsonDocument doc(obj);
    
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson());
        file.close();
    }
}

QString PluginManager::getPluginConfigPath(const QString &pluginName) {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QDir::separator() + "plugins" + QDir::separator() + "config";
    
    return configDir + QDir::separator() + pluginName + ".json";
}
