# AGENTS.md

## Build & Run

```sh
gcc -o termo termo.c -Wall -Wextra
./termo
```

Requires `palavras.txt` in the same directory as the executable (one word per line, exactly 5 letters each).

## Architecture

Single-file C program (`termo.c`). No external dependencies beyond libc.

Key functions:
- `char_to_code()` — maps UTF-8 char to unique integer (handles accented chars: á≠ã≠ç)
- `utf8_len()` / `utf8_valid()` — UTF-8 byte counting and validation
- `utf8_upper()` / `to_lower_utf8()` — case conversion for accented Latin chars (0xC3 range)
- `calc_feedback()` — Wordle feedback with correct repeated-letter handling (3-pass: count→exact→misplaced)
- `load_words()` — reads dict, validates UTF-8 + 5-letter length, skips invalid lines
- `is_valid_guess()` — linear scan against loaded dictionary

## Key Gotchas

- **UTF-8 indexing**: never use `strlen()` for character count; use `utf8_len()`. Accented chars are 2 bytes.
- **Feedback array indexing**: `feedback[]` is indexed by *letter position* (0-4), not byte position.
- **Dictionary validation**: `load_words()` silently skips words with wrong length or bad UTF-8. Check stderr on startup for skipped entries.
- **Lowercase normalization**: input is lowercased via `to_lower_utf8()` before comparison. Dictionary words must be lowercase.
- **No build system**: just raw gcc. No Makefile, no cmake.

## palavras.txt

Must contain only valid 5-letter Portuguese words (one per line). Currently ~270 words. Words with wrong length or invalid UTF-8 are silently skipped at load time.
