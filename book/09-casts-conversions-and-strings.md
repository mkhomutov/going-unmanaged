## Chapter 9 — Casts, Conversions, and Strings

### The four C++ casts

C-style casts like `(int)x` work but are a red flag in reviews — they can silently do any of four different things. Modern C++ names the intent:

| Cast | Purpose | C# analogy |
|---|---|---|
| `static_cast<T>(x)` | 'sensible' conversions: numeric, up/down class hierarchy when YOU know the type. No runtime check. | (int)x, explicit conversions |
| `dynamic_cast<T*>(x)` | checked downcast on polymorphic types; nullptr on failure (reference form throws). | as / is |
| `const_cast<T>(x)` | add or REMOVE const. Legitimate ~only for bad legacy APIs. Modifying an originally-const object is UB. | (none) |
| `reinterpret_cast<T>(x)` | reinterpret the bits: pointer-to-integer, unrelated pointer types. Danger zone; serialization/interop only. | unsafe pointer tricks |

```cpp
double d = 3.7;
int i = static_cast<int>(d);              // explicit, searchable, intentional

Shape* s = GetShape();
if (auto* c = dynamic_cast<Circle*>(s))   // checked downcast (needs vtable)
    c->radius = 5;

Derived* d2 = static_cast<Derived*>(s);   // UNchecked downcast: fast, but if
                                          // s isn't really a Derived -> UB
```

> **Key principle:** "I use static_cast for conversions I can prove, dynamic_cast when I must query at runtime — and I treat const_cast or reinterpret_cast in a code review as a question mark."

### Strings and encodings

C# strings are immutable, interned, UTF-16 objects. **std::string is a mutable byte buffer with no encoding awareness** — it stores bytes; whether they're ASCII, UTF-8, or garbage is your problem. The modern convention: keep std::string as UTF-8 everywhere.

```cpp
std::string s = "hello";
s += " world";              // mutable in place - no C# immutability
s[0] = 'H';                 // legal!
s.size();                   // BYTES, not characters - differs from what
                            // you'd expect with non-ASCII text!

const char* c = s.c_str();  // borrow a C-style pointer (valid only while
                            // s lives and is unmodified - dangling trap)
std::string_view v = s;     // non-owning view (Chapter 10)
```

Comparison is by value out of the box (`s1 == s2` compares contents), formatting is `std::format` (C++20, like string interpolation) or the classic streams.

The SDK reality — multiple string types in one function. Most large SDKs ship their own string class (Qt's `QString`, Windows' `BSTR`/`std::wstring`, many vendor "UniString" types), typically UTF-16 like C# strings internally. Conversions at the boundary are daily work:

```cpp
VendorString title("Wall label");          // vendor string: UTF-16 inside
// vendor <-> std::string conversions, encoding NAMED explicitly:
std::string utf8 = title.ToUtf8();
VendorString back = VendorString::FromUtf8(utf8);
```

> **Trap:** Encoding bugs are THE classic plug-in pitfall: user file and project names with non-ASCII characters (German umlauts, Cyrillic, CJK...) silently corrupt if you treat UTF-16 vendor strings as byte strings. Always convert explicitly with the encoding named.

---

