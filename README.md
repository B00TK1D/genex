# genex
Generate regexes that tightly match input sets

Build: `gcc -o genex genex.c && chmod +x genex`

```
Usage: ./genex [-sSd:f:] [inputs...]\
  -s: Strict mode - do not attempt to expand charsets based on statistical analysis and common charsets\
  -S: Very strict mode - do not merge variables from different inputs into combined charsets\
  -d <directory>: Read inputs from a directory (every file in directory is read as its own input)\
  -f <file>: Read inputs from a file (every line in file is read as its own input)\
```

Examples:

`./genex -d tests/long_http`
