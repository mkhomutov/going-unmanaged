// Appendix F, Recipe 41 - call an HTTP endpoint.
//
// Easy, HttpResult, append_chunk() and http_get() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding, and its
// oracle needs no network: libcurl speaks file:// in every default build,
// so the harness writes a fixture and fetches it - the write callback runs
// the same way it would for https://, the bytes are asserted equal, and a
// missing path is asserted to come back as the transport's verdict
// (CURLE_FILE_COULDNT_READ_FILE) with ok() false. What file:// cannot
// exercise is the SERVER's verdict - CURLINFO_RESPONSE_CODE is 0 for it -
// and the recipe's prose says so rather than the harness pretending.
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

// Recipe 41 - HttpClient.GetStringAsync, through the C API the ecosystem uses
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
    curl_easy_setopt(easy.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, &append_chunk);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &r.body);
    curl_easy_setopt(easy.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count()));   // Recipe 30's hand-off
    r.transport = curl_easy_perform(easy.get());              // blocks: this IS the await, spelled as a call
    curl_easy_getinfo(easy.get(), CURLINFO_RESPONSE_CODE, &r.status);
    return r;
}

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// A second, counting callback for the harness only: proves the "once per
// chunk" claim by fetching a fixture larger than one chunk.
static std::size_t count_chunk(char*, std::size_t size, std::size_t count, void* userdata) {
    ++*static_cast<int*>(userdata);
    return size * count;
}

int main() {
    // Once per process, before any thread exists (Chapter 32): in a plug-in
    // this is the init entry point's job, never a static initializer's.
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "curl_global_init failed\n";
        return 1;
    }

    const fs::path fixture = fs::temp_directory_path() / "cookbook_http_fixture.txt";
    const std::string expected = "{\"channels\": 4}\n";
    std::ofstream(fixture, std::ios::binary) << expected;
    const std::string url = "file://" + fixture.generic_string();

    // The happy path: the bytes arrive through the callback, unchanged.
    HttpResult got = http_get(url, 2000ms);
    assert(got.transport == CURLE_OK);
    assert(got.status == 0);                     // file://: there was no server to have an opinion
    assert(got.ok());
    assert(got.body == expected);

    // The transport's verdict: a path that is not there is the wire failing,
    // not a status code - and ok() says no.
    HttpResult missing = http_get("file://" + (fs::temp_directory_path() / "cookbook_http_nope.txt").generic_string(), 2000ms);
    assert(missing.transport == CURLE_FILE_COULDNT_READ_FILE);
    assert(!missing.ok());
    assert(missing.body.empty());
    assert(std::string(curl_easy_strerror(missing.transport)).find("file") != std::string::npos);

    // The server's verdict is the half the harness cannot reach through
    // file://; what it CAN show is that ok() reads it: a 500 with a clean
    // transport is not ok.
    HttpResult error_page;
    error_page.status = 500;
    error_page.body = "<html>Internal Server Error</html>";
    assert(error_page.transport == CURLE_OK && !error_page.ok());

    // Many times per response: a fixture past one chunk (CURL_MAX_WRITE_SIZE,
    // 16 KB by default) reaches the callback in several pieces.
    const fs::path big = fs::temp_directory_path() / "cookbook_http_big.txt";
    std::ofstream(big, std::ios::binary) << std::string(200 * 1024, 'x');
    int chunks = 0;
    {
        Easy easy(curl_easy_init(), &curl_easy_cleanup);
        curl_easy_setopt(easy.get(), CURLOPT_URL, ("file://" + big.generic_string()).c_str());
        curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, &count_chunk);
        curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &chunks);
        assert(curl_easy_perform(easy.get()) == CURLE_OK);
    }
    assert(chunks > 1);

    fs::remove(fixture);
    fs::remove(big);
    curl_global_cleanup();
    std::cout << "http ok: " << got.body.size() << " bytes through the callback, the missing file refused as "
              << curl_easy_strerror(missing.transport) << ", " << chunks << " chunks for 200 KB\n";
    return 0;
}
