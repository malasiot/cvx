#ifndef SQLITE3_DATABASE_HPP
#define SQLITE3_DATABASE_HPP

// Simple C++ API for SQlite database

/*
    SQLite::Connection con ;
    con.open("/home/malasiot/tmp/poses.sqlite", SQLITE_OPEN_READONLY) ;

    // make select statement and get query cursor
    auto r = con.query("SELECT * FROM poses WHERE id > ?", 10) ;

    // alternatively  bind parameters sequentially
    Query q(con, "SELECT * FROM poses WHERE id > ?") ;
    q.bind(0, 10) ;
    auto r = q.exec() ;

    // iterate over rows
    while ( r.next() ) {
        // get column value
        cout << r.get<string>(1) << ' ' << r[2].as<string>() << endl ;

        // or get all column values
        string b, j, p ;
        int i ;
        r.into(b, i, j, p) ;
    }

    // or iterate using row accessor

    for( const auto row: r ) {
        // access column value at index 0 as string
        cout << row[0].as<string>() << endl ;
    }

    con.execute("INSERT INTO table (id, type) VALUES (?, ?)", 1, "int") ;
    Statement s(INSERT INTO table (id, type) VALUES (?, ?)") ;

    for (auto v: values) {
        s.bindAll(v.id, v.type) ;
        s.exec() ;
        // or s(v.id, v.type) ;

        s.reset() ; // to clear bindings
    }

*/

#include <sqlite3.h>
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <cstdint>
#include <memory>
#include <cassert>

namespace cvx {

namespace SQLite3 {

class Statement ;
class QueryResult ;
class Exception ;
class NullType;
class Connection ;
class Blob ;

class NullType {} ;

extern NullType Nil;

class Query ;
class Transaction ;
class QueryResult ;



class Exception: public std::runtime_error
{
public:
    Exception(const std::string &msg) ;
    Exception(sqlite3 *handle) ;
};

/**
 * @brief The Statement class is a wrapper for prepared statements
 */

class Statement
{
public:

    /**
     * @brief make a new prepared statement from the SQL string and the current connection
     */
    Statement(Connection& con, const std::string & sql) ;

    template<typename ...Args>
    Statement(Connection& con, const std::string & sql, Args... args): Statement(con, sql) {
        bindAll(args...) ;
    }

    ~Statement();

    Statement() = default ;


    /** \brief clear is used if you'd like to reuse a command object
    */
    void clear();

    /** \brief Bind value with corresponding placeholder index
     */

    Statement &bind(int idx, const NullType &) ;
    Statement &bind(int idx, unsigned char v) ;
    Statement &bind(int idx, char v) ;
    Statement &bind(int idx, unsigned short v) ;
    Statement &bind(int idx, short v) ;
    Statement &bind(int idx, unsigned long v) ;
    Statement &bind(int idx, long v) ;
    Statement &bind(int idx, unsigned int v) ;
    Statement &bind(int idx, int v) ;
    Statement &bind(int idx, unsigned long long int v) ;
    Statement &bind(int idx, long long int v) ;
    Statement &bind(int idx, double v) ;
    Statement &bind(int idx, float v) ;
    Statement &bind(int idx, const std::string &v) ;
    Statement &bind(int idx, const Blob &blob) ;
    Statement &bind(int idx, const char *str) ;

    // bind value to placeholder index

    template <class T>
    Statement &bind(int idx, T v) {
       bind(idx, v) ;
       return *this ;
    }

    /** \brief Bind value with corresponding placeholder parameter name
     */

    template <class T>
    Statement &bind(const std::string &name, const T &p) {
        int idx = sqlite3_bind_parameter_index(handle_.get(), name.c_str() );
        if ( idx ) return bind(idx, p) ;
        else throw Exception(name + " is not a valid statement placeholder") ;
    }


    // bind all values sequentially

    template <typename ... Args>
    Statement &bindAll(Args&& ... args) {
        return bindm((uint)1, args...) ;
    }

    // bind values and execute statement

    template<typename ...Args>
    void operator()(Args&&... args) {
       bindAll(args...) ;
       exec() ;
    }


    sqlite3_stmt *handle() const { return handle_.get() ; }

    void exec() {
        check() ;
        step() ;
    }

protected:

    void check() const ;
    bool step();

protected:

    Statement &bindm(uint idx) {
        return *this ;
    }

    template <typename T>
    Statement &bindm(uint idx, T&& t) {
        return bind(idx, t) ;
    }

    template <typename First, typename ... Args>
    Statement &bindm(uint idx, First f, Args&& ... args) {
        return bind(idx++, f).bindm(idx, args...) ;
    }

private:

    void prepare();
    void finalize();

    void throwStmtException() ;

protected:

    friend class QueryResult ;

    std::shared_ptr<sqlite3_stmt> handle_;

};


class Query: public Statement {
public:
    Query(Connection &con, const std::string &sql) ;

    template<typename ...Args>
    Query(Connection& con, const std::string & sql, Args... args): Query(con, sql) {
        bindm((uint)1, args...) ;
    }

    QueryResult exec() ;


private:
    friend class QueryResult ;

    int columnIdx(const std::string &name) const ;
    bool hasColumn(const std::string &name) const;

    std::map<std::string, int> field_map_ ;
};

// Wraps pointer to buffer and its size. Memory management is external
class Blob {
public:

    Blob(const char *data, uint32_t sz): size_(sz), data_(data) {}

    const char *data() const { return data_ ; }
    uint32_t size() const { return size_ ; }

private:
    const char *data_ = nullptr;
    uint32_t size_ = 0 ;
};

class QueryResult ;

class Column {
public:
    template <class T> T as() const ;

private:

    friend class QueryResult ;

    Column(QueryResult &qr, int idx);
    Column(QueryResult &qr, const std::string &name);

    QueryResult &qres_ ;
    int idx_ ;
};
class QueryResult {

public:

    QueryResult(QueryResult &&other) = default ;
    QueryResult(QueryResult &other) = delete ;
    QueryResult& operator=(const QueryResult &other) = delete;
    QueryResult& operator=(QueryResult &&other) = default;

    bool isValid() const { return !empty_ ; }
    operator int () { return !empty_ ; }

    bool next() ;

    int columns() const;
    int columnType(int idx) const;
    const char *columnName(int idx) const ;
    int columnIdx(const std::string &name) const ;
    int columnBytes(int idx) const ;
    bool columnIsNull(int idx) const ;
    bool hasColumn(const std::string &name) ;

    template<class T>
    T get(int idx) const ;

    template <class T>
    T get(const std::string &name) const {
        int idx = columnIdx(name) ;
        return get<T>(idx) ;
    }

    template<class T>
    T get(int idx, const T &def) const {
        return ( !columnIsNull(idx) ) ? get<T>(idx) : def ;
    }

    template<class T>
    T get(const std::string &name, const T &def) const {
        int idx = columnIdx(name) ;
        return get<T>(idx, def) ;
    }

    Column operator [] (int idx) { return Column(*this, idx); }
    Column operator [] (const std::string &name) { return Column(*this, name); }

    // fetch column values and store to variables

    template <typename ... Args>
    void into(Args &... args) const {
        readi(0, args...) ;
    }

private:

    friend class Query ;
    friend class Connection ;

    QueryResult(Query &cmd);

private:

    Query cmd_ ;
    bool empty_ ;

    template <typename T>
    void readi(int idx, T &t) const {
        t = get<T>(idx) ;
    }

    template <typename First, typename ... Args>
    void readi(int idx, First &f, Args &... args) const {
        readi(idx, f) ;
        readi(idx+1, args...) ;
    }

} ;



template <class T>
inline T Column::as() const {
    return qres_.get<T>(idx_) ;
}



class Transaction
{
public:

    Transaction(Connection &con_); // the construcctor starts the constructor

    // you should explicitly call commit or rollback to close it
   void commit();
   void rollback();

private:

   Connection &con_ ;
 };

class Connection {

    public:

    explicit Connection();
    ~Connection();

    // open connection to database withe given flags
    void open(const std::string &name, int flags);
    void close() ;

     operator int () { return handle_ != nullptr ; }
    /**
     * @brief Helper for executing an sql statement, including a colon separated list of statements
     * @param sql Format string similar to printf. Use %q for arguments that need quoting (see sqlite3_mprintf documentation)
     */
    void exec(const std::string &sql, ...) ;

    sqlite3_int64 last_insert_rowid() {
        return sqlite3_last_insert_rowid(handle_);
    }

    int changes() {
        return sqlite3_changes(handle_);
    }

    sqlite3 *handle() { return handle_ ; }

    Statement prepareStatement(const std::string &sql) ;
    Query prepareQuery(const std::string &sql) ;

    // helper for creating a connection and binding parameters
    template<typename ...Args>
    QueryResult query(const std::string & sql, Args... args) {
       return Query(*this, sql, args...).exec() ;
    }

    template<typename ...Args>
    void execute(const std::string &sql, Args... args) {
        Statement(*this, sql, args...).exec() ;
    }

    Transaction transaction() ;

protected:

    friend class Statement ;
    friend class Transaction ;

    void check() ;

    sqlite3 *handle_ ;
};



} // namespace SQLite3

} // namespace cvx


#endif
