/*
 * This file is part of ctrl_iip
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include "core/Exceptions.h"
#include "forwarder/Scoreboard.h"
#include "core/SimpleLogger.h"
#include "core/RedisConnection.h"

int NUM_EVENTS = 2;

Scoreboard::Scoreboard(const std::string& host,
                       const int& port,
                       const int& db_num,
                       const std::string& password) { 
    _host = host;
    _port = port;
    _db_num = db_num;
    _password = password;
}

Scoreboard::~Scoreboard() { 

}

bool Scoreboard::is_ready(const std::string& image_id) { 
    auto list = _db[image_id];
    if (list.size() == NUM_EVENTS) { 
        return true; 
    }
    return false;
} 

void Scoreboard::add(const std::string& image_id, const std::string& event) { 
    auto itr = _db.find(image_id); 
    std::set<std::string> events;
    if (itr != _db.end()) {
        events = _db[image_id];
    }
    events.insert(event);
    _db[image_id] = events;
}

void Scoreboard::remove(const std::string& image_id) {
    auto itr = _db.find(image_id); 
    if (itr == _db.end()) {
        throw L1::KeyNotFound("Cannot remove " + image_id + " because key not found");
    }
    _db.erase(image_id);
}

void Scoreboard::add_xfer(const std::string& image_id, const xfer_info& xfer) { 
    _xfer[image_id] = xfer;
}

xfer_info Scoreboard::get_xfer(const std::string& image_id) { 
    auto itr = _db.find(image_id); 
    if (itr == _db.end()) {
        throw L1::KeyNotFound("Cannot get transfer parameters for " + image_id +
                " because key not found");
    }
    return _xfer[image_id];
}

void Scoreboard::set_fwd(const std::string& key, const std::string& body) { 
    RedisConnection redis(_host, _port);
    redis.select(_db_num);
    redis.lpush(key.c_str(), body);
}
