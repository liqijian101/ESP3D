/*
sd_native_esp32.cpp - ESP3D sd support class

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
#include "../../include/esp3d_config.h"
#if defined (ARCH_ESP32) && defined(SD_FEATURE)
#if (SD_FEATURE == ESP_SD_NATIVE)
#include "esp_sd.h"
#include "FS.h"
#include "SD.h"

extern File tSDFile_handle[ESP_MAX_SD_OPENHANDLE];

bool ESP_SDFileSystem::begin()
{
    _started = SD.begin();
    return _started;
}

void ESP_SDFileSystem::end()
{
    SD.end();
    _started = false;
}

size_t ESP_SDFileSystem::totalBytes()
{
    return SD.totalBytes();
}

size_t ESP_SDFileSystem::usedBytes()
{
    return (SD.totalBytes() - SD.freeBytes());
}


const char * ESP_SDFileSystem::FilesystemName()
{
    return "FAT";
}

bool ESP_SDFileSystem::format()
{
    return SD.format();
}

ESP_SDFile ESP_SDFileSystem::open(const char* path, uint8_t mode)
{
    //do some check
    if(((strcmp(path,"/") == 0) && ((mode == ESP_FILE_WRITE) || (mode == ESP_FILE_APPEND))) || (strlen(path) == 0)) {
        return ESP_SDFile();
    }
    // path must start by '/'
    if (path[0] != '/') {
        return ESP_SDFile();
    }
    if (mode != ESP_FILE_READ) {
        //check container exists
        String p = path;
        p.remove(p.lastIndexOf('/') +1);
        if (!exists(p.c_str())) {
            log_esp3d("Error opening: %s", path);
            return ESP_SDFile();
        }
    }
    File tmp = SD.open(path, (mode == ESP_FILE_READ)?FILE_READ:(mode == ESP_FILE_WRITE)?FILE_WRITE:FILE_APPEND);
    ESP_SDFile esptmp(&tmp, tmp.isDirectory(),(mode == ESP_FILE_READ)?false:true, path);
    return esptmp;
}

bool ESP_SDFileSystem::exists(const char* path)
{
    bool res = false;
    //root should always be there if started
    if (strcmp(path, "/") == 0) {
        return _started;
    }
    res = SD.exists(path);
    if (!res) {
        ESP_SDFile root = ESP_SDFileSystem::open(path, ESP_FILE_READ);
        if (root) {
            res = root.isDirectory();
        }
    }
    return res;
}

bool ESP_SDFileSystem::remove(const char *path)
{
    return SD.remove(path);
}

bool ESP_SDFileSystem::mkdir(const char *path)
{
    return SD.mkdir(path);
}

bool ESP_SDFileSystem::rmdir(const char *path)
{
    if (!exists(path)) {
        return false;
    }
    bool res = true;
    GenLinkedList<String > pathlist;
    String p = path;
    pathlist.push(p);
    while (pathlist.count() > 0) {
        File dir = SD.open(pathlist.getLast().c_str());
        File f = dir.openNextFile();
        bool candelete = true;
        while (f) {
            if (f.isDirectory()) {
                candelete = false;
                String newdir = f.name();
                pathlist.push(newdir);
                f.close();
                f = File();
            } else {
                SD.remove(f.name());
                f.close();
                f = dir.openNextFile();
            }
        }
        if (candelete) {
            if (pathlist.getLast() !="/") {
                res = SD.rmdir(pathlist.getLast().c_str());
            }
            pathlist.pop();
        }
        dir.close();
    }
    p = String();
    log_esp3d("count %d", pathlist.count());
    return res;
}

void ESP_SDFileSystem::closeAll()
{
    for (uint8_t i = 0; i < ESP_MAX_OPENHANDLE; i++) {
        tSDFile_handle[i].close();
        tSDFile_handle[i] = File();
    }
}

ESP_SDFile::ESP_SDFile(void* handle, bool isdir, bool iswritemode, const char * path)
{
    _isdir = isdir;
    _dirlist = "";
    _isfakedir = false;
    _index = -1;
    _filename = "";
    _name = "";
#ifdef FILESYSTEM_TIMESTAMP_FEATURE
    memset (&_lastwrite,0,sizeof(time_t));
#endif //FILESYSTEM_TIMESTAMP_FEATURE 
    _iswritemode = iswritemode;
    _size = 0;
    if (!handle) {
        return ;
    }
    bool set =false;
    for (uint8_t i=0; (i < ESP_MAX_OPENHANDLE) && !set; i++) {
        if (!tSDFile_handle[i]) {
            tSDFile_handle[i] = *((File*)handle);
            //filename
            _filename = tSDFile_handle[i].name();

            //if root
            if (_filename == "/") {
                _filename = "/.";
            }
            if (_isdir) {
                if (_filename[_filename.length()-1] != '.') {
                    if (_filename[_filename.length()-2] != '/') {
                        _filename+="/";
                    }
                    _filename+=".";
                }
            }
            //name
            if (_filename == "/.") {
                _name = "/";
            } else {
                _name = _filename;
                if (_name.endsWith("/.")) {
                    _name.remove( _name.length() - 2,2);
                    _isfakedir = true;
                    _isdir = true;
                }
                if (_name[0] == '/') {
                    _name.remove( 0, 1);
                }
                int pos = _name.lastIndexOf('/');
                if (pos != -1) {
                    _name.remove( 0, pos+1);
                }
            }
            //size
            _size = tSDFile_handle[i].size();
            //time
#ifdef FILESYSTEM_TIMESTAMP_FEATURE
            _lastwrite =  tSDFile_handle[i].getLastWrite();
#endif //FILESYSTEM_TIMESTAMP_FEATURE
            _index = i;
            //log_esp3d("Opening File at index %d",_index);
            set = true;
        }
    }
}

void ESP_SDFile::close()
{
    if (_index != -1) {
        //log_esp3d("Closing File at index %d", _index);
        tSDFile_handle[_index].close();
        //reopen if mode = write
        //udate size + date
        if (_iswritemode && !_isdir) {
            File ftmp = SD.open(_filename.c_str());
            if (ftmp) {
                _size = ftmp.size();
#ifdef FILESYSTEM_TIMESTAMP_FEATURE
                _lastwrite = ftmp.getLastWrite();
#endif //FILESYSTEM_TIMESTAMP_FEATURE
                ftmp.close();
            }
        }
        tSDFile_handle[_index] = File();
        //log_esp3d("Closing File at index %d",_index);
        _index = -1;
    }
}

ESP_SDFile  ESP_SDFile::openNextFile()
{
    if ((_index == -1) || !_isdir) {
        log_esp3d("openNextFile failed");
        return ESP_SDFile();
    }
    File tmp = tSDFile_handle[_index].openNextFile();
    while (tmp) {
        log_esp3d("tmp name :%s %s", tmp.name(), (tmp.isDirectory())?"isDir":"isFile");
        ESP_SDFile esptmp(&tmp, tmp.isDirectory());
        esptmp.close();
        String sub = esptmp.filename();
        sub.remove(0,_filename.length()-1);
        int pos = sub.indexOf("/");
        if (pos!=-1) {
            //is subdir
            sub = sub.substring(0,pos);
            //log_esp3d("file name:%s name: %s %s  sub:%s root:%s", esptmp.filename(), esptmp.name(), (esptmp.isDirectory())?"isDir":"isFile", sub.c_str(), _filename.c_str());
            String tag = "*" + sub + "*";
            //test if already in directory list
            if (_dirlist.indexOf(tag) == -1) {//not in list so add it and return the info
                _dirlist+= tag;
                String fname = _filename.substring(0,_filename.length()-1) + sub + "/.";
                //log_esp3d("Found dir  name: %s filename:%s", sub.c_str(), fname.c_str());
                esptmp = ESP_SDFile(sub.c_str(), fname.c_str());
                return esptmp;
            } else { //already in list so ignore it
                //log_esp3d("Dir name: %s already in list", sub.c_str());
                tmp = tSDFile_handle[_index].openNextFile();
            }
        } else { //is file
            //log_esp3d("file name:%s name: %s %s  sub:%s root:%s", esptmp.filename(), esptmp.name(), (esptmp.isDirectory())?"isDir":"isFile", sub.c_str(), _filename.c_str());
            if (sub == ".") {
                //log_esp3d("Dir tag, ignore it");
                tmp = tSDFile_handle[_index].openNextFile();
            } else {
                return esptmp;
            }
        }

    }
    return  ESP_SDFile();
}


#endif //SD_FEATURE == ESP_SD_NATIVE
#endif //ARCH_ESP32 && SD_FEATURE