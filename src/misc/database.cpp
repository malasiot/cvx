#include <cvx/misc/database.hpp>

#include <cstring>
#include <sstream>

using namespace std ;

namespace cvx {

namespace SQLite3 {

NullType Nil ;

Connection::Connection(): handle_(NULL) {

}

void Connection::open(const std::string &db, int flags) {
    if ( sqlite3_open_v2(db.c_str(), &handle_, flags, NULL)  != SQLITE_OK )
         throw Exception("Could not open database");
}

 void Connection::close() {
     check() ;

     if( sqlite3_close(handle_) != SQLITE_OK )
         throw Exception(sqlite3_errmsg(handle_));

     handle_ = nullptr ;
 }

void Connection::exec(const string &sql, ...)
{
    va_list arguments ;
    va_start(arguments, sql);

    char *sql_ = sqlite3_vmprintf(sql.c_str(), arguments) ;

    char *err_msg ;
    if ( sqlite3_exec(handle_, sql_, NULL, NULL, &err_msg) != SQLITE_OK )
    {
        string msg(err_msg) ;
        sqlite3_free(err_msg) ;

        throw Exception(msg) ;

    }

    sqlite3_free(sql_) ;

    va_end(arguments);
}

void Connection::check() {
     if( !handle_ )
         throw Exception("Database is not open.");
}

Connection::~Connection()
{
    close() ;
}

Statement Connection::prepareStatement(const string &sql) {
    return Statement(*this, sql) ;
}


Query Connection::prepareQuery(const string &sql) {
    return Query(*this, sql) ;
}

Transaction Connection::transaction() {
    return Transaction(*this) ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Exception::Exception(sqlite3 *handle): std::runtime_error(sqlite3_errmsg(handle)) {}
Exception::Exception(const std::string &msg): std::runtime_error(msg) {}

Statement::Statement(Connection &con, const string &sql) {
    con.check() ;

    const char * tail = 0;
    sqlite3_stmt *handle ;
    if ( sqlite3_prepare_v2(con.handle(), sql.c_str(), -1, &handle ,&tail) != SQLITE_OK )
          throw Exception(con.handle()) ;
    handle_.reset(handle, sqlite3_finalize) ;

}

Statement::~Statement() {
}

void Statement::clear() {
     check();
     if ( sqlite3_reset(handle_.get()) != SQLITE_OK ) throwStmtException();
}

void Statement::finalize() {
    check();
    if ( sqlite3_finalize(handle_.get()) != SQLITE_OK ) throwStmtException();
    handle_ = 0;
}

void Statement::throwStmtException() {
    throw SQLite3::Exception(sqlite3_db_handle(handle_.get())) ;
}

void Statement::check() const {
    if ( !handle_ )
        throw Exception("Statement has not been compiled.");
}

/* returns true if the command returned data */

bool Statement::step() {
    check() ;

    switch( sqlite3_step(handle_.get()) ) {
        case SQLITE_ROW:
            return true;
        case SQLITE_DONE:
            return false;
        default:
            throwStmtException();
    }
    return false;
}

Statement &Statement::bind(int idx, const NullType &) {
    check();
    if ( sqlite3_bind_null(handle_.get(), idx) != SQLITE_OK ) throwStmtException();
    return *this ;
}


Statement &Statement::bind(int idx, unsigned char v) {
    check();
    if ( sqlite3_bind_int(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, char v) {
    check();
    if ( sqlite3_bind_int(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, int v) {
    check();
    if ( sqlite3_bind_int(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, unsigned int v) {
    check();
    if ( sqlite3_bind_int(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, unsigned short int v) {
    check();
    if ( sqlite3_bind_int(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, short int v) {
    check();
    if ( sqlite3_bind_int(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, long int v) {
    check();
    if ( sqlite3_bind_int64(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, unsigned long int v) {
    check();
    if ( sqlite3_bind_int64(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, long long int v){
    check();
    if ( sqlite3_bind_int64(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, unsigned long long int v){
    check();
    if ( sqlite3_bind_int64(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, double v){
    check() ;
    if ( sqlite3_bind_double(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, float v){
    check() ;
    if ( sqlite3_bind_double(handle_.get(), idx, v) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, const string &v){
    check() ;
    if ( sqlite3_bind_text(handle_.get(), idx, v.c_str(), int(v.size()), SQLITE_TRANSIENT ) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, const Blob &blob){
    check() ;
    if ( sqlite3_bind_blob(handle_.get(), idx, blob.data(), blob.size(), SQLITE_TRANSIENT ) != SQLITE_OK ) throwStmtException();
    return *this ;
}

Statement &Statement::bind(int idx, const char *v){
    check() ;
    if ( sqlite3_bind_text(handle_.get(), idx, v, strlen(v), SQLITE_TRANSIENT ) != SQLITE_OK ) throwStmtException();
    return *this ;
}


////////////////////////////////////////////////////////////////////////////////////////
Query::Query(Connection &con, const string &sql):
    Statement(con, sql) {

    int num_fields = sqlite3_column_count(handle_.get());

    for( int index = 0; index < num_fields; index++ ) {
        const char* field_name = sqlite3_column_name(handle_.get(), index);
        field_map_[field_name] = index ;
    }
}

int Query::columnIdx(const string &name) const {
    auto it = field_map_.find(name) ;
    if ( it != field_map_.end() ) return (*it).second ;
    else throw Exception("There is no column '" + name + "' in the results table") ;
    return 0 ;
}

bool Query::hasColumn(const std::string &name) const {
    return field_map_.find(name) != field_map_.end() ;
}

QueryResult Query::exec() {
    return QueryResult(*this) ;
}


///////////////////////////////////////////////////////////////////////////////////////

QueryResult::QueryResult(Query &cmd): cmd_(cmd) {
  //  next() ;
}

int QueryResult::columns() const {
    cmd_.check() ;
    return ( sqlite3_data_count(cmd_.handle()) ) ;
}

const char *QueryResult::columnName(int idx) const {
   cmd_.check() ;
   const char *name = sqlite3_column_name(cmd_.handle(), idx)  ;
   if ( name == nullptr ) {
       stringstream error ;
       error << "There is no column with index " << idx ;
       throw Exception(error.str()) ;
   }
   else return name ;
}

int QueryResult::columnIdx(const string &name) const {
    cmd_.check() ;
    return cmd_.columnIdx(name) ;
}

bool QueryResult::next() {
    empty_ = !cmd_.step() ;
    return !empty_ ;
}

int QueryResult::columnType(int idx) const {
    cmd_.check() ;
    return sqlite3_column_type(cmd_.handle(), idx);
}

int QueryResult::columnBytes(int idx) const {
    cmd_.check() ;
    return sqlite3_column_bytes(cmd_.handle(), idx);
}

template<>
int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int(cmd_.handle(), idx);
}

template<>
unsigned int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int(cmd_.handle(), idx);
}

template<>
short int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int(cmd_.handle(), idx);
}

template<>
unsigned short int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int(cmd_.handle(), idx);
}

template<>
long int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int64(cmd_.handle(), idx);
}

template<>
unsigned long int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int64(cmd_.handle(), idx);
}

template<>
bool QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int(cmd_.handle(), idx);
}

template<>
double QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_double(cmd_.handle(), idx);
}

template<>
float QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_double(cmd_.handle(), idx);
}

template<>
long long int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int64(cmd_.handle(), idx);
}

template<>
unsigned long long int QueryResult::get(int idx) const {
    cmd_.check() ;
    return sqlite3_column_int64(cmd_.handle(), idx);
}

template<>
char const* QueryResult::get(int idx) const {
    return reinterpret_cast<char const*>(sqlite3_column_text(cmd_.handle(), idx));
}

template<>
std::string QueryResult::get(int idx) const {
    const char *res = reinterpret_cast<char const*>(sqlite3_column_text(cmd_.handle(), idx));
    if ( res == nullptr ) return string() ;
    else return res ;
}

template<>
Blob QueryResult::get(int idx) const {
    cmd_.check() ;
    const void *data = sqlite3_column_blob(cmd_.handle(), idx);
    int bytes = sqlite3_column_bytes(cmd_.handle(), idx) ;
    return Blob((const char *)data, bytes) ;
}

bool QueryResult::columnIsNull(int idx) const {
    return ( sqlite3_column_type(cmd_.handle(), idx) == SQLITE_NULL ) ;
}

bool QueryResult::hasColumn(const string &name)
{
    return cmd_.hasColumn(name) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////


Transaction::Transaction(Connection &con): con_(con) {
    char *err_msg ;

    if ( sqlite3_exec (con_.handle(), "BEGIN", NULL, NULL, &err_msg) != SQLITE_OK ) {
        throw Exception(err_msg);
        sqlite3_free (err_msg);
    }
}

void Transaction::commit()
{
    char *err_msg ;

    if ( sqlite3_exec (con_.handle(), "COMMIT", NULL, NULL, &err_msg) != SQLITE_OK ) {
        throw Exception(err_msg);
        sqlite3_free (err_msg);
    }
}

void Transaction::rollback()
{
    char *err_msg ;

    if ( sqlite3_exec (con_.handle(), "ROLLBACK", NULL, NULL, &err_msg) != SQLITE_OK ) {
        throw Exception(err_msg);
        sqlite3_free (err_msg);
    }
}

Column::Column(QueryResult &qr, int idx): qres_(qr), idx_(idx) {}

Column::Column(QueryResult &qr, const string &name): qres_(qr), idx_(qr.columnIdx(name)) {}

}

}
