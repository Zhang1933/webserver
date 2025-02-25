/**************************************************************************
   Copyright (c) 2017 sewenew

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *************************************************************************/

#include "muduo/net/db/connection_pool.h"
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include "muduo/base/Logging.h"
#include "muduo/net/db/connection.h"
#include "muduo/net/db/errors.h"


namespace sw {

namespace redis {

ConnectionPool::ConnectionPool(const ConnectionPoolOptions &pool_opts,
        const ConnectionOptions &connection_opts) :
            _opts(connection_opts),
            _pool_opts(pool_opts) {
    if (_pool_opts.size == 0) {
        throw Error("CANNOT create an empty pool");
    }
    _reconnection_thread.reset(new muduo::EventLoopThread);
    _recoonction_loop.reset(_reconnection_thread->startLoop());
    _reconneciotn_list_Sptr=std::make_shared<std::list<Connection>>();
    _recoonction_loop->setContext(_reconneciotn_list_Sptr);
    // Lazily create connections.
}


void ConnectionPool::_move(ConnectionPool &&that) {
    _opts = std::move(that._opts);
    _pool_opts = std::move(that._pool_opts);
    _pool = std::move(that._pool);
    _used_connections = that._used_connections;
    // _sentinel = std::move(that._sentinel);
}

ConnectionPool::ConnectionPool(ConnectionPool &&that) {
    std::lock_guard<std::mutex> lock(that._mutex);

    _move(std::move(that));
}

ConnectionPool& ConnectionPool::operator=(ConnectionPool &&that) {
    if (this != &that) {
        std::lock(_mutex, that._mutex);
        std::lock_guard<std::mutex> lock_this(_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock_that(that._mutex, std::adopt_lock);

        _move(std::move(that));
    }

    return *this;
}

Connection ConnectionPool::fetch() {
    std::unique_lock<std::mutex> lock(_mutex);

    auto connection = _fetch(lock);

    auto connection_lifetime = _pool_opts.connection_lifetime;
    auto connection_idle_time = _pool_opts.connection_idle_time;

    lock.unlock();

    if (_need_reconnect(connection, connection_lifetime, connection_idle_time)) {
        try {
            connection.reconnect();
        } catch (const Error &) {
            // Failed to reconnect, return it to the pool, and retry latter.
            LOG_WARN<<"Failed to reconnect,retry";
            queueInLoop_try_reconnction(std::move(connection));
            throw;
        }
    }
    return connection;
}



void ConnectionPool::queueInLoop_try_reconnction(Connection conn){
    std::list<Connection>::iterator it;
    {
        std::unique_lock<std::mutex>unique_lock(_reconneciotn_list_mutex);
        
        _reconneciotn_list_Sptr->push_back(std::move(conn));
        it=--_reconneciotn_list_Sptr->end();
    }
    _recoonction_loop->queueInLoop(
        std::bind(
            &ConnectionPool::try_reconnction,this,it
        )
    );
}

constexpr int MAXRECONNECTIONRETRYTIME=10;

void ConnectionPool::try_reconnction(std::list<Connection>::iterator connection){
    try{
        connection->reconnect();
        release(std::move(*connection));
        {
            std::unique_lock<std::mutex>lock(_reconneciotn_list_mutex);
            _reconneciotn_list_Sptr->erase(connection);
        }
    }catch(const Error&){
        if(connection->reconnectFailedTimes<MAXRECONNECTIONRETRYTIME){
            connection->reconnectFailedTimes++;
        }
        LOG_WARN<<"Failed to reconnect,"<<connection->getConnecitonINFO()<<"retry after:2^"<<connection->reconnectFailedTimes;
        
        _recoonction_loop->runAfter((1<<connection->reconnectFailedTimes),
        std::bind(
            &ConnectionPool::try_reconnction,this,connection
            )
        );
    }
}

ConnectionOptions ConnectionPool::connection_options() {
    std::lock_guard<std::mutex> lock(_mutex);

    return _opts;
}

void ConnectionPool::release(Connection connection) {
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _pool.push_back(std::move(connection));
    }

    _cv.notify_one();
}

Connection ConnectionPool::create() {
    std::unique_lock<std::mutex> lock(_mutex);

    auto opts = _opts;

    return Connection(opts);
}

ConnectionPool ConnectionPool::clone() {
    std::unique_lock<std::mutex> lock(_mutex);

    auto opts = _opts;
    auto pool_opts = _pool_opts;

    return ConnectionPool(pool_opts, opts);

}



Connection ConnectionPool::_fetch(std::unique_lock<std::mutex> &lock) {
    if (_pool.empty()) {
        if (_used_connections == _pool_opts.size) {
            _wait_for_connection(lock);
        } else {
            ++_used_connections;

            // Lazily create a new (broken) connection to avoid connecting with lock.
            return Connection(_opts, Connection::Dummy{});
        }
    }

    // _pool is NOT empty.
    return _fetch();
}

Connection ConnectionPool::_fetch() {
    assert(!_pool.empty());

    auto connection = std::move(_pool.front());
    _pool.pop_front();

    return connection;
}

void ConnectionPool::_wait_for_connection(std::unique_lock<std::mutex> &lock) {
    auto timeout = _pool_opts.wait_timeout;
    if (timeout > std::chrono::milliseconds(0)) {
        // Wait until _pool is no longer empty or timeout.
        if (!_cv.wait_for(lock,
                    timeout,
                    [this] { return !(this->_pool).empty(); })) {
            throw Error("Failed to fetch a connection in "
                    + std::to_string(timeout.count()) + " milliseconds");
        }
    } else {
        // Wait forever.
        _cv.wait(lock, [this] { return !(this->_pool).empty(); });
    }
}

bool ConnectionPool::_need_reconnect(const Connection &connection,
                                    const std::chrono::milliseconds &connection_lifetime,
                                    const std::chrono::milliseconds &connection_idle_time) const {
    if (connection.broken()) {
        return true;
    }

    auto now = std::chrono::steady_clock::now();
    if (connection_lifetime > std::chrono::milliseconds(0)) {
        if (now - connection.create_time() > connection_lifetime) {
            return true;
        }
    }

    if (connection_idle_time > std::chrono::milliseconds(0)) {
        if (now - connection.last_active() > connection_idle_time) {
            return true;
        }
    }

    return false;
}

}

}
