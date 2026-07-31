# Exercise: The Word Counter (STL fluency end to end)

Full statement, reference solution, and what each part teaches: **Chapter 19** of the book.
Do it COLD — rule: cppreference ONLY. This is the docs-navigation drill. ~60 min.

Write: read a text file (`words_sample.txt` here, or any prose you like); count word
frequencies case-insensitively, stripping punctuation from word edges; print the
top 10 words occurring at least twice, sorted by count descending, ties broken
alphabetically. That one sentence forces: file I/O, `unordered_map` counting, the
map-to-vector transfer, a two-key sort lambda, filtering, and bounded output.

Build: g++ -std=c++20 -Wall -Wextra -fsanitize=address,undefined -g your.cpp -o task
  (C++20 for std::erase_if; or: STD=c++20 scripts/check.sh your.cpp words_sample.txt)
Run:   ./task words_sample.txt
