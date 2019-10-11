/*
  esp_sd.h - ESP3D SD support class

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _ESP_SD_H
#define _ESP_SD_H
#include "../../include/esp3d_config.h"
#ifdef SD_TIMESTAMP_FEATURE
#include <time.h>
#endif //SD_TIMESTAMP_FEATURE
#define ESP_SD_FILE_READ       0
#define ESP_SD_FILE_WRITE      1
#define ESP_SD_FILE_APPEND     2

#define ESP_SD_HEADER "/SD:"

#define ESP_MAX_SD_OPENHANDLE 4

class ESP_SDFile
{
public:
    ESP_SDFile(void *  handle = nullptr, bool isdir =false, bool iswritemode = false, const char * path = nullptr);
    ESP_SDFile(const char * name, const char * filename, bool isdir = true, size_t size =0);
    ~ESP_SDFile();
    operator bool() const;
    bool isDirectory();
    const char* name() const;
    const char* filename() const;
    void close();
    bool isOpen();
    ESP_SDFile & operator=(const ESP_SDFile & other);
    size_t size();
#ifdef SD_TIMESTAMP_FEATURE
    time_t getLastWrite();
#endif //SD_TIMESTAMP_FEATURE
    int available();
    size_t write(uint8_t i);
    size_t write(const uint8_t *buf, size_t size);
    int read();
    size_t read(uint8_t* buf, size_t size);
    void flush();
    ESP_SDFile openNextFile();
private:
    String _dirlist;
    bool _isdir;
    bool _isfakedir;
    bool _iswritemode;
    int8_t _index;
    String _filename;
    String _name;
    size_t _size;
#ifdef SD_TIMESTAMP_FEATURE
    time_t _lastwrite;
#endif //SD_TIMESTAMP_FEATURE
};

class ESP_SD
{
public:
    static String & formatBytes (uint64_t bytes);
    ESP_SD();
    ~ESP_SD();
    static bool begin();
    static void handle();
    static void end();
    static uint8_t getState(bool refresh);
    static uint8_t setState(uint8_t state);
    static uint64_t totalBytes();
    static uint64_t usedBytes();
    static uint64_t freeBytes();
    static const char * FilesystemName();
    static bool format();
    static ESP_SDFile open(const char* path, uint8_t mode = ESP_SD_FILE_READ);
    static bool exists(const char* path);
    static bool remove(const char *path);
    static bool mkdir(const char *path);
    static bool rmdir(const char *path);
    static void closeAll();
private:
    static bool _started;
    static uint8_t _state;
};


#endif //_ESP_SD_H