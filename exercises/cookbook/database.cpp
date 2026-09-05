// Appendix F, Recipe 42 - open a local database and run a query.
//
// Db, Statement, Transaction and the functions below are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding, and its
// oracle is an in-memory database - no file, no network, nothing outside
// the run. Every claim the recipe makes has a line here that fails without
// it (each was run as a mutant): the values a query returns, in ORDER BY's
// order and not insertion order; the step-code sequence (100, 100, 101: two
// successes that are not zero); a bound TEMPORARY read back after it died,
// so SQLITE_TRANSIENT is load-bearing and SQLITE_STATIC is a use-after-free
// ASan names; a transaction abandoned by a throw rolled back, and a COMMIT
// that fails (a deferred foreign key) rolled back by the destructor, so the
// order of commit()'s two statements matters; a failed open judged by
// SQLite's own memory counter, so "own it BEFORE checking" is not a comment;
// and - the load-bearing one - close_database returning SQLITE_OK at the
// end, which it does ONLY when every statement was finalized first: the
// close code is this recipe's leak detector, the job FakeSdk_LiveAllocations
// did in Chapter 17. Delete Statement's destructor and that assertion is the
// one that fails.
//
// This TU is behind a probe: build_all.sh locates sqlite3 through
// pkg-config and links it from the system (CLAUDE.md invariant 5), and CI
// passes --require-sqlite.
#include <sqlite3.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Recipe 42 - SqliteConnection, SqliteCommand, ExecuteReader: the C API underneath
using Db = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;   // Recipe 7's shape, again

Db open_database(const std::string& path) {                        // ":memory:" is a database too
    sqlite3* raw = nullptr;
    const int rc = sqlite3_open_v2(path.c_str(), &raw, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    Db db(raw, &sqlite3_close);                                     // own it BEFORE checking: a failed open still allocates
    if (rc != SQLITE_OK) {
        throw std::runtime_error("cannot open " + path + ": " + sqlite3_errmsg(raw));
    }
    return db;
}

// A prepared statement: SqliteCommand with its parameters, owned so that
// finalize runs on every path - and it must, because a database with a live
// statement refuses to close.
class Statement {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("prepare: ") + sqlite3_errmsg(db));
        }
    }
    ~Statement() { sqlite3_finalize(stmt_); }                      // null-safe by the SDK's contract
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    void bind(int index, int value) { check(sqlite3_bind_int(stmt_, index, value)); }
    void bind(int index, const std::string& value) {
        // SQLITE_TRANSIENT: copy the bytes now. SQLITE_STATIC would be a loan
        // (Appendix H) that must outlive every step - the recipe does not
        // make that promise on the caller's behalf.
        check(sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT));
    }

    // One row, or done. The two codes are 100 and 101: successes that are not
    // zero, which is Chapter 8's "usually zero is not a contract" in production.
    bool step() {
        const int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw std::runtime_error(std::string("step: ") + sqlite3_errmsg(db_));   // SQLITE_BUSY too: one connection, so
                                                                                // busy IS exceptional here - see the Why
    }
    int column_int(int i) const { return sqlite3_column_int(stmt_, i); }
    std::string column_text(int i) const {
        // A LOAN (Chapter 33): valid until the next step, reset or finalize.
        // Copied out on the spot, so no caller keeps a pointer into the row.
        const unsigned char* text = sqlite3_column_text(stmt_, i);
        return text ? reinterpret_cast<const char*>(text) : "";   // NULL column: the one null there is
    }
    void reset() { check(sqlite3_reset(stmt_)); }                 // reuse the plan - and rebind EVERY parameter:
                                                                  // a reset keeps the old bindings

private:
    void check(int rc) const {
        if (rc != SQLITE_OK) throw std::runtime_error(std::string("sqlite: ") + sqlite3_errmsg(db_));
    }
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

// One statement with no rows to read: CREATE, INSERT, BEGIN.
void execute(sqlite3* db, const char* sql) {
    Statement s(db, sql);
    while (s.step()) {}
}

// A transaction that rolls back unless told otherwise: Chapter 1's shape
// over Chapter 8's unwinding, so a throw between BEGIN and commit() leaves
// the database as it was.
class Transaction {
public:
    explicit Transaction(sqlite3* db) : db_(db) { execute(db_, "BEGIN"); }
    ~Transaction() {
        if (!committed_) sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);   // no throw in a destructor
    }
    void commit() { execute(db_, "COMMIT"); committed_ = true; }
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

private:
    sqlite3* db_;
    bool committed_ = false;
};

// The verdict a deleter cannot report: a unique_ptr's deleter returns
// nothing, so the shutdown path takes the handle back and closes it by hand.
// SQLITE_BUSY here is a statement nobody finalized - log it; the handle stays
// open, which is the leak made visible rather than the leak made worse.
int close_database(Db db) {
    return sqlite3_close(db.release());
}

struct Reading {
    int sensor;
    std::string unit;
};

// SELECT with a parameter, the rows copied out row by row.
std::vector<Reading> readings_above(sqlite3* db, int threshold) {
    Statement q(db, "SELECT sensor, unit FROM readings WHERE sensor > ?1 ORDER BY sensor");
    q.bind(1, threshold);
    std::vector<Reading> out;
    while (q.step()) {
        out.push_back({q.column_int(0), q.column_text(1)});
    }
    return out;
}

// SQLite reports API misuse - a close of a handle already closed, a bind on
// a statement mid-step - through its log hook, and returns SQLITE_MISUSE to
// a caller that may be a deleter discarding it. The harness counts them: a
// double close reads a freed block inside a library no sanitizer
// instruments, so this hook is the only judge that sees it.
static void count_misuse(void* counter, int code, const char*) {
    if (code == SQLITE_MISUSE) {
        ++*static_cast<int*>(counter);
    }
}

int main() {
    // Two of SQLite's own instruments, switched on before its first call:
    // the log hook above, and the allocation counter (Apple's build ships it
    // off) - the judge for "a failed open still allocates, so own the handle
    // before checking": two opens of a directory throw, and nothing of
    // theirs stays allocated.
    int misuses = 0;
    sqlite3_config(SQLITE_CONFIG_LOG, &count_misuse, &misuses);
    sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 1);
    const long long before_opens = sqlite3_memory_used();
    for (int i = 0; i < 2; ++i) {
        bool refused = false;
        try {
            open_database("/");               // a directory is not a database, on every platform
        } catch (const std::runtime_error& e) {
            refused = std::string(e.what()).find("cannot open /") == 0;
        }
        assert(refused);
    }
    assert(sqlite3_memory_used() == before_opens);   // check-then-wrap would leak the failed handle

    Db db = open_database(":memory:");
    execute(db.get(), "CREATE TABLE readings (sensor INTEGER NOT NULL, unit TEXT)");

    // One prepared statement, reset and rebound three times: the plan is
    // compiled once, which is what SqliteCommand.Prepare was for.
    {
        Statement ins(db.get(), "INSERT INTO readings (sensor, unit) VALUES (?1, ?2)");
        const std::vector<Reading> rows = {{3, "rpm"}, {1, "C"}, {2, "kPa"}};   // NOT in order: ORDER BY must earn it
        for (const Reading& r : rows) {
            ins.bind(1, r.sensor);
            ins.bind(2, r.unit);
            assert(!ins.step());          // an INSERT has no row: DONE on the first step
            ins.reset();
        }
        // A TEMPORARY bound and dead before the step: SQLITE_TRANSIENT copied
        // it, SQLITE_STATIC would read freed memory here - and ASan says so.
        ins.bind(1, 4);
        ins.bind(2, std::string(40, 'u'));
        assert(!ins.step());
        ins.reset();
    }
    assert(readings_above(db.get(), 3).at(0).unit == std::string(40, 'u'));

    // The step-code sequence itself: 100, 100, 101 - and never 0.
    {
        Statement q(db.get(), "SELECT sensor FROM readings WHERE sensor >= 2 ORDER BY sensor");
        sqlite3_stmt* raw = nullptr;
        const int prepared = sqlite3_prepare_v2(db.get(), "SELECT sensor FROM readings WHERE sensor >= 2 AND sensor < 4",
                                                -1, &raw, nullptr);
        assert(prepared == SQLITE_OK);
        assert(sqlite3_step(raw) == SQLITE_ROW && SQLITE_ROW == 100);
        assert(sqlite3_step(raw) == SQLITE_ROW);
        assert(sqlite3_step(raw) == SQLITE_DONE && SQLITE_DONE == 101);
        const int finalized = sqlite3_finalize(raw);
        assert(finalized == SQLITE_OK);
        assert(q.step() && q.column_int(0) == 2);
    }

    // The values, through the recipe's query.
    const std::vector<Reading> above = readings_above(db.get(), 1);
    assert(above.size() == 3);
    assert(above[0].sensor == 2 && above[0].unit == "kPa");       // inserted third, returned first: ORDER BY
    assert(above[1].sensor == 3 && above[1].unit == "rpm");
    assert(above[2].sensor == 4 && above[2].unit == std::string(40, 'u'));

    // A NULL column reads as the empty string, not as a crash.
    execute(db.get(), "INSERT INTO readings (sensor, unit) VALUES (5, NULL)");
    assert(readings_above(db.get(), 4).at(0).unit.empty());

    // A transaction abandoned by a throw rolls back: the row count is what
    // it was before BEGIN.
    bool threw = false;
    try {
        Transaction tx(db.get());
        execute(db.get(), "INSERT INTO readings (sensor, unit) VALUES (99, 'lost')");
        throw std::runtime_error("something between BEGIN and COMMIT failed");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
    assert(readings_above(db.get(), 50).empty());
    {
        Transaction tx(db.get());
        execute(db.get(), "INSERT INTO readings (sensor, unit) VALUES (6, 'V')");
        tx.commit();
    }
    assert(readings_above(db.get(), 5).size() == 1);

    // A COMMIT that fails: a deferred foreign key is checked at commit, so
    // the throw comes out of commit() itself, and the destructor's rollback
    // must still run - which it does only if committed_ is set AFTER the
    // COMMIT succeeded. The connection is back in autocommit afterwards.
    execute(db.get(), "PRAGMA foreign_keys = ON");
    execute(db.get(), "CREATE TABLE sensors (id INTEGER PRIMARY KEY)");
    execute(db.get(), "CREATE TABLE calibrations (sensor INTEGER REFERENCES sensors(id) DEFERRABLE INITIALLY DEFERRED)");
    threw = false;
    try {
        Transaction tx(db.get());
        execute(db.get(), "INSERT INTO calibrations (sensor) VALUES (404)");   // no such sensor: deferred
        tx.commit();                                                           // ...until here
    } catch (const std::runtime_error& e) {
        threw = std::string(e.what()).find("FOREIGN KEY") != std::string::npos;
    }
    assert(threw);
    assert(sqlite3_get_autocommit(db.get()) != 0);   // rolled back by the destructor: no transaction left open
    assert(readings_above(db.get(), 0).size() == 6);

    // The leak detector: close succeeds only when every statement is
    // finalized. Every Statement above died at its closing brace; a
    // Statement whose destructor forgot finalize would make this SQLITE_BUSY.
    // And the Db is empty afterwards - a second close through the deleter
    // would be SQLITE_MISUSE, returned into a deleter that discards it, from
    // inside a library no sanitizer instruments.
    const int closed = close_database(std::move(db));
    assert(closed == SQLITE_OK);
    assert(!db);
    assert(misuses == 0);            // a close_database that closed twice logged SQLITE_MISUSE here

    std::cout << "sqlite ok: " << sqlite3_libversion() << ", 6 rows, two rollbacks, close returned " << closed << "\n";
    return 0;
}
