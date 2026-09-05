// Appendix F, Recipe 42 - open a local database and run a query.
//
// Db, Statement, Transaction and the two functions below are quoted VERBATIM
// in book/F-rosetta-cookbook.md: editing one means editing the appendix in
// the same commit (the testlab discipline). main() is scaffolding, and its
// oracle is an in-memory database - no file, no network, nothing outside
// the run. It asserts the values a query returns, the step-code sequence
// (100, 100, 101: two successes that are not zero), that a transaction
// abandoned by a throw rolled back, and - the load-bearing one - that
// sqlite3_close returns SQLITE_OK at the end, which it does ONLY when every
// statement was finalized first: the close code is this recipe's leak
// detector, the job FakeSdk_LiveAllocations did in Chapter 17. Delete
// Statement's destructor and that assertion is the one that fails.
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
        throw std::runtime_error(std::string("step: ") + sqlite3_errmsg(db_));   // SQLITE_BUSY lands here too
    }
    int column_int(int i) const { return sqlite3_column_int(stmt_, i); }
    std::string column_text(int i) const {
        // A LOAN (Chapter 33): valid until the next step, reset or finalize.
        // Copied out on the spot, so no caller keeps a pointer into the row.
        const unsigned char* text = sqlite3_column_text(stmt_, i);
        return text ? reinterpret_cast<const char*>(text) : "";   // NULL column: the one null there is
    }
    void reset() { check(sqlite3_reset(stmt_)); }                 // reuse the plan: bind again, step again

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

int main() {
    Db db = open_database(":memory:");
    execute(db.get(), "CREATE TABLE readings (sensor INTEGER NOT NULL, unit TEXT)");

    // One prepared statement, reset and rebound three times: the plan is
    // compiled once, which is what SqliteCommand.Prepare was for.
    {
        Statement ins(db.get(), "INSERT INTO readings (sensor, unit) VALUES (?1, ?2)");
        const std::vector<Reading> rows = {{1, "C"}, {2, "kPa"}, {3, "rpm"}};
        for (const Reading& r : rows) {
            ins.bind(1, r.sensor);
            ins.bind(2, r.unit);
            assert(!ins.step());          // an INSERT has no row: DONE on the first step
            ins.reset();
        }
    }

    // The step-code sequence itself: 100, 100, 101 - and never 0.
    {
        Statement q(db.get(), "SELECT sensor FROM readings WHERE sensor >= 2");
        sqlite3_stmt* raw = nullptr;
        assert(sqlite3_prepare_v2(db.get(), "SELECT sensor FROM readings WHERE sensor >= 2", -1, &raw, nullptr) == SQLITE_OK);
        assert(sqlite3_step(raw) == SQLITE_ROW && SQLITE_ROW == 100);
        assert(sqlite3_step(raw) == SQLITE_ROW);
        assert(sqlite3_step(raw) == SQLITE_DONE && SQLITE_DONE == 101);
        assert(sqlite3_finalize(raw) == SQLITE_OK);
        assert(q.step() && q.column_int(0) == 2);
    }

    // The values, through the recipe's query.
    const std::vector<Reading> above = readings_above(db.get(), 1);
    assert(above.size() == 2);
    assert(above[0].sensor == 2 && above[0].unit == "kPa");
    assert(above[1].sensor == 3 && above[1].unit == "rpm");

    // A NULL column reads as the empty string, not as a crash.
    execute(db.get(), "INSERT INTO readings (sensor, unit) VALUES (4, NULL)");
    assert(readings_above(db.get(), 3).at(0).unit.empty());

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
        execute(db.get(), "INSERT INTO readings (sensor, unit) VALUES (5, 'V')");
        tx.commit();
    }
    assert(readings_above(db.get(), 4).size() == 1);

    // The leak detector: close succeeds only when every statement is
    // finalized. Every Statement above died at its closing brace; a
    // Statement whose destructor forgot finalize would make this SQLITE_BUSY.
    sqlite3* raw = db.release();
    const int closed = sqlite3_close(raw);
    assert(closed == SQLITE_OK);

    std::cout << "sqlite ok: " << sqlite3_libversion() << ", 5 rows, one rollback, close returned " << closed << "\n";
    return 0;
}
