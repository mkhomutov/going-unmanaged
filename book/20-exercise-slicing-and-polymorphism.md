## Chapter 20 — Exercise: Slicing and Polymorphism

*Trains: Chapter 2 (slicing), Chapter 5 (virtual dispatch, virtual destructors). Time: ~45 min.*

### The task

Build a small `Shape` hierarchy (`Circle`, `Rect`) with a virtual `Area()`. Then, deliberately, do it wrong first: put shapes into a `std::vector<Shape>` and total the areas. Predict what happens *before* compiling. Then fix the design so polymorphism actually works, and prove with output that the right `Area()` runs for each element. Finally: remove `virtual` from the base destructor and explain (then demonstrate under ASan with a heap-allocated derived object owning memory) what breaks.

### The broken version, and what it teaches

```cpp
std::vector<Shape> shapes;          // a vector of BASE OBJECTS
shapes.push_back(Circle(1.0));      // sliced: only the Shape part is stored
```

Two outcomes depending on the base: if `Shape` is abstract (pure virtual `Area`), this **does not compile** — the container cannot hold instances of an abstract class, and the error message is your first taste of template error novels (Chapter 7). If `Shape` is concrete, it compiles and silently stores amputated objects: `radius` is gone, and `shapes[0].Area()` calls `Shape::Area`. Make the base abstract *first* so the compiler catches the design error — that is itself a design lesson: **pure virtual functions turn slicing from a runtime surprise into a compile error.**

### Reference solution (the fixed design)

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
--8<-- "solutions/shapes.cpp"
```

</details>

### The details that carry the weight

**`vector<unique_ptr<Shape>>` is *the* polymorphic container.** Objects live on the heap behind pointers; the vector holds owners. Copying the vector is deleted (unique_ptr), which is honest: a hierarchy of polymorphic objects has no cheap, obvious copy semantics — if you need copying, you design a virtual `Clone()` method, explicitly.

**`virtual ~Shape() = default;` is load-bearing, not boilerplate.** Each `unique_ptr<Shape>` deletes through a `Shape*`. Without the virtual destructor that is undefined behavior (Chapter 5): derived destructors never run, derived members leak. The demonstration to run: give `Circle` a `std::vector<double>` member, remove `virtual` from `~Shape`, and — on a platform with LeakSanitizer (Linux; not macOS/arm64, where a leaking run stays silent, Chapter 31) — ASan's leak report at exit points at the vector's allocation: a leak with one stack, exactly the Finding 10 report shape. On a Mac your evidence is clang's `-Wdelete-non-abstract-non-virtual-dtor` warning at compile time instead.

**`final` on the leaves** documents that the hierarchy ends here and lets the compiler devirtualize calls where it can prove the type.

**`const auto&` in the loop** — copying a `unique_ptr` doesn't compile (deleted), so the wrong loop here fails loudly rather than silently. Notice how the ownership design converts Chapter 2's silent copy trap into a compile error: good types make wrong code not compile.
