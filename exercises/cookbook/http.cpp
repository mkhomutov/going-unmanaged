// Appendix F, Recipe 41 - call an HTTP endpoint.
//
// Easy, HttpResult, append_chunk() and http_get() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding, and its
// oracle needs no network. Two halves: a file:// fixture, the one URL
// scheme with nothing behind it, exercises the write callback (a 200 KB
// fixture arrives in many chunks, so an append that assigned would lose
// all but the last) and the transport's error path (a missing file is
// CURLE_FILE_COULDNT_READ_FILE, and response code 0 because there was no
// server); and a loopback server of forty lines - POSIX sockets, because
// the standard library has none (Chapter 27) - answers with a redirect to
// follow, a 500 whose body is an error page, and a stall the client's
// deadline must cut short. Every wait is bounded: the server closes each
// connection after one canned reply, so a mutant that waits in seconds
// where the recipe waits in milliseconds gets "server returned nothing",
// never a hang. The harness's server is POSIX-only; the cookbook is not
// built by the MSVC job.
//
// This TU is the cookbook's third behind a probe: build_all.sh locates
// libcurl through pkg-config and links it from the system - a dependency
// the repository links and never copies in (Chapter 27's fourth strategy,
// CLAUDE.md invariant 5) - and CI passes --require-curl.
#include <curl/curl.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#if !defined(_WIN32)
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

// Recipe 41 - HttpClient.GetStringAsync, through the C API the ecosystem uses
// (curl_global_init(CURL_GLOBAL_DEFAULT) runs once per process before this,
// on the thread that starts the others - see the Why.)
using Easy = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;   // Recipe 7's shape: the cleanup is the type

// Two verdicts, both kept: the transport's (did the bytes arrive?) and the
// server's (are they the answer?). HttpClient folded them into one
// exception; here each is data, and ok() is the question most callers ask.
struct HttpResult {
    CURLcode transport = CURLE_OK;    // DNS, connect, TLS, timeout: the wire's opinion
    long status = 0;                  // the server's opinion; 0 when there was no server (file://)
    std::string body;
    bool ok() const { return transport == CURLE_OK && status < 400; }
};

// The trampoline (Chapter 18): libcurl calls this with the void* it was
// handed - once per CHUNK, many times per response, never once.
static std::size_t append_chunk(char* data, std::size_t size, std::size_t count, void* userdata) {
    static_cast<std::string*>(userdata)->append(data, size * count);
    return size * count;    // anything less tells libcurl to abort the transfer
}

HttpResult http_get(const std::string& url, std::chrono::milliseconds timeout) {
    Easy easy(curl_easy_init(), &curl_easy_cleanup);
    if (!easy) {
        throw std::runtime_error("curl_easy_init failed");    // the library itself: Chapter 8's event pole
    }
    HttpResult r;
    // setopt's own return is unchecked on purpose: the options below fail
    // only for a build that lacks them, and a URL it cannot parse is
    // refused by perform, where the verdict is read anyway.
    curl_easy_setopt(easy.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, &append_chunk);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &r.body);
    curl_easy_setopt(easy.get(), CURLOPT_FOLLOWLOCATION, 1L);   // HttpClient's default: a 3xx is followed, not returned
    curl_easy_setopt(easy.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count()));   // Recipe 30's hand-off
    r.transport = curl_easy_perform(easy.get());              // blocks: this IS the await, spelled as a call
    curl_easy_getinfo(easy.get(), CURLINFO_RESPONSE_CODE, &r.status);
    return r;
}

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// A file:// URL for a path, with the path percent-encoded: a temp directory
// with a space in it is a malformed URL otherwise. curl's own URL API, and
// Recipe 7's shape once more for the handle it returns.
static std::string file_url(const fs::path& p) {
    std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> u(curl_url(), &curl_url_cleanup);
    assert(curl_url_set(u.get(), CURLUPART_SCHEME, "file", 0) == CURLUE_OK);
    assert(curl_url_set(u.get(), CURLUPART_PATH, p.generic_string().c_str(), CURLU_URLENCODE) == CURLUE_OK);
    char* full = nullptr;
    assert(curl_url_get(u.get(), CURLUPART_URL, &full, 0) == CURLUE_OK);
    std::string url = full;
    curl_free(full);
    return url;
}

// A second, counting callback for the harness only: proves the "once per
// chunk" claim by fetching a fixture larger than one chunk.
static std::size_t count_chunk(char*, std::size_t size, std::size_t count, void* userdata) {
    ++*static_cast<int*>(userdata);
    return size * count;
}

#if !defined(_WIN32)
// The harness's server: one canned reply per accepted connection, in order,
// on a loopback port the kernel picks, then it stops. "stall" means hold the
// connection past the client's deadline and close it - so a client that
// waits in the wrong unit is refused by the close, never left hanging.
class CannedServer {
public:
    explicit CannedServer(std::vector<std::string> replies) : replies_(std::move(replies)) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(fd_ >= 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;                                    // any free port
        assert(::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) == 0);
        socklen_t len = sizeof addr;
        assert(::getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
        port_ = ntohs(addr.sin_port);
        assert(::listen(fd_, 4) == 0);
        worker_ = std::thread([this] {
            for (const std::string& reply : replies_) {
                const int client = ::accept(fd_, nullptr, nullptr);
                if (client < 0) {
                    return;
                }
                char request[4096];
                (void)::read(client, request, sizeof request);    // one GET, unread
                if (reply == "stall") {
                    std::this_thread::sleep_for(600ms);           // past the client's 250 ms, then hang up
                } else {
                    (void)::write(client, reply.data(), reply.size());
                }
                ::close(client);
            }
        });
    }
    ~CannedServer() {
        worker_.join();     // every reply was consumed, or an assert already aborted the process
        ::close(fd_);
    }
    CannedServer(const CannedServer&) = delete;
    CannedServer& operator=(const CannedServer&) = delete;

    std::string url(const char* path) const { return "http://127.0.0.1:" + std::to_string(port_) + path; }

private:
    std::vector<std::string> replies_;
    int fd_ = -1;
    int port_ = 0;
    std::thread worker_;
};

static std::string reply(const char* status_line, const char* body, const char* extra = "") {
    return std::string("HTTP/1.1 ") + status_line + "\r\n" + extra +
           "Content-Length: " + std::to_string(std::string(body).size()) + "\r\nConnection: close\r\n\r\n" + body;
}
#endif

int main() {
    // Once per process, before any thread exists (Chapter 32): in a plug-in
    // this is the init entry point's job, never a static initializer's.
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "curl_global_init failed\n";
        return 1;
    }

    // Per-run names: a second harness beside this one must not read its fixtures.
    const std::string tag = std::to_string(static_cast<long long>(std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path fixture = fs::temp_directory_path() / ("cookbook_http_fixture_" + tag + ".txt");
    const fs::path big = fs::temp_directory_path() / ("cookbook_http_big_" + tag + ".txt");
    const std::string expected = "{\"channels\": 4}\n";
    std::ofstream(fixture, std::ios::binary) << expected;

    // The happy path: the bytes arrive through the callback, unchanged.
    HttpResult got = http_get(file_url(fixture), 2000ms);
    assert(got.transport == CURLE_OK);
    assert(got.status == 0);                     // file://: there was no server to have an opinion
    assert(got.ok());
    assert(got.body == expected);

    // The transport's verdict: a path that is not there is the wire failing,
    // not a status code - and ok() says no.
    HttpResult missing = http_get(file_url(fs::temp_directory_path() / ("cookbook_http_nope_" + tag + ".txt")), 2000ms);
    assert(missing.transport == CURLE_FILE_COULDNT_READ_FILE);
    assert(!missing.ok());
    assert(missing.body.empty());
    assert(std::string(curl_easy_strerror(missing.transport)).find("file") != std::string::npos);

    // Many times per response: a fixture past one chunk (CURL_MAX_WRITE_SIZE,
    // 16 KB by default) reaches the callback in several pieces - counted
    // through a callback of the harness's own, and then fetched through the
    // recipe, whose body must be all of it: an append_chunk that assigned
    // would keep the last piece only.
    const std::string payload(200 * 1024, 'x');
    std::ofstream(big, std::ios::binary) << payload;
    int chunks = 0;
    {
        Easy easy(curl_easy_init(), &curl_easy_cleanup);
        curl_easy_setopt(easy.get(), CURLOPT_URL, file_url(big).c_str());
        curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, &count_chunk);
        curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &chunks);
        assert(curl_easy_perform(easy.get()) == CURLE_OK);
    }
    assert(chunks > 1);
    assert(http_get(file_url(big), 2000ms).body == payload);

#if !defined(_WIN32)
    // The server's verdict, from a server: a redirect followed to its JSON,
    // a 500 whose body is an error page, and a stall the deadline cuts.
    CannedServer server({
        reply("302 Found", "<html>Moved</html>", "Location: /ok\r\n"),
        reply("200 OK", "{\"ok\": 1}", "Content-Type: application/json\r\n"),
        reply("500 Internal Server Error", "<html>Internal Server Error</html>"),
        "stall",
    });

    const HttpResult followed = http_get(server.url("/old"), 2000ms);
    assert(followed.transport == CURLE_OK);
    assert(followed.status == 200);              // HttpClient's AllowAutoRedirect: the 302 was followed
    assert(followed.body == "{\"ok\": 1}");
    assert(followed.ok());

    const HttpResult error_page = http_get(server.url("/broken"), 2000ms);
    assert(error_page.transport == CURLE_OK);    // the bytes arrived...
    assert(error_page.status == 500);            // ...and they are not the answer
    assert(!error_page.ok());
    assert(error_page.body.find("<html>") == 0);

    const HttpResult stalled = http_get(server.url("/slow"), 250ms);
    assert(stalled.transport == CURLE_OPERATION_TIMEDOUT);   // milliseconds: 250 of them, not 250 seconds
    assert(!stalled.ok());
    std::cout << "http ok: " << got.body.size() << " bytes through the callback, the missing file refused as "
              << curl_easy_strerror(missing.transport) << ", " << chunks << " chunks for 200 KB, "
              << "a 302 followed to " << followed.status << ", a " << error_page.status
              << " not ok, a stall cut at 250 ms\n";
#else
    std::cout << "http ok: " << got.body.size() << " bytes through the callback, the missing file refused as "
              << curl_easy_strerror(missing.transport) << ", " << chunks << " chunks for 200 KB "
              << "(no loopback server on this platform)\n";
#endif

    fs::remove(fixture);
    fs::remove(big);
    curl_global_cleanup();
    return 0;
}
