# Translation Reports

Automated translation linter reports for cs2kz-metamod.

## Status

| Metric | Value |
|--------|-------|
| **Linter Errors** | 0 |
| **Linter Warnings** | 25 |
| **Total Languages** | 13 |
| **Total Phrases** | 521 |
| **Phrases Missing Translations** | 441 |
| **Total Missing Entries** | 2036 |
| **Menu Files** | 9 |

## Language Coverage (Phrases)

```
  chi (schinese)       [███████████████████░]  99.6% (519/521)
  de (german)          [███████████████████░]  99.6% (519/521)
  en (english)         [████████████████████] 100.0% (521/521)
  es (spanish)         [███████████░░░░░░░░░]  55.3% (288/521)
  fi (finnish)         [█████████░░░░░░░░░░░]  45.7% (238/521)
  it (italian)         [████████░░░░░░░░░░░░]  44.1% (230/521)
  ko (koreana)         [███████████░░░░░░░░░]  55.9% (291/521)
  lv (latvian)         [████████████░░░░░░░░]  63.9% (333/521)
  pl (polish)          [████████████████████] 100.0% (521/521)
  ru (russian)         [███░░░░░░░░░░░░░░░░░]  17.7% (92/521)
  sv (swedish)         [███████████████░░░░░]  76.2% (397/521)
  tr (turkish)         [██████████░░░░░░░░░░]  51.2% (267/521)
  ua (ukrainian)       [████████████████████] 100.0% (521/521)
```

## Menu Translation Status

```
  ar    (arabic      ): File missing
  bg    (bulgarian   ): File missing
  chi   (schinese    ): File missing
  cze   (czech       ): File missing
  da    (danish      ): File missing
  de    (german      ): Complete
  el    (greek       ): File missing
  en    (english     ): Complete
  es    (spanish     ): File missing
  fi    (finnish     ): File missing
  fr    (french      ): File missing
  he    (hebrew      ): File missing
  hu    (hungarian   ): File missing
  it    (italian     ): File missing
  jp    (japanese    ): File missing
  ko    (korean      ): Complete
  lt    (lithuanian  ): File missing
  lv    (latvian     ): Complete
  nl    (dutch       ): Complete
  no    (norwegian   ): File missing
  pl    (polish      ): Complete
  pt    (brazilian   ): File missing
  pt_p  (portuguese  ): File missing
  ro    (romanian    ): File missing
  ru    (russian     ): File missing
  sk    (slovak      ): File missing
  sv    (swedish     ): Complete
  th    (thai        ): File missing
  tr    (turkish     ): File missing
  ua    (ukrainian   ): Complete
  vi    (vietnamese  ): File missing
  zho   (tchinese    ): File missing
```

## Reports

| File | Description |
|------|-------------|
| [translation-lint-results.txt](translation-lint-results.txt) | Linter errors and warnings |
| [missing-translations.txt](missing-translations.txt) | Detailed missing translations report |
| [missing-translations.json](missing-translations.json) | Machine-readable JSON export |

## Last Updated

- **Commit:** [`cdc74cb`](https://github.com/laazzee/cs2kz-metamod/commit/cdc74cbc644f6f71474fe3f7361678318fd56b37)
- **Time:** 2026-01-10 11:45:51 UTC
- **Message:** Fix crucial infinite loop issue with racing

---

*This report is automatically generated on each push to master.*
