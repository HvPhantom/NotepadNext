/*
 * Plugin API - Lua bindings for NotepadNext plugins
 * This file is part of Notepad Next.
 * Copyright 2019 Justin Dailey
 *
 * Provides Lua API for plugins:
 * - plugin.*   (metadata and management)
 * - editor.*   (text editing)
 * - ui.*       (user interface)
 * - fs.*       (filesystem)
 * - settings.* (application settings)
 */

#include "PluginManager.h"
#include "ScintillaNext.h"

#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QDebug>

#include "lua.hpp"

// ============================================================================
// Helper functions for Lua C API
// ============================================================================

static int luaL_tableField(lua_State *L, const char *key, const char *value) {
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
    return 0;
}

static void luaL_tableSetString(lua_State *L, const char *key, const char *value) {
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
}

static void luaL_tableSetInteger(lua_State *L, const char *key, lua_Integer value) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

// ============================================================================
// plugin.* API
// ============================================================================

static int plugin_info(lua_State *L) {
    // Возвращает таблицу с информацией о плагине
    lua_newtable(L);
    
    // TODO: получить информацию из PluginManager
    luaL_tableSetString(L, "name", "placeholder");
    luaL_tableSetString(L, "version", "0.1.0");
    luaL_tableSetString(L, "author", "");
    
    return 1;
}

static int plugin_getConfig(lua_State *L) {
    // Получить конфигурацию плагина
    // TODO: реализовать
    lua_newtable(L);
    return 1;
}

static int plugin_saveConfig(lua_State *L) {
    // Сохранить конфигурацию плагина
    // TODO: реализовать
    return 0;
}

static int plugin_getRootPath(lua_State *L) {
    // Получить путь к корневой директории плагина
    // TODO: реализовать
    lua_pushstring(L, "/path/to/plugin");
    return 1;
}

static int plugin_registerCommand(lua_State *L) {
    // Регистрировать команду плагина
    // Аргумент: таблица с полями {id, title, execute, ...}
    
    if (!lua_istable(L, 1)) {
        luaL_error(L, "Expected table argument");
        return 0;
    }
    
    lua_getfield(L, 1, "id");
    const char *id = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "title");
    const char *title = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "execute");
    if (!lua_isfunction(L, -1)) {
        luaL_error(L, "execute field must be a function");
    }
    
    // Сохранить функцию в глобальное пространство
    QString funcName = QString("executeCommand_%1").arg(id);
    lua_setglobal(L, funcName.toStdString().c_str());
    
    qDebug() << "Plugin command registered:" << id << "-" << title;
    return 0;
}

static int plugin_on(lua_State *L) {
    // Зарегистрировать обработчик события
    // Использование: plugin.on("ready", function() ... end)
    
    const char *eventName = luaL_checkstring(L, 1);
    if (!lua_isfunction(L, 2)) {
        luaL_error(L, "Expected function as second argument");
        return 0;
    }
    
    // Сохранить функцию в глобальное пространство
    QString handlerName = QString("__plugin_event_%1").arg(eventName);
    lua_pushvalue(L, 2);
    lua_setglobal(L, handlerName.toStdString().c_str());
    
    return 0;
}

static int plugin_call(lua_State *L) {
    // Вызвать функцию из другого плагина
    // plugin.call("OtherPlugin", "functionName", {...args...})
    
    const char *pluginName = luaL_checkstring(L, 1);
    const char *functionName = luaL_checkstring(L, 2);
    
    QStringList args;
    int argc = lua_gettop(L) - 2;
    for (int i = 0; i < argc; i++) {
        if (lua_isstring(L, 3 + i)) {
            args.append(QString::fromUtf8(lua_tostring(L, 3 + i)));
        }
    }
    
    QString result = PluginManager::instance().callPluginFunction(
        QString::fromUtf8(pluginName),
        QString::fromUtf8(functionName),
        args
    );
    
    lua_pushstring(L, result.toUtf8().constData());
    return 1;
}

static int plugin_log(lua_State *L) {
    const char *message = luaL_checkstring(L, 1);
    qDebug() << "[Plugin]" << message;
    return 0;
}

static int plugin_logError(lua_State *L) {
    const char *message = luaL_checkstring(L, 1);
    qWarning() << "[Plugin Error]" << message;
    return 0;
}

// ============================================================================
// editor.* API (через IFaceTable - уже реализовано в LuaExtension)
// Здесь добавляем дополнительные функции
// ============================================================================

static int editor_getCurrentFile(lua_State *L) {
    // TODO: получить из EditorManager текущий файл
    lua_pushstring(L, "");
    return 1;
}

static int editor_openFile(lua_State *L) {
    // editor.openFile(filename)
    const char *filename = luaL_checkstring(L, 1);
    
    lua_pushboolean(L, true); // TODO: проверить результат
    return 1;
}

static int editor_saveFile(lua_State *L) {
    // editor.saveFile()
    lua_pushboolean(L, true); // TODO: проверить результат
    return 1;
}

static int editor_closeFile(lua_State *L) {
    // editor.closeFile(filename)
    const char *filename = luaL_checkstring(L, 1);
    
    lua_pushboolean(L, true); // TODO: проверить результат
    return 1;
}

// ============================================================================
// ui.* API
// ============================================================================

static int ui_message(lua_State *L) {
    const char *title = luaL_checkstring(L, 1);
    const char *message = luaL_checkstring(L, 2);
    
    // TODO: показать диалог MessageBox
    qDebug() << "[UI Message]" << title << "-" << message;
    
    return 0;
}

static int ui_confirm(lua_State *L) {
    const char *title = luaL_checkstring(L, 1);
    const char *message = luaL_checkstring(L, 2);
    
    // TODO: показать диалог подтверждения
    lua_pushboolean(L, true); // TODO: вернуть реальный результат
    
    return 1;
}

static int ui_input(lua_State *L) {
    const char *label = luaL_checkstring(L, 1);
    const char *defaultValue = luaL_optstring(L, 2, "");
    
    // TODO: показать диалог ввода
    lua_pushstring(L, defaultValue);
    
    return 1;
}

static int ui_select(lua_State *L) {
    // ui.select(items, defaultIndex)
    // items - это таблица строк
    
    if (!lua_istable(L, 1)) {
        luaL_error(L, "Expected table of items");
        return 0;
    }
    
    // TODO: показать список выбора
    lua_pushstring(L, "");
    
    return 1;
}

static int ui_getClipboard(lua_State *L) {
    // TODO: получить текст из буфера обмена
    lua_pushstring(L, "");
    return 1;
}

static int ui_setClipboard(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    
    // TODO: установить текст в буфер обмена
    lua_pushboolean(L, true);
    
    return 1;
}

// ============================================================================
// fs.* API (Filesystem)
// ============================================================================

static int fs_read(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    
    QFile file(QString::fromUtf8(filepath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lua_pushnil(L);
        return 1;
    }
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    
    lua_pushstring(L, content.toUtf8().constData());
    return 1;
}

static int fs_write(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    const char *data = luaL_checkstring(L, 2);
    
    QFile file(QString::fromUtf8(filepath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    file.write(QByteArray(data));
    file.close();
    
    lua_pushboolean(L, true);
    return 1;
}

static int fs_append(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    const char *data = luaL_checkstring(L, 2);
    
    QFile file(QString::fromUtf8(filepath));
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    file.write(QByteArray(data));
    file.close();
    
    lua_pushboolean(L, true);
    return 1;
}

static int fs_exists(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    bool exists = QFile::exists(QString::fromUtf8(filepath));
    lua_pushboolean(L, exists);
    return 1;
}

static int fs_isFile(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    QFileInfo info(QString::fromUtf8(filepath));
    lua_pushboolean(L, info.isFile());
    return 1;
}

static int fs_isDirectory(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    QFileInfo info(QString::fromUtf8(filepath));
    lua_pushboolean(L, info.isDir());
    return 1;
}

static int fs_mkdir(lua_State *L) {
    const char *dirpath = luaL_checkstring(L, 1);
    QDir dir;
    bool created = dir.mkpath(QString::fromUtf8(dirpath));
    lua_pushboolean(L, created);
    return 1;
}

static int fs_listdir(lua_State *L) {
    const char *dirpath = luaL_checkstring(L, 1);
    
    QDir dir(QString::fromUtf8(dirpath));
    if (!dir.exists()) {
        lua_pushnil(L);
        return 1;
    }
    
    QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    lua_newtable(L);
    for (int i = 0; i < entries.size(); i++) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, entries[i].toUtf8().constData());
        lua_settable(L, -3);
    }
    
    return 1;
}

static int fs_realpath(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    QFileInfo info(QString::fromUtf8(filepath));
    lua_pushstring(L, info.canonicalFilePath().toUtf8().constData());
    return 1;
}

static int fs_basename(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    QString name = QFileInfo(QString::fromUtf8(filepath)).fileName();
    lua_pushstring(L, name.toUtf8().constData());
    return 1;
}

static int fs_dirname(lua_State *L) {
    const char *filepath = luaL_checkstring(L, 1);
    QString dir = QFileInfo(QString::fromUtf8(filepath)).canonicalPath();
    lua_pushstring(L, dir.toUtf8().constData());
    return 1;
}

static int fs_join(lua_State *L) {
    int argc = lua_gettop(L);
    QString result;
    
    for (int i = 1; i <= argc; i++) {
        if (lua_isstring(L, i)) {
            if (i > 1) result += QDir::separator();
            result += QString::fromUtf8(lua_tostring(L, i));
        }
    }
    
    lua_pushstring(L, result.toUtf8().constData());
    return 1;
}

// ============================================================================
// settings.* API
// ============================================================================

static int settings_get(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    
    // TODO: получить из ApplicationSettings
    lua_pushstring(L, "");
    
    return 1;
}

static int settings_set(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    const char *value = luaL_checkstring(L, 2);
    
    // TODO: сохранить в ApplicationSettings
    lua_pushboolean(L, true);
    
    return 1;
}

static int settings_has(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    
    // TODO: проверить наличие в ApplicationSettings
    lua_pushboolean(L, false);
    
    return 1;
}

static int settings_remove(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    
    // TODO: удалить из ApplicationSettings
    lua_pushboolean(L, true);
    
    return 1;
}

// ============================================================================
// Регистрация всех функций в Lua
// ============================================================================

void registerPluginAPI(lua_State *L) {
    // plugin.*
    lua_newtable(L);
    
    lua_pushcfunction(L, plugin_info);
    lua_setfield(L, -2, "info");
    
    lua_pushcfunction(L, plugin_getConfig);
    lua_setfield(L, -2, "getConfig");
    
    lua_pushcfunction(L, plugin_saveConfig);
    lua_setfield(L, -2, "saveConfig");
    
    lua_pushcfunction(L, plugin_getRootPath);
    lua_setfield(L, -2, "getRootPath");
    
    lua_pushcfunction(L, plugin_registerCommand);
    lua_setfield(L, -2, "registerCommand");
    
    lua_pushcfunction(L, plugin_on);
    lua_setfield(L, -2, "on");
    
    lua_pushcfunction(L, plugin_call);
    lua_setfield(L, -2, "call");
    
    lua_pushcfunction(L, plugin_log);
    lua_setfield(L, -2, "log");
    
    lua_pushcfunction(L, plugin_logError);
    lua_setfield(L, -2, "logError");
    
    lua_setglobal(L, "plugin");
    
    // ui.*
    lua_newtable(L);
    
    lua_pushcfunction(L, ui_message);
    lua_setfield(L, -2, "message");
    
    lua_pushcfunction(L, ui_confirm);
    lua_setfield(L, -2, "confirm");
    
    lua_pushcfunction(L, ui_input);
    lua_setfield(L, -2, "input");
    
    lua_pushcfunction(L, ui_select);
    lua_setfield(L, -2, "select");
    
    lua_pushcfunction(L, ui_getClipboard);
    lua_setfield(L, -2, "getClipboard");
    
    lua_pushcfunction(L, ui_setClipboard);
    lua_setfield(L, -2, "setClipboard");
    
    lua_setglobal(L, "ui");
    
    // fs.*
    lua_newtable(L);
    
    lua_pushcfunction(L, fs_read);
    lua_setfield(L, -2, "read");
    
    lua_pushcfunction(L, fs_write);
    lua_setfield(L, -2, "write");
    
    lua_pushcfunction(L, fs_append);
    lua_setfield(L, -2, "append");
    
    lua_pushcfunction(L, fs_exists);
    lua_setfield(L, -2, "exists");
    
    lua_pushcfunction(L, fs_isFile);
    lua_setfield(L, -2, "isFile");
    
    lua_pushcfunction(L, fs_isDirectory);
    lua_setfield(L, -2, "isDirectory");
    
    lua_pushcfunction(L, fs_mkdir);
    lua_setfield(L, -2, "mkdir");
    
    lua_pushcfunction(L, fs_listdir);
    lua_setfield(L, -2, "listdir");
    
    lua_pushcfunction(L, fs_realpath);
    lua_setfield(L, -2, "realpath");
    
    lua_pushcfunction(L, fs_basename);
    lua_setfield(L, -2, "basename");
    
    lua_pushcfunction(L, fs_dirname);
    lua_setfield(L, -2, "dirname");
    
    lua_pushcfunction(L, fs_join);
    lua_setfield(L, -2, "join");
    
    lua_setglobal(L, "fs");
    
    // settings.*
    lua_newtable(L);
    
    lua_pushcfunction(L, settings_get);
    lua_setfield(L, -2, "get");
    
    lua_pushcfunction(L, settings_set);
    lua_setfield(L, -2, "set");
    
    lua_pushcfunction(L, settings_has);
    lua_setfield(L, -2, "has");
    
    lua_pushcfunction(L, settings_remove);
    lua_setfield(L, -2, "remove");
    
    lua_setglobal(L, "settings");
    
    qDebug() << "Plugin API registered successfully";
}
