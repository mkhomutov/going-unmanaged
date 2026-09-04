// session.h - the seam as a template parameter. Chapter 28 promised this:
// a dependency swapped at compile time, with no virtual call and no
// interface class, so the same Session compiles once against the vendor's
// SDK and once against a recording double.
//
// Quoted VERBATIM in Chapter 41 (book/41-templates-you-will-write.md) below
// this banner: editing it means editing the chapter in the same commit.
#pragma once
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

// The one metaprogramming trick worth owning before concepts: the detection
// idiom. `HasSdkShape<T>` is true when T offers the shape a Session needs,
// and the static_assert below refuses a half-shaped policy at the class -
// where, without it, a policy missing Poll compiles clean until the first
// Pump(), because a member of a class template is only compiled when used.
template <class T, class = void>
struct HasSdkShape : std::false_type {};

template <class T>
struct HasSdkShape<T, std::void_t<
    decltype(std::declval<typename T::Handle>() != typename T::Handle{}),
    std::enable_if_t<std::is_convertible_v<
        decltype(T::Open(std::declval<const char*>())), typename T::Handle>>,
    decltype(T::Close(std::declval<typename T::Handle>())),
    decltype(T::Poll(std::declval<typename T::Handle>(),
                     std::declval<std::function<void(int)>&>()))>> : std::true_type {};

template <class Sdk>
class Session {
    static_assert(HasSdkShape<Sdk>::value,
                  "Sdk policy needs: Handle (default-constructible, !=), "
                  "Open(const char*) -> Handle, Close(Handle), "
                  "Poll(Handle, std::function<void(int)>&)");
public:
    using Handle = typename Sdk::Handle;

    explicit Session(const char* name) : handle_(Sdk::Open(name)) {}
    ~Session() { Sdk::Close(handle_); }
    Session(const Session&) = delete;              // owns a handle: Chapter 6's rule
    Session& operator=(const Session&) = delete;
    Session(Session&& other) noexcept
        : handle_(std::exchange(other.handle_, Handle{})),
          samples_(std::move(other.samples_)) {}
    Session& operator=(Session&&) = delete;

    bool IsOpen() const { return handle_ != Handle{}; }

    // Drain the device: every sample the policy delivers lands in samples_.
    // A moved-from Session owns nothing and pumps nothing - the same answer
    // from both policies, rather than one policy's error and the other's throw.
    std::size_t Pump() {
        if (!IsOpen()) return samples_.size();
        std::function<void(int)> sink = [this](int s) { samples_.push_back(s); };
        Sdk::Poll(handle_, sink);
        return samples_.size();
    }
    const std::vector<int>& Samples() const { return samples_; }
    Handle Raw() const { return handle_; }        // for the SDK's own test hooks only

private:
    Handle handle_{};
    std::vector<int> samples_;
};
